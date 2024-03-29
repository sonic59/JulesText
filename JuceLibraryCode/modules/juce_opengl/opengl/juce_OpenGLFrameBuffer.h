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

#ifndef __JUCE_OPENGLFRAMEBUFFER_JUCEHEADER__
#define __JUCE_OPENGLFRAMEBUFFER_JUCEHEADER__


//==============================================================================
/**
    Creates an openGL frame buffer.
*/
class JUCE_API  OpenGLFrameBuffer   : public OpenGLRenderingTarget
{
public:
    /** Creates an uninitialised buffer.
        To actually allocate the buffer, use initialise().
    */
    OpenGLFrameBuffer();

    /** Destructor. */
    ~OpenGLFrameBuffer();

    //==============================================================================
    /** Tries to allocates a buffer of the given size.
        Note that a valid openGL context must be selected when you call this method,
        or it will fail.
    */
    bool initialise (int width, int height);

    /** Tries to allocates a buffer containing a copy of a given image.
        Note that a valid openGL context must be selected when you call this method,
        or it will fail.
    */
    bool initialise (const Image& content);

    /** Tries to allocate a copy of another framebuffer.
    */
    bool initialise (const OpenGLFrameBuffer& other);

    /** Releases the buffer, if one has been allocated.
        Any saved state that was created with saveAndRelease() will also be freed by this call.
    */
    void release();

    /** If the framebuffer is active, this will save a stashed copy of its contents in main memory,
        and will release the GL buffer.
        After saving, the original state can be restored again by calling reloadSavedCopy().
    */
    void saveAndRelease();

    /** Restores the framebuffer content that was previously saved using saveAndRelease().
        After saving to main memory, the original state can be restored by calling restoreToGPUMemory().
    */
    bool reloadSavedCopy();

    //==============================================================================
    /** Returns true if a valid buffer has been allocated. */
    bool isValid() const noexcept                       { return pimpl != nullptr; }

    /** Returns the width of the buffer. */
    int getWidth() const noexcept;

    /** Returns the height of the buffer. */
    int getHeight() const noexcept;

    int getRenderingTargetWidth() const         { return getWidth(); }
    int getRenderingTargetHeight() const        { return getHeight(); }

    /** Returns the texture ID number for using this buffer as a texture. */
    GLuint getTextureID() const noexcept;

    //==============================================================================
    /** Selects this buffer as the current OpenGL rendering target. */
    bool makeCurrentRenderingTarget();

    /** Deselects this buffer as the current OpenGL rendering target. */
    void releaseAsRenderingTarget();

    /** Selects a framebuffer as the active target, or deselects the current
        target buffer if you pass 0.
    */
    static void setCurrentFrameBufferTarget (GLuint frameBufferID);

    /** Returns the ID of the currently-bound framebuffer. */
    static GLuint getCurrentFrameBufferTarget();

    /** Clears the framebuffer with the specified colour. */
    void clear (const Colour& colour);

    /** Selects the framebuffer as the current target, and clears it to transparent. */
    void makeCurrentAndClear();

    /** Draws this framebuffer onto the current context, with the specified corner positions. */
    void draw2D (float x1, float y1,
                 float x2, float y2,
                 float x3, float y3,
                 float x4, float y4,
                 const Colour& colour) const;

    /** Draws this framebuffer onto the current context, with the specified corner positions. */
    void draw3D (float x1, float y1, float z1,
                 float x2, float y2, float z2,
                 float x3, float y3, float z3,
                 float x4, float y4, float z4,
                 const Colour& colour) const;

    /** Draws the framebuffer at a given position. */
    void drawAt (float x1, float y1) const;

    /** Reads an area of pixels from the framebuffer into a 32-bit ARGB pixel array.
        The lineStride is measured as a number of pixels, not bytes - pass a stride
        of 0 to indicate a packed array.
    */
    bool readPixels (PixelARGB* targetData, const Rectangle<int>& sourceArea);

    /** Writes an area of pixels into the framebuffer from a specified pixel array.
        The lineStride is measured as a number of pixels, not bytes - pass a stride
        of 0 to indicate a packed array.
    */
    bool writePixels (const PixelARGB* srcData, const Rectangle<int>& targetArea);

private:
    class Pimpl;
    friend class ScopedPointer<Pimpl>;
    ScopedPointer<Pimpl> pimpl;

    class SavedState;
    friend class ScopedPointer<SavedState>;
    ScopedPointer<SavedState> savedState;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenGLFrameBuffer);
};


#endif   // __JUCE_OPENGLFRAMEBUFFER_JUCEHEADER__
