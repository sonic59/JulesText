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

#if JUCE_USE_OGGVORBIS

#if JUCE_MAC && ! defined (__MACOSX__)
 #define __MACOSX__ 1
#endif

BEGIN_JUCE_NAMESPACE

namespace OggVorbisNamespace
{
#if JUCE_INCLUDE_OGGVORBIS_CODE || ! defined (JUCE_INCLUDE_OGGVORBIS_CODE)
 #if JUCE_MSVC
  #pragma warning (push)
  #pragma warning (disable: 4267 4127 4244 4996 4100 4701 4702 4013 4133 4206 4305 4189 4706 4995 4365)
 #endif

 #include "oggvorbis/vorbisenc.h"
 #include "oggvorbis/codec.h"
 #include "oggvorbis/vorbisfile.h"

 #include "oggvorbis/bitwise.c"
 #include "oggvorbis/framing.c"
 #include "oggvorbis/libvorbis-1.3.2/lib/analysis.c"
 #include "oggvorbis/libvorbis-1.3.2/lib/bitrate.c"
 #include "oggvorbis/libvorbis-1.3.2/lib/block.c"
 #include "oggvorbis/libvorbis-1.3.2/lib/codebook.c"
 #include "oggvorbis/libvorbis-1.3.2/lib/envelope.c"
 #include "oggvorbis/libvorbis-1.3.2/lib/floor0.c"
 #include "oggvorbis/libvorbis-1.3.2/lib/floor1.c"
 #include "oggvorbis/libvorbis-1.3.2/lib/info.c"
 #include "oggvorbis/libvorbis-1.3.2/lib/lpc.c"
 #include "oggvorbis/libvorbis-1.3.2/lib/lsp.c"
 #include "oggvorbis/libvorbis-1.3.2/lib/mapping0.c"
 #include "oggvorbis/libvorbis-1.3.2/lib/mdct.c"
 #include "oggvorbis/libvorbis-1.3.2/lib/psy.c"
 #include "oggvorbis/libvorbis-1.3.2/lib/registry.c"
 #include "oggvorbis/libvorbis-1.3.2/lib/res0.c"
 #include "oggvorbis/libvorbis-1.3.2/lib/sharedbook.c"
 #include "oggvorbis/libvorbis-1.3.2/lib/smallft.c"
 #include "oggvorbis/libvorbis-1.3.2/lib/synthesis.c"
 #include "oggvorbis/libvorbis-1.3.2/lib/vorbisenc.c"
 #include "oggvorbis/libvorbis-1.3.2/lib/vorbisfile.c"
 #include "oggvorbis/libvorbis-1.3.2/lib/window.c"

 #if JUCE_MSVC
  #pragma warning (pop)
 #endif
#else
 #include <vorbis/vorbisenc.h>
 #include <vorbis/codec.h>
 #include <vorbis/vorbisfile.h>
#endif
}

#undef max
#undef min

//==============================================================================
static const char* const oggFormatName = "Ogg-Vorbis file";
static const char* const oggExtensions[] = { ".ogg", 0 };

//==============================================================================
class OggReader : public AudioFormatReader
{
public:
    OggReader (InputStream* const inp)
        : AudioFormatReader (inp, TRANS (oggFormatName)),
          reservoir (2, 4096),
          reservoirStart (0),
          samplesInReservoir (0)
    {
        using namespace OggVorbisNamespace;
        sampleRate = 0;
        usesFloatingPointData = true;

        callbacks.read_func = &oggReadCallback;
        callbacks.seek_func = &oggSeekCallback;
        callbacks.close_func = &oggCloseCallback;
        callbacks.tell_func = &oggTellCallback;

        const int err = ov_open_callbacks (input, &ovFile, 0, 0, callbacks);

        if (err == 0)
        {
            vorbis_info* info = ov_info (&ovFile, -1);
            lengthInSamples = (uint32) ov_pcm_total (&ovFile, -1);
            numChannels = (unsigned int) info->channels;
            bitsPerSample = 16;
            sampleRate = info->rate;

            reservoir.setSize ((int) numChannels,
                               (int) jmin (lengthInSamples, (int64) reservoir.getNumSamples()));
        }
    }

    ~OggReader()
    {
        OggVorbisNamespace::ov_clear (&ovFile);
    }

