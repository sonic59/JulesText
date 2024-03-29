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

#ifndef __JUCE_OPENGLHELPERS_JUCEHEADER__
#define __JUCE_OPENGLHELPERS_JUCEHEADER__

class OpenGLTexture;
class OpenGLFrameBuffer;


//==============================================================================
/**
    A set of miscellaneous openGL helper functions.
*/
class JUCE_API  OpenGLHelpers
{
public:
    /** Clears the GL error state. */
    static void resetErrorState();

    /** Returns true if the current thread has an active OpenGL context. */
    static bool isContextActive();

    /** Clears the current context using the given colour. */
    static void clear (const Colour& colour);

    /** Sets the current colour using a JUCE colour. */
    static void setColour (const Colour& colour);

    /** Gives the current context an orthoganal rendering mode for 2D drawing into the given size. */
    static void prepareFor2D (int width, int height);

    /** This does the same job as gluPerspective(). */
    static void setPerspective (double fovy, double aspect, double zNear, double zFar);

    static void applyTransform (const AffineTransform& t);

    static void enableScissorTest (const Rectangle<int>& clip);

    /** Draws a 2D quad with the specified corner points. */
    static void drawQuad2D (float x1, float y1,
                            float x2, float y2,
                            float x3, float y3,
                            float x4, float y4,
                            const Colour& colour);

    /** Draws a 3D quad with the specified corner points. */
    static void drawQuad3D (float x1, float y1, float z1,
                            float x2, float y2, float z2,
                            float x3, float y3, float z3,
                            float x4, float y4, float z4,
                            const Colour& colour);

    static void drawTriangleStrip (const GLfloat* const vertices, const GLfloat* const textureCoords, const int numVertices) noexcept;

    static void drawTriangleStrip (const GLfloat* const vertices, const GLfloat* const textureCoords,
                                   const int numVertices, const GLuint textureID) noexcept;

    static void drawTextureQuad (GLuint textureID, const Rectangle<int>& rect);

    static void fillRectWithTexture (const Rectangle<int>& rect, GLuint textureID, const float alpha);

    static void fillRect (const Rectangle<int>& rect);

    /** Fills a rectangle with the specified colour. */
    static void fillRectWithColour (const Rectangle<int>& rect,
                                    const Colour& colour);

    /** Checks whether the current context supports the specified extension. */
    static bool isExtensionSupported (const char* extensionName);

    /** Returns the address of a named GL extension function */
    static void* getExtensionFunction (const char* functionName);
};

//==============================================================================
/**
    Used as a local object while rendering, this will create a temporary texture ID
    from an image in the quickest way possible.

    If the image is backed by an OpenGL framebuffer, it will use that directly; otherwise,
    this object will create a temporary texture or framebuffer and copy the image.
*/
class JUCE_API  OpenGLTextureFromImage
{
public:
    OpenGLTextureFromImage (const Image& image);
    ~OpenGLTextureFromImage();

    GLuint textureID;
    const int imageWidth, imageHeight;
    float fullWidthProportion, fullHeightProportion;

private:
    ScopedPointer<OpenGLTexture> texture;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenGLTextureFromImage);
};


#endif   // __JUCE_OPENGLHELPERS_JUCEHEADER__
