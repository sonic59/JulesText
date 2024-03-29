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
AudioFormatReader::AudioFormatReader (InputStream* const in,
                                      const String& formatName_)
    : sampleRate (0),
      bitsPerSample (0),
      lengthInSamples (0),
      numChannels (0),
      usesFloatingPointData (false),
      input (in),
      formatName (formatName_)
{
}

AudioFormatReader::~AudioFormatReader()
{
    delete input;
}

bool AudioFormatReader::read (int* const* destSamples,
                              int numDestChannels,
                              int64 startSampleInSource,
                              int numSamplesToRead,
                              const bool fillLeftoverChannelsWithCopies)
{
    jassert (numDestChannels > 0); // you have to actually give this some channels to work with!

    int startOffsetInDestBuffer = 0;

    if (startSampleInSource < 0)
    {
        const int silence = (int) jmin (-startSampleInSource, (int64) numSamplesToRead);

        for (int i = numDestChannels; --i >= 0;)
            if (destSamples[i] != nullptr)
                zeromem (destSamples[i], sizeof (int) * silence);

        startOffsetInDestBuffer += silence;
        numSamplesToRead -= silence;
        startSampleInSource = 0;
    }

    if (numSamplesToRead <= 0)
        return true;

    if (! readSamples (const_cast <int**> (destSamples),
                       jmin ((int) numChannels, numDestChannels), startOffsetInDestBuffer,
                       startSampleInSource, numSamplesToRead))
        return false;

    if (numDestChannels > (int) numChannels)
    {
        if (fillLeftoverChannelsWithCopies)
        {
            int* lastFullChannel = destSamples[0];

            for (int i = (int) numChannels; --i > 0;)
            {
                if (destSamples[i] != nullptr)
                {
                    lastFullChannel = destSamples[i];
                    break;
                }
            }

            if (lastFullChannel != nullptr)
                for (int i = (int) numChannels; i < numDestChannels; ++i)
                    if (destSamples[i] != nullptr)
                        memcpy (destSamples[i], lastFullChannel, sizeof (int) * numSamplesToRead);
        }
        else
        {
            for (int i = (int) numChannels; i < numDestChannels; ++i)
                if (destSamples[i] != nullptr)
                    zeromem (destSamples[i], sizeof (int) * numSamplesToRead);
        }
    }

    return true;
}

void AudioFormatReader::read (AudioSampleBuffer* buffer,
                              int startSample,
                              int numSamples,
                              int64 readerStartSample,
                              bool useReaderLeftChan,
                              bool useReaderRightChan)
{
    jassert (buffer != nullptr);
    jassert (startSample >= 0 && startSample + numSamples <= buffer->getNumSamples());

    if (numSamples > 0)
    {
        int* chans[3];

        if (useReaderLeftChan == useReaderRightChan)
        {
            chans[0] = reinterpret_cast<int*> (buffer->getSampleData (0, startSample));
            chans[1] = (numChannels > 1 && buffer->getNumChannels() > 1) ? reinterpret_cast<int*> (buffer->getSampleData (1, startSample)) : nullptr;
        }
        else if (useReaderLeftChan || (numChannels == 1))
        {
            chans[0] = reinterpret_cast<int*> (buffer->getSampleData (0, startSample));
            chans[1] = nullptr;
        }
        else if (useReaderRightChan)
        {
            chans[0] = nullptr;
            chans[1] = reinterpret_cast<int*> (buffer->getSampleData (0, startSample));
        }

        chans[2] = nullptr;

        read (chans, 2, readerStartSample, numSamples, true);

        if (! usesFloatingPointData)
        {
            for (int j = 0; j < 2; ++j)
            {
                float* const d = reinterpret_cast <float*> (chans[j]);

                if (d != nullptr)
                {
                    const float multiplier = 1.0f / 0x7fffffff;

                    for (int i = 0; i < numSamples; ++i)
                        d[i] = *reinterpret_cast<int*> (d + i) * multiplier;
                }
            }
        }

        if (numChannels > 1 && (chans[0] == nullptr || chans[1] == nullptr))
        {
            // if this is a stereo buffer and the source was mono, dupe the first channel..
            memcpy (buffer->getSampleData (1, startSample),
                    buffer->getSampleData (0, startSample),
                    sizeof (float) * numSamples);
        }
    }
}

