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
struct AudioThumbnail::MinMaxValue
{
    MinMaxValue() noexcept
    {
        values[0] = 0;
        values[1] = 0;
    }

    inline void set (const char newMin, const char newMax) noexcept
    {
        values[0] = newMin;
        values[1] = newMax;
    }

    inline char getMinValue() const noexcept        { return values[0]; }
    inline char getMaxValue() const noexcept        { return values[1]; }

    inline void setFloat (const float newMin, const float newMax) noexcept
    {
        values[0] = (char) jlimit (-128, 127, roundFloatToInt (newMin * 127.0f));
        values[1] = (char) jlimit (-128, 127, roundFloatToInt (newMax * 127.0f));

        if (values[0] == values[1])
        {
            if (values[1] == 127)
                values[0]--;
            else
                values[1]++;
        }
    }

    inline bool isNonZero() const noexcept
    {
        return values[1] > values[0];
    }

    inline int getPeak() const noexcept
    {
        return jmax (std::abs ((int) values[0]),
                     std::abs ((int) values[1]));
    }

    inline void read (InputStream& input)      { input.read (values, 2); }
    inline void write (OutputStream& output)   { output.write (values, 2); }

private:
    char values[2];
};

//==============================================================================
class AudioThumbnail::LevelDataSource   : public TimeSliceClient
{
public:
    LevelDataSource (AudioThumbnail& owner_, AudioFormatReader* newReader, int64 hash)
        : lengthInSamples (0), numSamplesFinished (0), sampleRate (0), numChannels (0),
          hashCode (hash), owner (owner_), reader (newReader)
    {
    }

    LevelDataSource (AudioThumbnail& owner_, InputSource* source_)
        : lengthInSamples (0), numSamplesFinished (0), sampleRate (0), numChannels (0),
          hashCode (source_->hashCode()), owner (owner_), source (source_)
    {
    }

    ~LevelDataSource()
    {
        owner.cache.removeTimeSliceClient (this);
    }

    enum { timeBeforeDeletingReader = 1000 };

    void initialise (int64 numSamplesFinished_)
    {
        const ScopedLock sl (readerLock);

        numSamplesFinished = numSamplesFinished_;

        createReader();

        if (reader != nullptr)
        {
            lengthInSamples = reader->lengthInSamples;
            numChannels = reader->numChannels;
            sampleRate = reader->sampleRate;

            if (lengthInSamples <= 0 || isFullyLoaded())
                reader = nullptr;
            else
                owner.cache.addTimeSliceClient (this);
        }
    }

    void getLevels (int64 startSample, int numSamples, Array<float>& levels)
    {
        const ScopedLock sl (readerLock);

        if (reader == nullptr)
        {
            createReader();

            if (reader != nullptr)
                owner.cache.addTimeSliceClient (this);
        }

        if (reader != nullptr)
        {
            float l[4] = { 0 };
            reader->readMaxLevels (startSample, numSamples, l[0], l[1], l[2], l[3]);

            levels.clearQuick();
            levels.addArray ((const float*) l, 4);
        }
    }

    void releaseResources()
    {
        const ScopedLock sl (readerLock);
        reader = nullptr;
    }

    int useTimeSlice()
    {
        if (isFullyLoaded())
        {
            if (reader != nullptr && source != nullptr)
                releaseResources();

            return -1;
        }

        bool justFinished = false;

        {
            const ScopedLock sl (readerLock);

            createReader();

            if (reader != nullptr)
            {
                if (! readNextBlock())
                    return 0;

                justFinished = true;
            }
        }

        if (justFinished)
            owner.cache.storeThumb (owner, hashCode);

        return timeBeforeDeletingReader;
    }

    bool isFullyLoaded() const noexcept
    {
        return numSamplesFinished >= lengthInSamples;
    }

    inline int sampleToThumbSample (const int64 originalSample) const noexcept
    {
        return (int) (originalSample / owner.samplesPerThumbSample);
    }

    int64 lengthInSamples, numSamplesFinished;
    double sampleRate;
    unsigned int numChannels;
    int64 hashCode;

private:
    AudioThumbnail& owner;
    ScopedPointer <InputSource> source;
    ScopedPointer <AudioFormatReader> reader;
    CriticalSection readerLock;

    void createReader()
    {
        if (reader == nullptr && source != nullptr)
        {
            InputStream* audioFileStream = source->createInputStream();

            if (audioFileStream != nullptr)
                reader = owner.formatManagerToUse.createReaderFor (audioFileStream);
        }
    }

