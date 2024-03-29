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

#ifndef __JUCE_SHA256_JUCEHEADER__
#define __JUCE_SHA256_JUCEHEADER__


//==============================================================================
/**
    SHA-256 secure hash generator.

    Create one of these objects from a block of source data or a stream, and it
    calculates the SHA-256 hash of that data.

    You can retrieve the hash as a raw 32-byte block, or as a 64-digit hex string.
    @see MD5
*/
class JUCE_API  SHA256
{
public:
    //==============================================================================
    /** Creates an empty SHA256 object.
        The default constructor just creates a hash filled with zeros. (This is not
        equal to the hash of an empty block of data).
    */
    SHA256() noexcept;

    /** Destructor. */
    ~SHA256() noexcept;

    /** Creates a copy of another SHA256. */
    SHA256 (const SHA256& other) noexcept;

    /** Copies another SHA256. */
    SHA256& operator= (const SHA256& other) noexcept;

    //==============================================================================
    /** Creates a hash from a block of raw data. */
    explicit SHA256 (const MemoryBlock& data);

    /** Creates a hash from a block of raw data. */
    SHA256 (const void* data, size_t numBytes);

    /** Creates a hash from the contents of a stream.

        This will read from the stream until the stream is exhausted, or until
        maxBytesToRead bytes have been read. If maxBytesToRead is negative, the entire
        stream will be read.
    */
    SHA256 (InputStream& input, int64 maxBytesToRead = -1);

    /** Reads a file and generates the hash of its contents.
        If the file can't be opened, the hash will be left uninitialised (i.e. full
        of zeros).
    */
    explicit SHA256 (const File& file);

    //==============================================================================
    /** Returns the hash as a 32-byte block of data. */
    MemoryBlock getRawData() const;

    /** Returns the checksum as a 64-digit hex string. */
    String toHexString() const;

    //==============================================================================
    bool operator== (const SHA256&) const noexcept;
    bool operator!= (const SHA256&) const noexcept;


private:
    //==============================================================================
    uint8 result [32];

    JUCE_LEAK_DETECTOR (SHA256);
};


#endif   // __JUCE_SHA256_JUCEHEADER__
