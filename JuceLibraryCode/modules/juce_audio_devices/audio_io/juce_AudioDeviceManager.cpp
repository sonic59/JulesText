/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-11 by Raw Material Software Ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the GNU General
   Public License (Version 2), as published by the Free Software Foundation.
   A copy of the license is included in the JUCE distribution, or can be found
   online at www.gnu.org/licenses.

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.rawmaterialsoftware.com/juce for more information.

  ==============================================================================
*/

BEGIN_JUCE_NAMESPACE

//==============================================================================
AudioDeviceManager::AudioDeviceSetup::AudioDeviceSetup()
    : sampleRate (0),
      bufferSize (0),
      useDefaultInputChannels (true),
      useDefaultOutputChannels (true)
{
}

bool AudioDeviceManager::AudioDeviceSetup::operator== (const AudioDeviceManager::AudioDeviceSetup& other) const
{
    return outputDeviceName == other.outputDeviceName
            && inputDeviceName == other.inputDeviceName
            && sampleRate == other.sampleRate
            && bufferSize == other.bufferSize
            && inputChannels == other.inputChannels
            && useDefaultInputChannels == other.useDefaultInputChannels
            && outputChannels == other.outputChannels
            && useDefaultOutputChannels == other.useDefaultOutputChannels;
}

//==============================================================================
AudioDeviceManager::AudioDeviceManager()
    : numInputChansNeeded (0),
      numOutputChansNeeded (2),
      listNeedsScanning (true),
      useInputNames (false),
      inputLevelMeasurementEnabledCount (0),
      inputLevel (0),
      tempBuffer (2, 2),
      cpuUsageMs (0),
      timeToCpuScale (0)
{
    callbackHandler.owner = this;
}

AudioDeviceManager::~AudioDeviceManager()
{
    currentAudioDevice = nullptr;
    defaultMidiOutput = nullptr;
}


//==============================================================================
void AudioDeviceManager::createDeviceTypesIfNeeded()
{
    if (availableDeviceTypes.size() == 0)
    {
        createAudioDeviceTypes (availableDeviceTypes);

        while (lastDeviceTypeConfigs.size() < availableDeviceTypes.size())
            lastDeviceTypeConfigs.add (new AudioDeviceSetup());

        if (availableDeviceTypes.size() > 0)
            currentDeviceType = availableDeviceTypes.getUnchecked(0)->getTypeName();

        for (int i = 0; i < availableDeviceTypes.size(); ++i)
            availableDeviceTypes.getUnchecked(i)->addListener (&callbackHandler);
    }
}

const OwnedArray <AudioIODeviceType>& AudioDeviceManager::getAvailableDeviceTypes()
{
    scanDevicesIfNeeded();
    return availableDeviceTypes;
}

void AudioDeviceManager::audioDeviceListChanged()
{
    sendChangeMessage();
}

//==============================================================================
static void addIfNotNull (OwnedArray <AudioIODeviceType>& list, AudioIODeviceType* const device)
{
    if (device != nullptr)
        list.add (device);
}

void AudioDeviceManager::createAudioDeviceTypes (OwnedArray <AudioIODeviceType>& list)
{
    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_WASAPI());
    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_DirectSound());
    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_ASIO());
    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_CoreAudio());
    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_iOSAudio());
    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_ALSA());
    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_JACK());
    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_Android());
}