    bool readNextBlock()
    {
        jassert (reader != nullptr);

        if (! isFullyLoaded())
        {
            const int numToDo = (int) jmin (256 * (int64) owner.samplesPerThumbSample, lengthInSamples - numSamplesFinished);

            if (numToDo > 0)
            {
                int64 startSample = numSamplesFinished;

                const int firstThumbIndex = sampleToThumbSample (startSample);
                const int lastThumbIndex  = sampleToThumbSample (startSample + numToDo);
                const int numThumbSamps = lastThumbIndex - firstThumbIndex;

                HeapBlock<MinMaxValue> levelData ((size_t) numThumbSamps * 2);
                MinMaxValue* levels[2] = { levelData, levelData + numThumbSamps };

                for (int i = 0; i < numThumbSamps; ++i)
                {
                    float lowestLeft, highestLeft, lowestRight, highestRight;

                    reader->readMaxLevels ((firstThumbIndex + i) * owner.samplesPerThumbSample, owner.samplesPerThumbSample,
                                           lowestLeft, highestLeft, lowestRight, highestRight);

                    levels[0][i].setFloat (lowestLeft, highestLeft);
                    levels[1][i].setFloat (lowestRight, highestRight);
                }

                {
                    const ScopedUnlock su (readerLock);
                    owner.setLevels (levels, firstThumbIndex, 2, numThumbSamps);
                }

                numSamplesFinished += numToDo;
            }
        }

        return isFullyLoaded();
    }
};

//==============================================================================
class AudioThumbnail::ThumbData
{
public:
    ThumbData (const int numThumbSamples)
        : peakLevel (-1)
    {
        ensureSize (numThumbSamples);
    }

    inline MinMaxValue* getData (const int thumbSampleIndex) noexcept
    {
        jassert (thumbSampleIndex < data.size());
        return data.getRawDataPointer() + thumbSampleIndex;
    }

    int getSize() const noexcept
    {
        return data.size();
    }

    void getMinMax (int startSample, int endSample, MinMaxValue& result) const noexcept
    {
        if (startSample >= 0)
        {
            endSample = jmin (endSample, data.size() - 1);

            char mx = -128;
            char mn = 127;

            while (startSample <= endSample)
            {
                const MinMaxValue& v = data.getReference (startSample);

                if (v.getMinValue() < mn)  mn = v.getMinValue();
                if (v.getMaxValue() > mx)  mx = v.getMaxValue();

                ++startSample;
            }

            if (mn <= mx)
            {
                result.set (mn, mx);
                return;
            }
        }

        result.set (1, 0);
    }

    void write (const MinMaxValue* const source, const int startIndex, const int numValues)
    {
        resetPeak();

        if (startIndex + numValues > data.size())
            ensureSize (startIndex + numValues);

        MinMaxValue* const dest = getData (startIndex);

        for (int i = 0; i < numValues; ++i)
            dest[i] = source[i];
    }

    void resetPeak() noexcept
    {
        peakLevel = -1;
    }

    int getPeak() noexcept
    {
        if (peakLevel < 0)
        {
            for (int i = 0; i < data.size(); ++i)
            {
                const int peak = data[i].getPeak();
                if (peak > peakLevel)
                    peakLevel = peak;
            }
        }

        return peakLevel;
    }

private:
    Array <MinMaxValue> data;
    int peakLevel;

    void ensureSize (const int thumbSamples)
    {
        const int extraNeeded = thumbSamples - data.size();
        if (extraNeeded > 0)
            data.insertMultiple (-1, MinMaxValue(), extraNeeded);
    }
};

//==============================================================================
class AudioThumbnail::CachedWindow
{
public:
    CachedWindow()
        : cachedStart (0), cachedTimePerPixel (0),
          numChannelsCached (0), numSamplesCached (0),
          cacheNeedsRefilling (true)
    {
    }

    void invalidate()
    {
        cacheNeedsRefilling = true;
    }