void AudioFormatReader::readMaxLevels (int64 startSampleInFile,
                                       int64 numSamples,
                                       float& lowestLeft, float& highestLeft,
                                       float& lowestRight, float& highestRight)
{
    if (numSamples <= 0)
    {
        lowestLeft = 0;
        lowestRight = 0;
        highestLeft = 0;
        highestRight = 0;
        return;
    }

    const int bufferSize = (int) jmin (numSamples, (int64) 4096);
    HeapBlock<int> tempSpace ((size_t) bufferSize * 2 + 64);

    int* tempBuffer[3];
    tempBuffer[0] = tempSpace.getData();
    tempBuffer[1] = tempSpace.getData() + bufferSize;
    tempBuffer[2] = 0;

    if (usesFloatingPointData)
    {
        float lmin = 1.0e6f;
        float lmax = -lmin;
        float rmin = lmin;
        float rmax = lmax;

        while (numSamples > 0)
        {
            const int numToDo = (int) jmin (numSamples, (int64) bufferSize);
            read (tempBuffer, 2, startSampleInFile, numToDo, false);

            numSamples -= numToDo;
            startSampleInFile += numToDo;

            float bufMin, bufMax;
            findMinAndMax (reinterpret_cast<float*> (tempBuffer[0]), numToDo, bufMin, bufMax);
            lmin = jmin (lmin, bufMin);
            lmax = jmax (lmax, bufMax);

            if (numChannels > 1)
            {
                findMinAndMax (reinterpret_cast<float*> (tempBuffer[1]), numToDo, bufMin, bufMax);
                rmin = jmin (rmin, bufMin);
                rmax = jmax (rmax, bufMax);
            }
        }

        if (numChannels <= 1)
        {
            rmax = lmax;
            rmin = lmin;
        }

        lowestLeft = lmin;
        highestLeft = lmax;
        lowestRight = rmin;
        highestRight = rmax;
    }
    else
    {
        int lmax = std::numeric_limits<int>::min();
        int lmin = std::numeric_limits<int>::max();
        int rmax = std::numeric_limits<int>::min();
        int rmin = std::numeric_limits<int>::max();

        while (numSamples > 0)
        {
            const int numToDo = (int) jmin (numSamples, (int64) bufferSize);
            if (! read (tempBuffer, 2, startSampleInFile, numToDo, false))
                break;

            numSamples -= numToDo;
            startSampleInFile += numToDo;

            for (int j = (int) numChannels; --j >= 0;)
            {
                int bufMin, bufMax;
                findMinAndMax (tempBuffer[j], numToDo, bufMin, bufMax);

                if (j == 0)
                {
                    lmax = jmax (lmax, bufMax);
                    lmin = jmin (lmin, bufMin);
                }
                else
                {
                    rmax = jmax (rmax, bufMax);
                    rmin = jmin (rmin, bufMin);
                }
            }
        }

        if (numChannels <= 1)
        {
            rmax = lmax;
            rmin = lmin;
        }

        lowestLeft = lmin / (float) std::numeric_limits<int>::max();
        highestLeft = lmax / (float) std::numeric_limits<int>::max();
        lowestRight = rmin / (float) std::numeric_limits<int>::max();
        highestRight = rmax / (float) std::numeric_limits<int>::max();
    }
}

int64 AudioFormatReader::searchForLevel (int64 startSample,
                                         int64 numSamplesToSearch,
                                         const double magnitudeRangeMinimum,
                                         const double magnitudeRangeMaximum,
                                         const int minimumConsecutiveSamples)
{
    if (numSamplesToSearch == 0)
        return -1;

    const int bufferSize = 4096;
    HeapBlock<int> tempSpace (bufferSize * 2 + 64);

    int* tempBuffer[3];
    tempBuffer[0] = tempSpace.getData();
    tempBuffer[1] = tempSpace.getData() + bufferSize;
    tempBuffer[2] = 0;

    int consecutive = 0;
    int64 firstMatchPos = -1;

    jassert (magnitudeRangeMaximum > magnitudeRangeMinimum);

    const double doubleMin = jlimit (0.0, (double) std::numeric_limits<int>::max(), magnitudeRangeMinimum * std::numeric_limits<int>::max());
    const double doubleMax = jlimit (doubleMin, (double) std::numeric_limits<int>::max(), magnitudeRangeMaximum * std::numeric_limits<int>::max());
    const int intMagnitudeRangeMinimum = roundToInt (doubleMin);
    const int intMagnitudeRangeMaximum = roundToInt (doubleMax);

    while (numSamplesToSearch != 0)
    {
        const int numThisTime = (int) jmin (abs64 (numSamplesToSearch), (int64) bufferSize);
        int64 bufferStart = startSample;

        if (numSamplesToSearch < 0)
            bufferStart -= numThisTime;

        if (bufferStart >= (int) lengthInSamples)
            break;

        read (tempBuffer, 2, bufferStart, numThisTime, false);

        int num = numThisTime;
        while (--num >= 0)
        {
            if (numSamplesToSearch < 0)
                --startSample;

            bool matches = false;
            const int index = (int) (startSample - bufferStart);

            if (usesFloatingPointData)
            {
                const float sample1 = std::abs (((float*) tempBuffer[0]) [index]);

                if (sample1 >= magnitudeRangeMinimum
                     && sample1 <= magnitudeRangeMaximum)
                {
                    matches = true;
                }
                else if (numChannels > 1)
                {
                    const float sample2 = std::abs (((float*) tempBuffer[1]) [index]);

                    matches = (sample2 >= magnitudeRangeMinimum
                                 && sample2 <= magnitudeRangeMaximum);
                }
            }
            else
            {
                const int sample1 = abs (tempBuffer[0] [index]);

                if (sample1 >= intMagnitudeRangeMinimum
                     && sample1 <= intMagnitudeRangeMaximum)
                {
                    matches = true;
                }
                else if (numChannels > 1)
                {
                    const int sample2 = abs (tempBuffer[1][index]);

                    matches = (sample2 >= intMagnitudeRangeMinimum
                                 && sample2 <= intMagnitudeRangeMaximum);
                }
            }

            if (matches)
            {
                if (firstMatchPos < 0)
                    firstMatchPos = startSample;

                if (++consecutive >= minimumConsecutiveSamples)
                {
                    if (firstMatchPos < 0 || firstMatchPos >= lengthInSamples)
                        return -1;

                    return firstMatchPos;
                }
            }
            else
            {
                consecutive = 0;
                firstMatchPos = -1;
            }

            if (numSamplesToSearch > 0)
                ++startSample;
        }

        if (numSamplesToSearch > 0)
            numSamplesToSearch -= numThisTime;
        else
            numSamplesToSearch += numThisTime;
    }

    return -1;
}


END_JUCE_NAMESPACE