//==============================================================================
String AudioDeviceManager::initialise (const int numInputChannelsNeeded,
                                       const int numOutputChannelsNeeded,
                                       const XmlElement* const e,
                                       const bool selectDefaultDeviceOnFailure,
                                       const String& preferredDefaultDeviceName,
                                       const AudioDeviceSetup* preferredSetupOptions)
{
    scanDevicesIfNeeded();

    numInputChansNeeded = numInputChannelsNeeded;
    numOutputChansNeeded = numOutputChannelsNeeded;

    if (e != nullptr && e->hasTagName ("DEVICESETUP"))
    {
        lastExplicitSettings = new XmlElement (*e);

        String error;
        AudioDeviceSetup setup;

        if (preferredSetupOptions != nullptr)
            setup = *preferredSetupOptions;

        if (e->getStringAttribute ("audioDeviceName").isNotEmpty())
        {
            setup.inputDeviceName = setup.outputDeviceName
                = e->getStringAttribute ("audioDeviceName");
        }
        else
        {
            setup.inputDeviceName = e->getStringAttribute ("audioInputDeviceName");
            setup.outputDeviceName = e->getStringAttribute ("audioOutputDeviceName");
        }

        currentDeviceType = e->getStringAttribute ("deviceType");
        if (currentDeviceType.isEmpty())
        {
            AudioIODeviceType* const type = findType (setup.inputDeviceName, setup.outputDeviceName);

            if (type != nullptr)
                currentDeviceType = type->getTypeName();
            else if (availableDeviceTypes.size() > 0)
                currentDeviceType = availableDeviceTypes[0]->getTypeName();
        }

        setup.bufferSize = e->getIntAttribute ("audioDeviceBufferSize");
        setup.sampleRate = e->getDoubleAttribute ("audioDeviceRate");

        setup.inputChannels.parseString (e->getStringAttribute ("audioDeviceInChans", "11"), 2);
        setup.outputChannels.parseString (e->getStringAttribute ("audioDeviceOutChans", "11"), 2);

        setup.useDefaultInputChannels = ! e->hasAttribute ("audioDeviceInChans");
        setup.useDefaultOutputChannels = ! e->hasAttribute ("audioDeviceOutChans");

        error = setAudioDeviceSetup (setup, true);

        midiInsFromXml.clear();
        forEachXmlChildElementWithTagName (*e, c, "MIDIINPUT")
            midiInsFromXml.add (c->getStringAttribute ("name"));

        const StringArray allMidiIns (MidiInput::getDevices());

        for (int i = allMidiIns.size(); --i >= 0;)
            setMidiInputEnabled (allMidiIns[i], midiInsFromXml.contains (allMidiIns[i]));

        if (error.isNotEmpty() && selectDefaultDeviceOnFailure)
            error = initialise (numInputChannelsNeeded, numOutputChannelsNeeded, 0,
                                false, preferredDefaultDeviceName);

        setDefaultMidiOutput (e->getStringAttribute ("defaultMidiOutput"));

        return error;
    }
    else
    {
        AudioDeviceSetup setup;

        if (preferredSetupOptions != nullptr)
        {
            setup = *preferredSetupOptions;
        }
        else if (preferredDefaultDeviceName.isNotEmpty())
        {
            for (int j = availableDeviceTypes.size(); --j >= 0;)
            {
                AudioIODeviceType* const type = availableDeviceTypes.getUnchecked(j);

                StringArray outs (type->getDeviceNames (false));

                int i;
                for (i = 0; i < outs.size(); ++i)
                {
                    if (outs[i].matchesWildcard (preferredDefaultDeviceName, true))
                    {
                        setup.outputDeviceName = outs[i];
                        break;
                    }
                }

                StringArray ins (type->getDeviceNames (true));

                for (i = 0; i < ins.size(); ++i)
                {
                    if (ins[i].matchesWildcard (preferredDefaultDeviceName, true))
                    {
                        setup.inputDeviceName = ins[i];
                        break;
                    }
                }
            }
        }

        insertDefaultDeviceNames (setup);
        return setAudioDeviceSetup (setup, false);
    }
}

void AudioDeviceManager::insertDefaultDeviceNames (AudioDeviceSetup& setup) const
{
    AudioIODeviceType* type = getCurrentDeviceTypeObject();
    if (type != nullptr)
    {
        if (setup.outputDeviceName.isEmpty())
            setup.outputDeviceName = type->getDeviceNames (false) [type->getDefaultDeviceIndex (false)];

        if (setup.inputDeviceName.isEmpty())
            setup.inputDeviceName = type->getDeviceNames (true) [type->getDefaultDeviceIndex (true)];
    }
}

XmlElement* AudioDeviceManager::createStateXml() const
{
    return lastExplicitSettings != nullptr ? new XmlElement (*lastExplicitSettings) : nullptr;
}

//==============================================================================
void AudioDeviceManager::scanDevicesIfNeeded()
{
    if (listNeedsScanning)
    {
        listNeedsScanning = false;

        createDeviceTypesIfNeeded();

        for (int i = availableDeviceTypes.size(); --i >= 0;)
            availableDeviceTypes.getUnchecked(i)->scanForDevices();
    }
}