    void drawChannel (Graphics& g, const Rectangle<int>& area,
                      const double startTime, const double endTime,
                      const int channelNum, const float verticalZoomFactor,
                      const double sampleRate, const int numChannels, const int samplesPerThumbSample,
                      LevelDataSource* levelData, const OwnedArray<ThumbData>& channels)
    {
        refillCache (area.getWidth(), startTime, endTime, sampleRate,
                     numChannels, samplesPerThumbSample, levelData, channels);

        if (isPositiveAndBelow (channelNum, numChannelsCached))
        {
            const Rectangle<int> clip (g.getClipBounds().getIntersection (area.withWidth (jmin (numSamplesCached, area.getWidth()))));

            if (! clip.isEmpty())
            {
                const float topY = (float) area.getY();
                const float bottomY = (float) area.getBottom();
                const float midY = (topY + bottomY) * 0.5f;
                const float vscale = verticalZoomFactor * (bottomY - topY) / 256.0f;

                const MinMaxValue* cacheData = getData (channelNum, clip.getX() - area.getX());

                int x = clip.getX();
                for (int w = clip.getWidth(); --w >= 0;)
                {
                    if (cacheData->isNonZero())
                        g.drawVerticalLine (x, jmax (midY - cacheData->getMaxValue() * vscale - 0.3f, topY),
                                               jmin (midY - cacheData->getMinValue() * vscale + 0.3f, bottomY));

                    ++x;
                    ++cacheData;
                }
            }
        }
    }

private:
    Array <MinMaxValue> data;
    double cachedStart, cachedTimePerPixel;
    int numChannelsCached, numSamplesCached;
    bool cacheNeedsRefilling;

    void refillCache (const int numSamples, double startTime, const double endTime,
                      const double sampleRate, const int numChannels, const int samplesPerThumbSample,
                      LevelDataSource* levelData, const OwnedArray<ThumbData>& channels)
    {
        const double timePerPixel = (endTime - startTime) / numSamples;

        if (numSamples <= 0 || timePerPixel <= 0.0 || sampleRate <= 0)
        {
            invalidate();
            return;
        }

        if (numSamples == numSamplesCached
             && numChannelsCached == numChannels
             && startTime == cachedStart
             && timePerPixel == cachedTimePerPixel
             && ! cacheNeedsRefilling)
        {
            return;
        }

        numSamplesCached = numSamples;
        numChannelsCached = numChannels;
        cachedStart = startTime;
        cachedTimePerPixel = timePerPixel;
        cacheNeedsRefilling = false;

        ensureSize (numSamples);

        if (timePerPixel * sampleRate <= samplesPerThumbSample && levelData != nullptr)
        {
            int sample = roundToInt (startTime * sampleRate);
            Array<float> levels;

            int i;
            for (i = 0; i < numSamples; ++i)
            {
                const int nextSample = roundToInt ((startTime + timePerPixel) * sampleRate);

                if (sample >= 0)
                {
                    if (sample >= levelData->lengthInSamples)
                        break;

                    levelData->getLevels (sample, jmax (1, nextSample - sample), levels);

                    const int numChans = jmin (levels.size() / 2, numChannelsCached);

                    for (int chan = 0; chan < numChans; ++chan)
                        getData (chan, i)->setFloat (levels.getUnchecked (chan * 2),
                                                     levels.getUnchecked (chan * 2 + 1));
                }

                startTime += timePerPixel;
                sample = nextSample;
            }

            numSamplesCached = i;
        }
        else
        {
            jassert (channels.size() == numChannelsCached);

            for (int channelNum = 0; channelNum < numChannelsCached; ++channelNum)
            {
                ThumbData* channelData = channels.getUnchecked (channelNum);
                MinMaxValue* cacheData = getData (channelNum, 0);

                const double timeToThumbSampleFactor = sampleRate / (double) samplesPerThumbSample;

                startTime = cachedStart;
                int sample = roundToInt (startTime * timeToThumbSampleFactor);

                for (int i = numSamples; --i >= 0;)
                {
                    const int nextSample = roundToInt ((startTime + timePerPixel) * timeToThumbSampleFactor);

                    channelData->getMinMax (sample, nextSample, *cacheData);

                    ++cacheData;
                    startTime += timePerPixel;
                    sample = nextSample;
                }
            }
        }
    }

    MinMaxValue* getData (const int channelNum, const int cacheIndex) noexcept
    {
        jassert (isPositiveAndBelow (channelNum, numChannelsCached) && isPositiveAndBelow (cacheIndex, data.size()));

        return data.getRawDataPointer() + channelNum * numSamplesCached
                                        + cacheIndex;
    }

    void ensureSize (const int numSamples)
    {
        const int itemsRequired = numSamples * numChannelsCached;

        if (data.size() < itemsRequired)
            data.insertMultiple (-1, MinMaxValue(), itemsRequired - data.size());
    }
};

//==============================================================================
AudioThumbnail::AudioThumbnail (const int originalSamplesPerThumbnailSample,
                                AudioFormatManager& formatManagerToUse_,
                                AudioThumbnailCache& cacheToUse)
    : formatManagerToUse (formatManagerToUse_),
      cache (cacheToUse),
      window (new CachedWindow()),
      samplesPerThumbSample (originalSamplesPerThumbnailSample),
      totalSamples (0),
      numChannels (0),
      sampleRate (0)
{
}

