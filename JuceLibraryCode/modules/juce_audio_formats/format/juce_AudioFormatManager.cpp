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
AudioFormatManager::AudioFormatManager()
    : defaultFormatIndex (0)
{
}

AudioFormatManager::~AudioFormatManager()
{
}

//==============================================================================
void AudioFormatManager::registerFormat (AudioFormat* newFormat, const bool makeThisTheDefaultFormat)
{
    jassert (newFormat != nullptr);

    if (newFormat != nullptr)
    {
       #if JUCE_DEBUG
        for (int i = getNumKnownFormats(); --i >= 0;)
        {
            if (getKnownFormat (i)->getFormatName() == newFormat->getFormatName())
            {
                jassertfalse; // trying to add the same format twice!
            }
        }
       #endif

        if (makeThisTheDefaultFormat)
            defaultFormatIndex = getNumKnownFormats();

        knownFormats.add (newFormat);
    }
}

void AudioFormatManager::registerBasicFormats()
{
    registerFormat (new WavAudioFormat(), true);
    registerFormat (new AiffAudioFormat(), false);

   #if JUCE_MAC || JUCE_IOS
    registerFormat (new CoreAudioFormat(), false);
   #endif

   #if JUCE_USE_FLAC
    registerFormat (new FlacAudioFormat(), false);
   #endif

   #if JUCE_USE_OGGVORBIS
    registerFormat (new OggVorbisAudioFormat(), false);
   #endif
}

void AudioFormatManager::clearFormats()
{
    knownFormats.clear();
    defaultFormatIndex = 0;
}

int AudioFormatManager::getNumKnownFormats() const
{
    return knownFormats.size();
}

AudioFormat* AudioFormatManager::getKnownFormat (const int index) const
{
    return knownFormats [index];
}

AudioFormat* AudioFormatManager::getDefaultFormat() const
{
    return getKnownFormat (defaultFormatIndex);
}

AudioFormat* AudioFormatManager::findFormatForFileExtension (const String& fileExtension) const
{
    if (! fileExtension.startsWithChar ('.'))
        return findFormatForFileExtension ("." + fileExtension);

    for (int i = 0; i < getNumKnownFormats(); ++i)
        if (getKnownFormat(i)->getFileExtensions().contains (fileExtension, true))
            return getKnownFormat(i);

    return nullptr;
}

String AudioFormatManager::getWildcardForAllFormats() const
{
    StringArray extensions;

    int i;
    for (i = 0; i < getNumKnownFormats(); ++i)
        extensions.addArray (getKnownFormat(i)->getFileExtensions());

    extensions.trim();
    extensions.removeEmptyStrings();

    for (i = 0; i < extensions.size(); ++i)
        extensions.set (i, (extensions[i].startsWithChar ('.') ? "*" : "*.") + extensions[i]);

    extensions.removeDuplicates (true);
    return extensions.joinIntoString (";");
}

//==============================================================================
AudioFormatReader* AudioFormatManager::createReaderFor (const File& file)
{
    // you need to actually register some formats before the manager can
    // use them to open a file!
    jassert (getNumKnownFormats() > 0);

    for (int i = 0; i < getNumKnownFormats(); ++i)
    {
        AudioFormat* const af = getKnownFormat(i);

        if (af->canHandleFile (file))
        {
            InputStream* const in = file.createInputStream();

            if (in != nullptr)
            {
                AudioFormatReader* const r = af->createReaderFor (in, true);

                if (r != nullptr)
                    return r;
            }
        }
    }

    return nullptr;
}

AudioFormatReader* AudioFormatManager::createReaderFor (InputStream* audioFileStream)
{
    // you need to actually register some formats before the manager can
    // use them to open a file!
    jassert (getNumKnownFormats() > 0);

    ScopedPointer <InputStream> in (audioFileStream);

    if (in != nullptr)
    {
        const int64 originalStreamPos = in->getPosition();

        for (int i = 0; i < getNumKnownFormats(); ++i)
        {
            AudioFormatReader* const r = getKnownFormat(i)->createReaderFor (in, false);

            if (r != nullptr)
            {
                in.release();
                return r;
            }

            in->setPosition (originalStreamPos);

            // the stream that is passed-in must be capable of being repositioned so
            // that all the formats can have a go at opening it.
            jassert (in->getPosition() == originalStreamPos);
        }
    }

    return nullptr;
}


END_JUCE_NAMESPACE