AudioIODeviceType* AudioDeviceManager::findType (const String& inputName, const String& outputName)
{
    scanDevicesIfNeeded();

    for (int i = availableDeviceTypes.size(); --i >= 0;)
    {
        AudioIODeviceType* const type = availableDeviceTypes.getUnchecked(i);

        if ((inputName.isNotEmpty() && type->getDeviceNames (true).contains (inputName, true))
            || (outputName.isNotEmpty() && type->getDeviceNames (false).contains (outputName, true)))
        {
            return type;
        }
    }

    return nullptr;
}

void AudioDeviceManager::getAudioDeviceSetup (AudioDeviceSetup& setup)
{
    setup = currentSetup;
}

void AudioDeviceManager::deleteCurrentDevice()
{
    currentAudioDevice = nullptr;
    currentSetup.inputDeviceName = String::empty;
    currentSetup.outputDeviceName = String::empty;
}

void AudioDeviceManager::setCurrentAudioDeviceType (const String& type,
                                                    const bool treatAsChosenDevice)
{
    for (int i = 0; i < availableDeviceTypes.size(); ++i)
    {
        if (availableDeviceTypes.getUnchecked(i)->getTypeName() == type
             && currentDeviceType != type)
        {
            currentDeviceType = type;

            AudioDeviceSetup s (*lastDeviceTypeConfigs.getUnchecked(i));
            insertDefaultDeviceNames (s);

            setAudioDeviceSetup (s, treatAsChosenDevice);

            sendChangeMessage();
            break;
        }
    }
}

AudioIODeviceType* AudioDeviceManager::getCurrentDeviceTypeObject() const
{
    for (int i = 0; i < availableDeviceTypes.size(); ++i)
        if (availableDeviceTypes[i]->getTypeName() == currentDeviceType)
            return availableDeviceTypes[i];

    return availableDeviceTypes[0];
}

String AudioDeviceManager::setAudioDeviceSetup (const AudioDeviceSetup& newSetup,
                                                const bool treatAsChosenDevice)
{
    jassert (&newSetup != &currentSetup);    // this will have no effect

    if (newSetup == currentSetup && currentAudioDevice != nullptr)
        return String::empty;

    if (! (newSetup == currentSetup))
        sendChangeMessage();

    stopDevice();

    const String newInputDeviceName (numInputChansNeeded == 0 ? String::empty : newSetup.inputDeviceName);
    const String newOutputDeviceName (numOutputChansNeeded == 0 ? String::empty : newSetup.outputDeviceName);

    String error;
    AudioIODeviceType* type = getCurrentDeviceTypeObject();

    if (type == nullptr || (newInputDeviceName.isEmpty() && newOutputDeviceName.isEmpty()))
    {
        deleteCurrentDevice();

        if (treatAsChosenDevice)
            updateXml();

        return String::empty;
    }

    if (currentSetup.inputDeviceName != newInputDeviceName
         || currentSetup.outputDeviceName != newOutputDeviceName
         || currentAudioDevice == nullptr)
    {
        deleteCurrentDevice();
        scanDevicesIfNeeded();

        if (newOutputDeviceName.isNotEmpty()
             && ! type->getDeviceNames (false).contains (newOutputDeviceName))
        {
            return "No such device: " + newOutputDeviceName;
        }

        if (newInputDeviceName.isNotEmpty()
             && ! type->getDeviceNames (true).contains (newInputDeviceName))
        {
            return "No such device: " + newInputDeviceName;
        }

        currentAudioDevice = type->createDevice (newOutputDeviceName, newInputDeviceName);

        if (currentAudioDevice == nullptr)
            error = "Can't open the audio device!\n\nThis may be because another application is currently using the same device - if so, you should close any other applications and try again!";
        else
            error = currentAudioDevice->getLastError();

        if (error.isNotEmpty())
        {
            deleteCurrentDevice();
            return error;
        }

        if (newSetup.useDefaultInputChannels)
        {
            inputChannels.clear();
            inputChannels.setRange (0, numInputChansNeeded, true);
        }

        if (newSetup.useDefaultOutputChannels)
        {
            outputChannels.clear();
            outputChannels.setRange (0, numOutputChansNeeded, true);
        }

        if (newInputDeviceName.isEmpty())
            inputChannels.clear();

        if (newOutputDeviceName.isEmpty())
            outputChannels.clear();
    }

    if (! newSetup.useDefaultInputChannels)
        inputChannels = newSetup.inputChannels;

    if (! newSetup.useDefaultOutputChannels)
        outputChannels = newSetup.outputChannels;

    currentSetup = newSetup;

    currentSetup.sampleRate = chooseBestSampleRate (newSetup.sampleRate);
    currentSetup.bufferSize = chooseBestBufferSize (newSetup.bufferSize);

    error = currentAudioDevice->open (inputChannels,
                                      outputChannels,
                                      currentSetup.sampleRate,
                                      currentSetup.bufferSize);

    if (error.isEmpty())
    {
        currentDeviceType = currentAudioDevice->getTypeName();

        currentAudioDevice->start (&callbackHandler);

        currentSetup.sampleRate = currentAudioDevice->getCurrentSampleRate();
        currentSetup.bufferSize = currentAudioDevice->getCurrentBufferSizeSamples();
        currentSetup.inputChannels = currentAudioDevice->getActiveInputChannels();
        currentSetup.outputChannels = currentAudioDevice->getActiveOutputChannels();

        for (int i = 0; i < availableDeviceTypes.size(); ++i)
            if (availableDeviceTypes.getUnchecked (i)->getTypeName() == currentDeviceType)
                *(lastDeviceTypeConfigs.getUnchecked (i)) = currentSetup;

        if (treatAsChosenDevice)
            updateXml();
    }
    else
    {
        deleteCurrentDevice();
    }

    return error;
}