AudioThumbnail::~AudioThumbnail()
{
    clear();
}

void AudioThumbnail::clear()
{
    source = nullptr;
    clearChannelData();
}

void AudioThumbnail::clearChannelData()
{
    const ScopedLock sl (lock);
    window->invalidate();
    channels.clear();
    totalSamples = numSamplesFinished = 0;
    numChannels = 0;
    sampleRate = 0;

    sendChangeMessage();
}

void AudioThumbnail::reset (int newNumChannels, double newSampleRate, int64 totalSamplesInSource)
{
    clear();

    numChannels = newNumChannels;
    sampleRate = newSampleRate;
    totalSamples = totalSamplesInSource;

    createChannels (1 + (int) (totalSamplesInSource / samplesPerThumbSample));
}

void AudioThumbnail::createChannels (const int length)
{
    while (channels.size() < numChannels)
        channels.add (new ThumbData (length));
}

//==============================================================================
void AudioThumbnail::loadFrom (InputStream& rawInput)
{
    clearChannelData();

    BufferedInputStream input (rawInput, 4096);

    if (input.readByte() != 'j' || input.readByte() != 'a' || input.readByte() != 't' || input.readByte() != 'm')
        return;

    samplesPerThumbSample = input.readInt();
    totalSamples = input.readInt64();             // Total number of source samples.
    numSamplesFinished = input.readInt64();       // Number of valid source samples that have been read into the thumbnail.
    int32 numThumbnailSamples = input.readInt();  // Number of samples in the thumbnail data.
    numChannels = input.readInt();                // Number of audio channels.
    sampleRate = input.readInt();                 // Source sample rate.
    input.skipNextBytes (16);                     // (reserved)

    createChannels (numThumbnailSamples);

    for (int i = 0; i < numThumbnailSamples; ++i)
        for (int chan = 0; chan < numChannels; ++chan)
            channels.getUnchecked(chan)->getData(i)->read (input);
}

void AudioThumbnail::saveTo (OutputStream& output) const
{
    const ScopedLock sl (lock);

    const int numThumbnailSamples = channels.size() == 0 ? 0 : channels.getUnchecked(0)->getSize();

    output.write ("jatm", 4);
    output.writeInt (samplesPerThumbSample);
    output.writeInt64 (totalSamples);
    output.writeInt64 (numSamplesFinished);
    output.writeInt (numThumbnailSamples);
    output.writeInt (numChannels);
    output.writeInt ((int) sampleRate);
    output.writeInt64 (0);
    output.writeInt64 (0);

    for (int i = 0; i < numThumbnailSamples; ++i)
        for (int chan = 0; chan < numChannels; ++chan)
            channels.getUnchecked(chan)->getData(i)->write (output);
}

//==============================================================================
bool AudioThumbnail::setDataSource (LevelDataSource* newSource)
{
    jassert (MessageManager::getInstance()->currentThreadHasLockedMessageManager());

    numSamplesFinished = 0;

    if (cache.loadThumb (*this, newSource->hashCode) && isFullyLoaded())
    {
        source = newSource; // (make sure this isn't done before loadThumb is called)

        source->lengthInSamples = totalSamples;
        source->sampleRate = sampleRate;
        source->numChannels = (unsigned int) numChannels;
        source->numSamplesFinished = numSamplesFinished;
    }
    else
    {
        source = newSource; // (make sure this isn't done before loadThumb is called)

        const ScopedLock sl (lock);
        source->initialise (numSamplesFinished);

        totalSamples = source->lengthInSamples;
        sampleRate = source->sampleRate;
        numChannels = (int32) source->numChannels;

        createChannels (1 + (int) (totalSamples / samplesPerThumbSample));
    }

    return sampleRate > 0 && totalSamples > 0;
}

bool AudioThumbnail::setSource (InputSource* const newSource)
{
    clear();

    return newSource != nullptr && setDataSource (new LevelDataSource (*this, newSource));
}

void AudioThumbnail::setReader (AudioFormatReader* newReader, int64 hash)
{
    clear();

    if (newReader != nullptr)
        setDataSource (new LevelDataSource (*this, newReader, hash));
}

int64 AudioThumbnail::getHashCode() const
{
    return source == nullptr ? 0 : source->hashCode;
}

