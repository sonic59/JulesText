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
AudioProcessorPlayer::AudioProcessorPlayer()
    : processor (nullptr),
      sampleRate (0),
      blockSize (0),
      isPrepared (false),
      numInputChans (0),
      numOutputChans (0),
      tempBuffer (1, 1)
{
}

AudioProcessorPlayer::~AudioProcessorPlayer()
{
    setProcessor (nullptr);
}

//==============================================================================
void AudioProcessorPlayer::setProcessor (AudioProcessor* const processorToPlay)
{
    if (processor != processorToPlay)
    {
        if (processorToPlay != nullptr && sampleRate > 0 && blockSize > 0)
        {
            processorToPlay->setPlayConfigDetails (numInputChans, numOutputChans,
                                                   sampleRate, blockSize);

            processorToPlay->prepareToPlay (sampleRate, blockSize);
        }

        AudioProcessor* oldOne;

        {
            const ScopedLock sl (lock);
            oldOne = isPrepared ? processor : nullptr;
            processor = processorToPlay;
            isPrepared = true;
        }

        if (oldOne != nullptr)
            oldOne->releaseResources();
    }
}

//==============================================================================
void AudioProcessorPlayer::audioDeviceIOCallback (const float** const inputChannelData,
                                                  const int numInputChannels,
                                                  float** const outputChannelData,
                                                  const int numOutputChannels,
                                                  const int numSamples)
{
    // these should have been prepared by audioDeviceAboutToStart()...
    jassert (sampleRate > 0 && blockSize > 0);

    incomingMidi.clear();
    messageCollector.removeNextBlockOfMessages (incomingMidi, numSamples);
    int i, totalNumChans = 0;

    if (numInputChannels > numOutputChannels)
    {
        // if there aren't enough output channels for the number of
        // inputs, we need to create some temporary extra ones (can't
        // use the input data in case it gets written to)
        tempBuffer.setSize (numInputChannels - numOutputChannels, numSamples,
                            false, false, true);

        for (i = 0; i < numOutputChannels; ++i)
        {
            channels[totalNumChans] = outputChannelData[i];
            memcpy (channels[totalNumChans], inputChannelData[i], sizeof (float) * numSamples);
            ++totalNumChans;
        }

        for (i = numOutputChannels; i < numInputChannels; ++i)
        {
            channels[totalNumChans] = tempBuffer.getSampleData (i - numOutputChannels, 0);
            memcpy (channels[totalNumChans], inputChannelData[i], sizeof (float) * numSamples);
            ++totalNumChans;
        }
    }
    else
    {
        for (i = 0; i < numInputChannels; ++i)
        {
            channels[totalNumChans] = outputChannelData[i];
            memcpy (channels[totalNumChans], inputChannelData[i], sizeof (float) * numSamples);
            ++totalNumChans;
        }

        for (i = numInputChannels; i < numOutputChannels; ++i)
        {
            channels[totalNumChans] = outputChannelData[i];
            zeromem (channels[totalNumChans], sizeof (float) * numSamples);
            ++totalNumChans;
        }
    }

    AudioSampleBuffer buffer (channels, totalNumChans, numSamples);

    const ScopedLock sl (lock);

    if (processor != nullptr)
    {
        const ScopedLock sl2 (processor->getCallbackLock());

        if (processor->isSuspended())
        {
            for (i = 0; i < numOutputChannels; ++i)
                zeromem (outputChannelData[i], sizeof (float) * numSamples);
        }
        else
        {
            processor->processBlock (buffer, incomingMidi);
        }
    }
}

void AudioProcessorPlayer::audioDeviceAboutToStart (AudioIODevice* device)
{
    const ScopedLock sl (lock);

    sampleRate = device->getCurrentSampleRate();
    blockSize = device->getCurrentBufferSizeSamples();
    numInputChans = device->getActiveInputChannels().countNumberOfSetBits();
    numOutputChans = device->getActiveOutputChannels().countNumberOfSetBits();

    messageCollector.reset (sampleRate);
    zeromem (channels, sizeof (channels));

    if (processor != nullptr)
    {
        if (isPrepared)
            processor->releaseResources();

        AudioProcessor* const oldProcessor = processor;
        setProcessor (nullptr);
        setProcessor (oldProcessor);
    }
}

void AudioProcessorPlayer::audioDeviceStopped()
{
    const ScopedLock sl (lock);

    if (processor != nullptr && isPrepared)
        processor->releaseResources();

    sampleRate = 0.0;
    blockSize = 0;
    isPrepared = false;
    tempBuffer.setSize (1, 1);
}

void AudioProcessorPlayer::handleIncomingMidiMessage (MidiInput*, const MidiMessage& message)
{
    messageCollector.addMessageToQueue (message);
}


END_JUCE_NAMESPACE