double AudioDeviceManager::chooseBestSampleRate (double rate) const
{
    jassert (currentAudioDevice != nullptr);

    if (rate > 0)
        for (int i = currentAudioDevice->getNumSampleRates(); --i >= 0;)
            if (currentAudioDevice->getSampleRate (i) == rate)
                return rate;

    double lowestAbove44 = 0.0;

    for (int i = currentAudioDevice->getNumSampleRates(); --i >= 0;)
    {
        const double sr = currentAudioDevice->getSampleRate (i);

        if (sr >= 44100.0 && (lowestAbove44 < 1.0 || sr < lowestAbove44))
            lowestAbove44 = sr;
    }

    if (lowestAbove44 > 0.0)
        return lowestAbove44;

    return currentAudioDevice->getSampleRate (0);
}

int AudioDeviceManager::chooseBestBufferSize (int bufferSize) const
{
    jassert (currentAudioDevice != nullptr);

    if (bufferSize > 0)
        for (int i = currentAudioDevice->getNumBufferSizesAvailable(); --i >= 0;)
            if (currentAudioDevice->getBufferSizeSamples(i) == bufferSize)
                return bufferSize;

    return currentAudioDevice->getDefaultBufferSize();
}

void AudioDeviceManager::stopDevice()
{
    if (currentAudioDevice != nullptr)
        currentAudioDevice->stop();

    testSound = nullptr;
}

void AudioDeviceManager::closeAudioDevice()
{
    stopDevice();
    currentAudioDevice = nullptr;
}

void AudioDeviceManager::restartLastAudioDevice()
{
    if (currentAudioDevice == nullptr)
    {
        if (currentSetup.inputDeviceName.isEmpty()
              && currentSetup.outputDeviceName.isEmpty())
        {
            // This method will only reload the last device that was running
            // before closeAudioDevice() was called - you need to actually open
            // one first, with setAudioDevice().
            jassertfalse;
            return;
        }

        AudioDeviceSetup s (currentSetup);
        setAudioDeviceSetup (s, false);
    }
}