    //==============================================================================
    bool readSamples (int** destSamples, int numDestChannels, int startOffsetInDestBuffer,
                      int64 startSampleInFile, int numSamples)
    {
        while (numSamples > 0)
        {
            const int numAvailable = reservoirStart + samplesInReservoir - startSampleInFile;

            if (startSampleInFile >= reservoirStart && numAvailable > 0)
            {
                // got a few samples overlapping, so use them before seeking..

                const int numToUse = jmin (numSamples, numAvailable);

                for (int i = jmin (numDestChannels, reservoir.getNumChannels()); --i >= 0;)
                    if (destSamples[i] != nullptr)
                        memcpy (destSamples[i] + startOffsetInDestBuffer,
                                reservoir.getSampleData (i, (int) (startSampleInFile - reservoirStart)),
                                sizeof (float) * numToUse);

                startSampleInFile += numToUse;
                numSamples -= numToUse;
                startOffsetInDestBuffer += numToUse;

                if (numSamples == 0)
                    break;
            }

            if (startSampleInFile < reservoirStart
                || startSampleInFile + numSamples > reservoirStart + samplesInReservoir)
            {
                // buffer miss, so refill the reservoir
                int bitStream = 0;

                reservoirStart = jmax (0, (int) startSampleInFile);
                samplesInReservoir = reservoir.getNumSamples();

                if (reservoirStart != (int) OggVorbisNamespace::ov_pcm_tell (&ovFile))
                    OggVorbisNamespace::ov_pcm_seek (&ovFile, reservoirStart);

                int offset = 0;
                int numToRead = samplesInReservoir;

                while (numToRead > 0)
                {
                    float** dataIn = nullptr;

                    const int samps = OggVorbisNamespace::ov_read_float (&ovFile, &dataIn, numToRead, &bitStream);
                    if (samps <= 0)
                        break;

                    jassert (samps <= numToRead);

                    for (int i = jmin ((int) numChannels, reservoir.getNumChannels()); --i >= 0;)
                    {
                        memcpy (reservoir.getSampleData (i, offset),
                                dataIn[i],
                                sizeof (float) * samps);
                    }

                    numToRead -= samps;
                    offset += samps;
                }

                if (numToRead > 0)
                    reservoir.clear (offset, numToRead);
            }
        }

        if (numSamples > 0)
        {
            for (int i = numDestChannels; --i >= 0;)
                if (destSamples[i] != nullptr)
                    zeromem (destSamples[i] + startOffsetInDestBuffer, sizeof (int) * numSamples);
        }

        return true;
    }

    //==============================================================================
    static size_t oggReadCallback (void* ptr, size_t size, size_t nmemb, void* datasource)
    {
        return (size_t) (static_cast <InputStream*> (datasource)->read (ptr, (int) (size * nmemb)) / size);
    }

    static int oggSeekCallback (void* datasource, OggVorbisNamespace::ogg_int64_t offset, int whence)
    {
        InputStream* const in = static_cast <InputStream*> (datasource);

        if (whence == SEEK_CUR)
            offset += in->getPosition();
        else if (whence == SEEK_END)
            offset += in->getTotalLength();

        in->setPosition (offset);
        return 0;
    }

    static int oggCloseCallback (void*)
    {
        return 0;
    }

    static long oggTellCallback (void* datasource)
    {
        return (long) static_cast <InputStream*> (datasource)->getPosition();
    }

private:
    OggVorbisNamespace::OggVorbis_File ovFile;
    OggVorbisNamespace::ov_callbacks callbacks;
    AudioSampleBuffer reservoir;
    int reservoirStart, samplesInReservoir;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OggReader);
};

//==============================================================================
class OggWriter  : public AudioFormatWriter
{
public:
    OggWriter (OutputStream* const out,
               const double sampleRate_,
               const unsigned int numChannels_,
               const unsigned int bitsPerSample_,
               const int qualityIndex,
               const StringPairArray& metadataValues)
        : AudioFormatWriter (out, TRANS (oggFormatName), sampleRate_, numChannels_, bitsPerSample_),
          ok (false)
    {
        using namespace OggVorbisNamespace;

        vorbis_info_init (&vi);

        if (vorbis_encode_init_vbr (&vi, (int) numChannels_, (int) sampleRate_,
                                    jlimit (0.0f, 1.0f, qualityIndex * 0.1f)) == 0)
        {
            vorbis_comment_init (&vc);

            const String encoder (metadataValues [OggVorbisAudioFormat::encoderName]);
            if (encoder.isNotEmpty())
                vorbis_comment_add_tag (&vc, "ENCODER", const_cast <char*> (encoder.toUTF8().getAddress()));

            vorbis_analysis_init (&vd, &vi);
            vorbis_block_init (&vd, &vb);

            ogg_stream_init (&os, Random::getSystemRandom().nextInt());

            ogg_packet header;
            ogg_packet header_comm;
            ogg_packet header_code;

            vorbis_analysis_headerout (&vd, &vc, &header, &header_comm, &header_code);

            ogg_stream_packetin (&os, &header);
            ogg_stream_packetin (&os, &header_comm);
            ogg_stream_packetin (&os, &header_code);

            for (;;)
            {
                if (ogg_stream_flush (&os, &og) == 0)
                    break;

                output->write (og.header, og.header_len);
                output->write (og.body, og.body_len);
            }

            ok = true;
        }
    }

    ~OggWriter()
    {
        using namespace OggVorbisNamespace;
        if (ok)
        {
            // write a zero-length packet to show ogg that we're finished..
            writeSamples (0);

            ogg_stream_clear (&os);
            vorbis_block_clear (&vb);
            vorbis_dsp_clear (&vd);
            vorbis_comment_clear (&vc);

            vorbis_info_clear (&vi);
            output->flush();
        }
        else
        {
            vorbis_info_clear (&vi);
            output = nullptr; // to stop the base class deleting this, as it needs to be returned
                              // to the caller of createWriter()
        }
    }