void AudioThumbnail::addBlock (const int64 startSample, const AudioSampleBuffer& incoming,
                               int startOffsetInBuffer, int numSamples)
{
    jassert (startSample >= 0);

    const int firstThumbIndex = (int) (startSample / samplesPerThumbSample);
    const int lastThumbIndex  = (int) ((startSample + numSamples + (samplesPerThumbSample - 1)) / samplesPerThumbSample);
    const int numToDo = lastThumbIndex - firstThumbIndex;

    if (numToDo > 0)
    {
        const int numChans = jmin (channels.size(), incoming.getNumChannels());

        const HeapBlock<MinMaxValue> thumbData ((size_t) (numToDo * numChans));
        const HeapBlock<MinMaxValue*> thumbChannels ((size_t) numChans);

        for (int chan = 0; chan < numChans; ++chan)
        {
            const float* const sourceData = incoming.getSampleData (chan, startOffsetInBuffer);
            MinMaxValue* const dest = thumbData + numToDo * chan;
            thumbChannels [chan] = dest;

            for (int i = 0; i < numToDo; ++i)
            {
                float low, high;
                const int start = i * samplesPerThumbSample;
                findMinAndMax (sourceData + start, jmin (samplesPerThumbSample, numSamples - start), low, high);
                dest[i].setFloat (low, high);
            }
        }

        setLevels (thumbChannels, firstThumbIndex, numChans, numToDo);
    }
}

void AudioThumbnail::setLevels (const MinMaxValue* const* values, int thumbIndex, int numChans, int numValues)
{
    const ScopedLock sl (lock);

    for (int i = jmin (numChans, channels.size()); --i >= 0;)
        channels.getUnchecked(i)->write (values[i], thumbIndex, numValues);

    const int64 start = thumbIndex * (int64) samplesPerThumbSample;
    const int64 end = (thumbIndex + numValues) * (int64) samplesPerThumbSample;

    if (numSamplesFinished >= start && end > numSamplesFinished)
        numSamplesFinished = end;

    totalSamples = jmax (numSamplesFinished, totalSamples);
    window->invalidate();
    sendChangeMessage();
}

//==============================================================================
int AudioThumbnail::getNumChannels() const noexcept
{
    return numChannels;
}

double AudioThumbnail::getTotalLength() const noexcept
{
    return sampleRate > 0 ? (totalSamples / sampleRate) : 0;
}

bool AudioThumbnail::isFullyLoaded() const noexcept
{
    return numSamplesFinished >= totalSamples - samplesPerThumbSample;
}

int64 AudioThumbnail::getNumSamplesFinished() const noexcept
{
    return numSamplesFinished;
}

float AudioThumbnail::getApproximatePeak() const
{
    int peak = 0;

    for (int i = channels.size(); --i >= 0;)
        peak = jmax (peak, channels.getUnchecked(i)->getPeak());

    return jlimit (0, 127, peak) / 127.0f;
}

void AudioThumbnail::getApproximateMinMax (const double startTime, const double endTime, const int channelIndex,
                                           float& minValue, float& maxValue) const noexcept
{
    MinMaxValue result;
    const ThumbData* const data = channels [channelIndex];

    if (data != nullptr && sampleRate > 0)
    {
        const int firstThumbIndex = (int) ((startTime * sampleRate) / samplesPerThumbSample);
        const int lastThumbIndex  = (int) (((endTime * sampleRate) + samplesPerThumbSample - 1) / samplesPerThumbSample);

        data->getMinMax (jmax (0, firstThumbIndex), lastThumbIndex, result);
    }

    minValue = result.getMinValue() / 128.0f;
    maxValue = result.getMaxValue() / 128.0f;
}

void AudioThumbnail::drawChannel (Graphics& g, const Rectangle<int>& area, double startTime,
                                  double endTime, int channelNum, float verticalZoomFactor)
{
    const ScopedLock sl (lock);

    window->drawChannel (g, area, startTime, endTime, channelNum, verticalZoomFactor,
                         sampleRate, numChannels, samplesPerThumbSample, source, channels);
}

void AudioThumbnail::drawChannels (Graphics& g, const Rectangle<int>& area, double startTimeSeconds,
                                   double endTimeSeconds, float verticalZoomFactor)
{
    for (int i = 0; i < numChannels; ++i)
    {
        const int y1 = roundToInt ((i * area.getHeight()) / numChannels);
        const int y2 = roundToInt (((i + 1) * area.getHeight()) / numChannels);

        drawChannel (g, Rectangle<int> (area.getX(), area.getY() + y1, area.getWidth(), y2 - y1),
                     startTimeSeconds, endTimeSeconds, i, verticalZoomFactor);
    }
}


END_JUCE_NAMESPACE