void AudioDeviceManager::updateXml()
{
    lastExplicitSettings = new XmlElement ("DEVICESETUP");

    lastExplicitSettings->setAttribute ("deviceType", currentDeviceType);
    lastExplicitSettings->setAttribute ("audioOutputDeviceName", currentSetup.outputDeviceName);
    lastExplicitSettings->setAttribute ("audioInputDeviceName", currentSetup.inputDeviceName);

    if (currentAudioDevice != nullptr)
    {
        lastExplicitSettings->setAttribute ("audioDeviceRate", currentAudioDevice->getCurrentSampleRate());

        if (currentAudioDevice->getDefaultBufferSize() != currentAudioDevice->getCurrentBufferSizeSamples())
            lastExplicitSettings->setAttribute ("audioDeviceBufferSize", currentAudioDevice->getCurrentBufferSizeSamples());

        if (! currentSetup.useDefaultInputChannels)
            lastExplicitSettings->setAttribute ("audioDeviceInChans", currentSetup.inputChannels.toString (2));

        if (! currentSetup.useDefaultOutputChannels)
            lastExplicitSettings->setAttribute ("audioDeviceOutChans", currentSetup.outputChannels.toString (2));
    }

    for (int i = 0; i < enabledMidiInputs.size(); ++i)
    {
        XmlElement* const m = lastExplicitSettings->createNewChildElement ("MIDIINPUT");
        m->setAttribute ("name", enabledMidiInputs[i]->getName());
    }

    if (midiInsFromXml.size() > 0)
    {
        // Add any midi devices that have been enabled before, but which aren't currently
        // open because the device has been disconnected.
        const StringArray availableMidiDevices (MidiInput::getDevices());

        for (int i = 0; i < midiInsFromXml.size(); ++i)
        {
            if (! availableMidiDevices.contains (midiInsFromXml[i], true))
            {
                XmlElement* const m = lastExplicitSettings->createNewChildElement ("MIDIINPUT");
                m->setAttribute ("name", midiInsFromXml[i]);
            }
        }
    }

    if (defaultMidiOutputName.isNotEmpty())
        lastExplicitSettings->setAttribute ("defaultMidiOutput", defaultMidiOutputName);
}

//==============================================================================
void AudioDeviceManager::addAudioCallback (AudioIODeviceCallback* newCallback)
{
    {
        const ScopedLock sl (audioCallbackLock);
        if (callbacks.contains (newCallback))
            return;
    }

    if (currentAudioDevice != nullptr && newCallback != nullptr)
        newCallback->audioDeviceAboutToStart (currentAudioDevice);

    const ScopedLock sl (audioCallbackLock);
    callbacks.add (newCallback);
}

void AudioDeviceManager::removeAudioCallback (AudioIODeviceCallback* callbackToRemove)
{
    if (callbackToRemove != nullptr)
    {
        bool needsDeinitialising = currentAudioDevice != nullptr;

        {
            const ScopedLock sl (audioCallbackLock);

            needsDeinitialising = needsDeinitialising && callbacks.contains (callbackToRemove);
            callbacks.removeValue (callbackToRemove);
        }

        if (needsDeinitialising)
            callbackToRemove->audioDeviceStopped();
    }
}

void AudioDeviceManager::audioDeviceIOCallbackInt (const float** inputChannelData,
                                                   int numInputChannels,
                                                   float** outputChannelData,
                                                   int numOutputChannels,
                                                   int numSamples)
{
    const ScopedLock sl (audioCallbackLock);

    if (inputLevelMeasurementEnabledCount > 0 && numInputChannels > 0)
    {
        for (int j = 0; j < numSamples; ++j)
        {
            float s = 0;

            for (int i = 0; i < numInputChannels; ++i)
                s += std::abs (inputChannelData[i][j]);

            s /= numInputChannels;

            const double decayFactor = 0.99992;

            if (s > inputLevel)
                inputLevel = s;
            else if (inputLevel > 0.001f)
                inputLevel *= decayFactor;
            else
                inputLevel = 0;
        }
    }
    else
    {
        inputLevel = 0;
    }

    if (callbacks.size() > 0)
    {
        const double callbackStartTime = Time::getMillisecondCounterHiRes();

        tempBuffer.setSize (jmax (1, numOutputChannels), jmax (1, numSamples), false, false, true);

        callbacks.getUnchecked(0)->audioDeviceIOCallback (inputChannelData, numInputChannels,
                                                          outputChannelData, numOutputChannels, numSamples);

        float** const tempChans = tempBuffer.getArrayOfChannels();

        for (int i = callbacks.size(); --i > 0;)
        {
            callbacks.getUnchecked(i)->audioDeviceIOCallback (inputChannelData, numInputChannels,
                                                              tempChans, numOutputChannels, numSamples);

            for (int chan = 0; chan < numOutputChannels; ++chan)
            {
                const float* const src = tempChans [chan];
                float* const dst = outputChannelData [chan];

                if (src != nullptr && dst != nullptr)
                    for (int j = 0; j < numSamples; ++j)
                        dst[j] += src[j];
            }
        }

        const double msTaken = Time::getMillisecondCounterHiRes() - callbackStartTime;
        const double filterAmount = 0.2;
        cpuUsageMs += filterAmount * (msTaken - cpuUsageMs);
    }
    else
    {
        for (int i = 0; i < numOutputChannels; ++i)
            zeromem (outputChannelData[i], sizeof (float) * numSamples);
    }

    if (testSound != nullptr)
    {
        const int numSamps = jmin (numSamples, testSound->getNumSamples() - testSoundPosition);
        const float* const src = testSound->getSampleData (0, testSoundPosition);

        for (int i = 0; i < numOutputChannels; ++i)
            for (int j = 0; j < numSamps; ++j)
                outputChannelData [i][j] += src[j];

        testSoundPosition += numSamps;
        if (testSoundPosition >= testSound->getNumSamples())
            testSound = nullptr;
    }
}