    //==============================================================================
    bool write (const int** samplesToWrite, int numSamples)
    {
        if (ok)
        {
            using namespace OggVorbisNamespace;

            if (numSamples > 0)
            {
                const double gain = 1.0 / 0x80000000u;
                float** const vorbisBuffer = vorbis_analysis_buffer (&vd, numSamples);

                for (int i = (int) numChannels; --i >= 0;)
                {
                    float* const dst = vorbisBuffer[i];
                    const int* const src = samplesToWrite [i];

                    if (src != nullptr && dst != nullptr)
                    {
                        for (int j = 0; j < numSamples; ++j)
                            dst[j] = (float) (src[j] * gain);
                    }
                }
            }

            writeSamples (numSamples);
        }

        return ok;
    }

    void writeSamples (int numSamples)
    {
        using namespace OggVorbisNamespace;

        vorbis_analysis_wrote (&vd, numSamples);

        while (vorbis_analysis_blockout (&vd, &vb) == 1)
        {
            vorbis_analysis (&vb, 0);
            vorbis_bitrate_addblock (&vb);

            while (vorbis_bitrate_flushpacket (&vd, &op))
            {
                ogg_stream_packetin (&os, &op);

                for (;;)
                {
                    if (ogg_stream_pageout (&os, &og) == 0)
                        break;

                    output->write (og.header, og.header_len);
                    output->write (og.body, og.body_len);

                    if (ogg_page_eos (&og))
                        break;
                }
            }
        }
    }

    bool ok;

private:
    OggVorbisNamespace::ogg_stream_state os;
    OggVorbisNamespace::ogg_page og;
    OggVorbisNamespace::ogg_packet op;
    OggVorbisNamespace::vorbis_info vi;
    OggVorbisNamespace::vorbis_comment vc;
    OggVorbisNamespace::vorbis_dsp_state vd;
    OggVorbisNamespace::vorbis_block vb;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OggWriter);
};


//==============================================================================
OggVorbisAudioFormat::OggVorbisAudioFormat()
    : AudioFormat (TRANS (oggFormatName), StringArray (oggExtensions))
{
}

OggVorbisAudioFormat::~OggVorbisAudioFormat()
{
}

Array<int> OggVorbisAudioFormat::getPossibleSampleRates()
{
    const int rates[] = { 22050, 32000, 44100, 48000, 88200, 96000, 176400, 192000, 0 };
    return Array <int> (rates);
}

Array<int> OggVorbisAudioFormat::getPossibleBitDepths()
{
    const int depths[] = { 32, 0 };
    return Array <int> (depths);
}

bool OggVorbisAudioFormat::canDoStereo()    { return true; }
bool OggVorbisAudioFormat::canDoMono()      { return true; }
bool OggVorbisAudioFormat::isCompressed()   { return true; }

const char* const OggVorbisAudioFormat::encoderName = "encoder";

AudioFormatReader* OggVorbisAudioFormat::createReaderFor (InputStream* in, const bool deleteStreamIfOpeningFails)
{
    ScopedPointer<OggReader> r (new OggReader (in));

    if (r->sampleRate > 0)
        return r.release();

    if (! deleteStreamIfOpeningFails)
        r->input = nullptr;

    return nullptr;
}

AudioFormatWriter* OggVorbisAudioFormat::createWriterFor (OutputStream* out,
                                                          double sampleRate,
                                                          unsigned int numChannels,
                                                          int bitsPerSample,
                                                          const StringPairArray& metadataValues,
                                                          int qualityOptionIndex)
{
    ScopedPointer <OggWriter> w (new OggWriter (out, sampleRate, numChannels,
                                                (unsigned int) bitsPerSample, qualityOptionIndex, metadataValues));

    return w->ok ? w.release() : nullptr;
}

StringArray OggVorbisAudioFormat::getQualityOptions()
{
    const char* options[] = { "64 kbps", "80 kbps", "96 kbps", "112 kbps", "128 kbps", "160 kbps",
                              "192 kbps", "224 kbps", "256 kbps", "320 kbps", "500 kbps", 0 };
    return StringArray (options);
}

int OggVorbisAudioFormat::estimateOggFileQuality (const File& source)
{
    FileInputStream* const in = source.createInputStream();

    if (in != nullptr)
    {
        ScopedPointer <AudioFormatReader> r (createReaderFor (in, true));

        if (r != nullptr)
        {
            const int64 numSamps = r->lengthInSamples;
            r = nullptr;

            const int64 fileNumSamps = source.getSize() / 4;
            const double ratio = numSamps / (double) fileNumSamps;

            if (ratio > 12.0)
                return 0;
            else if (ratio > 6.0)
                return 1;
            else
                return 2;
        }
    }

    return 1;
}

END_JUCE_NAMESPACE

#endif