void AudioDeviceManager::audioDeviceAboutToStartInt (AudioIODevice* const device)
{
    cpuUsageMs = 0;

    const double sampleRate = device->getCurrentSampleRate();
    const int blockSize = device->getCurrentBufferSizeSamples();

    if (sampleRate > 0.0 && blockSize > 0)
    {
        const double msPerBlock = 1000.0 * blockSize / sampleRate;
        timeToCpuScale = (msPerBlock > 0.0) ? (1.0 / msPerBlock) : 0.0;
    }

    {
        const ScopedLock sl (audioCallbackLock);
        for (int i = callbacks.size(); --i >= 0;)
            callbacks.getUnchecked(i)->audioDeviceAboutToStart (device);
    }

    sendChangeMessage();
}

void AudioDeviceManager::audioDeviceStoppedInt()
{
    cpuUsageMs = 0;
    timeToCpuScale = 0;
    sendChangeMessage();

    const ScopedLock sl (audioCallbackLock);
    for (int i = callbacks.size(); --i >= 0;)
        callbacks.getUnchecked(i)->audioDeviceStopped();
}

double AudioDeviceManager::getCpuUsage() const
{
    return jlimit (0.0, 1.0, timeToCpuScale * cpuUsageMs);
}

//==============================================================================
void AudioDeviceManager::setMidiInputEnabled (const String& name,
                                              const bool enabled)
{
    if (enabled != isMidiInputEnabled (name))
    {
        if (enabled)
        {
            const int index = MidiInput::getDevices().indexOf (name);

            if (index >= 0)
            {
                MidiInput* const midiIn = MidiInput::openDevice (index, &callbackHandler);

                if (midiIn != nullptr)
                {
                    enabledMidiInputs.add (midiIn);
                    midiIn->start();
                }
            }
        }
        else
        {
            for (int i = enabledMidiInputs.size(); --i >= 0;)
                if (enabledMidiInputs[i]->getName() == name)
                    enabledMidiInputs.remove (i);
        }

        updateXml();
        sendChangeMessage();
    }
}

bool AudioDeviceManager::isMidiInputEnabled (const String& name) const
{
    for (int i = enabledMidiInputs.size(); --i >= 0;)
        if (enabledMidiInputs[i]->getName() == name)
            return true;

    return false;
}

void AudioDeviceManager::addMidiInputCallback (const String& name,
                                               MidiInputCallback* callbackToAdd)
{
    removeMidiInputCallback (name, callbackToAdd);

    if (name.isEmpty() || isMidiInputEnabled (name))
    {
        const ScopedLock sl (midiCallbackLock);
        midiCallbacks.add (callbackToAdd);
        midiCallbackDevices.add (name);
    }
}

void AudioDeviceManager::removeMidiInputCallback (const String& name, MidiInputCallback* callbackToRemove)
{
    for (int i = midiCallbacks.size(); --i >= 0;)
    {
        if (midiCallbackDevices[i] == name && midiCallbacks.getUnchecked(i) == callbackToRemove)
        {
            const ScopedLock sl (midiCallbackLock);
            midiCallbacks.remove (i);
            midiCallbackDevices.remove (i);
        }
    }
}

void AudioDeviceManager::handleIncomingMidiMessageInt (MidiInput* source,
                                                       const MidiMessage& message)
{
    if (! message.isActiveSense())
    {
        const bool isDefaultSource = (source == nullptr || source == enabledMidiInputs.getFirst());

        const ScopedLock sl (midiCallbackLock);

        for (int i = midiCallbackDevices.size(); --i >= 0;)
        {
            const String name (midiCallbackDevices[i]);

            if ((isDefaultSource && name.isEmpty()) || (name.isNotEmpty() && name == source->getName()))
                midiCallbacks.getUnchecked(i)->handleIncomingMidiMessage (source, message);
        }
    }
}

//==============================================================================
void AudioDeviceManager::setDefaultMidiOutput (const String& deviceName)
{
    if (defaultMidiOutputName != deviceName)
    {
        Array <AudioIODeviceCallback*> oldCallbacks;

        {
            const ScopedLock sl (audioCallbackLock);
            oldCallbacks = callbacks;
            callbacks.clear();
        }

        if (currentAudioDevice != nullptr)
            for (int i = oldCallbacks.size(); --i >= 0;)
                oldCallbacks.getUnchecked(i)->audioDeviceStopped();

        defaultMidiOutput = nullptr;
        defaultMidiOutputName = deviceName;

        if (deviceName.isNotEmpty())
            defaultMidiOutput = MidiOutput::openDevice (MidiOutput::getDevices().indexOf (deviceName));

        if (currentAudioDevice != nullptr)
            for (int i = oldCallbacks.size(); --i >= 0;)
                oldCallbacks.getUnchecked(i)->audioDeviceAboutToStart (currentAudioDevice);

        {
            const ScopedLock sl (audioCallbackLock);
            callbacks = oldCallbacks;
        }

        updateXml();
        sendChangeMessage();
    }
}

//==============================================================================
void AudioDeviceManager::CallbackHandler::audioDeviceIOCallback (const float** inputChannelData,
                                                                 int numInputChannels,
                                                                 float** outputChannelData,
                                                                 int numOutputChannels,
                                                                 int numSamples)
{
    owner->audioDeviceIOCallbackInt (inputChannelData, numInputChannels, outputChannelData, numOutputChannels, numSamples);
}

void AudioDeviceManager::CallbackHandler::audioDeviceAboutToStart (AudioIODevice* device)
{
    owner->audioDeviceAboutToStartInt (device);
}

void AudioDeviceManager::CallbackHandler::audioDeviceStopped()
{
    owner->audioDeviceStoppedInt();
}

void AudioDeviceManager::CallbackHandler::handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message)
{
    owner->handleIncomingMidiMessageInt (source, message);
}

void AudioDeviceManager::CallbackHandler::audioDeviceListChanged()
{
    owner->audioDeviceListChanged();
}

//==============================================================================
void AudioDeviceManager::playTestSound()
{
    { // cunningly nested to swap, unlock and delete in that order.
        ScopedPointer <AudioSampleBuffer> oldSound;

        {
            const ScopedLock sl (audioCallbackLock);
            oldSound = testSound;
        }
    }

    testSoundPosition = 0;

    if (currentAudioDevice != nullptr)
    {
        const double sampleRate = currentAudioDevice->getCurrentSampleRate();
        const int soundLength = (int) sampleRate;

        AudioSampleBuffer* const newSound = new AudioSampleBuffer (1, soundLength);
        float* samples = newSound->getSampleData (0);

        const double frequency = MidiMessage::getMidiNoteInHertz (80);
        const float amplitude = 0.5f;

        const double phasePerSample = double_Pi * 2.0 / (sampleRate / frequency);

        for (int i = 0; i < soundLength; ++i)
            samples[i] = amplitude * (float) std::sin (i * phasePerSample);

        newSound->applyGainRamp (0, 0, soundLength / 10, 0.0f, 1.0f);
        newSound->applyGainRamp (0, soundLength - soundLength / 4, soundLength / 4, 1.0f, 0.0f);

        const ScopedLock sl (audioCallbackLock);
        testSound = newSound;
    }
}

void AudioDeviceManager::enableInputLevelMeasurement (const bool enableMeasurement)
{
    const ScopedLock sl (audioCallbackLock);

    if (enableMeasurement)
        ++inputLevelMeasurementEnabledCount;
    else
        --inputLevelMeasurementEnabledCount;

    inputLevel = 0;
}

double AudioDeviceManager::getCurrentInputLevel() const
{
    jassert (inputLevelMeasurementEnabledCount > 0); // you need to call enableInputLevelMeasurement() before using this!
    return inputLevel;
}


END_JUCE_NAMESPACE
