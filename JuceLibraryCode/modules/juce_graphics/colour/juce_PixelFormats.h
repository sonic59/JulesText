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

#ifndef __JUCE_PIXELFORMATS_JUCEHEADER__
#define __JUCE_PIXELFORMATS_JUCEHEADER__


//==============================================================================
#ifndef DOXYGEN
 #if JUCE_MSVC
  #pragma pack (push, 1)
  #define PACKED
 #elif JUCE_GCC
  #define PACKED __attribute__((packed))
 #else
  #define PACKED
 #endif
#endif

class PixelRGB;
class PixelAlpha;

//==============================================================================
/**
    Represents a 32-bit ARGB pixel with premultiplied alpha, and can perform compositing
    operations with it.

    This is used internally by the imaging classes.

    @see PixelRGB
*/
class JUCE_API  PixelARGB
{
public:
    /** Creates a pixel without defining its colour. */
    PixelARGB() noexcept {}
    ~PixelARGB() noexcept {}

    /** Creates a pixel from a 32-bit argb value.
    */
    PixelARGB (const uint32 argb_) noexcept
        : argb (argb_)
    {
    }

    PixelARGB (const uint8 a, const uint8 r, const uint8 g, const uint8 b) noexcept
    {
        components.b = b;
        components.g = g;
        components.r = r;
        components.a = a;
    }

    forcedinline uint32 getARGB() const noexcept                { return argb; }
    forcedinline uint32 getUnpremultipliedARGB() const noexcept { PixelARGB p (argb); p.unpremultiply(); return p.getARGB(); }

    forcedinline uint32 getRB() const noexcept      { return 0x00ff00ff & argb; }
    forcedinline uint32 getAG() const noexcept      { return 0x00ff00ff & (argb >> 8); }

    forcedinline uint8 getAlpha() const noexcept    { return components.a; }
    forcedinline uint8 getRed() const noexcept      { return components.r; }
    forcedinline uint8 getGreen() const noexcept    { return components.g; }
    forcedinline uint8 getBlue() const noexcept     { return components.b; }

    /** Blends another pixel onto this one.

        This takes into account the opacity of the pixel being overlaid, and blends
        it accordingly.
    */
    template <class Pixel>
    forcedinline void blend (const Pixel& src) noexcept
    {
        uint32 sargb = src.getARGB();
        const uint32 alpha = 0x100 - (sargb >> 24);

        sargb += 0x00ff00ff & ((getRB() * alpha) >> 8);
        sargb += 0xff00ff00 & (getAG() * alpha);

        argb = sargb;
    }

    /** Blends another pixel onto this one.

        This takes into account the opacity of the pixel being overlaid, and blends
        it accordingly.
    */
    forcedinline void blend (const PixelRGB& src) noexcept;


    /** Blends another pixel onto this one, applying an extra multiplier to its opacity.

        The opacity of the pixel being overlaid is scaled by the extraAlpha factor before
        being used, so this can blend semi-transparently from a PixelRGB argument.
    */
    template <class Pixel>
    forcedinline void blend (const Pixel& src, uint32 extraAlpha) noexcept
    {
        ++extraAlpha;

        uint32 sargb = ((extraAlpha * src.getAG()) & 0xff00ff00)
                         | (((extraAlpha * src.getRB()) >> 8) & 0x00ff00ff);

        const uint32 alpha = 0x100 - (sargb >> 24);

        sargb += 0x00ff00ff & ((getRB() * alpha) >> 8);
        sargb += 0xff00ff00 & (getAG() * alpha);

        argb = sargb;
    }

    /** Blends another pixel with this one, creating a colour that is somewhere
        between the two, as specified by the amount.
    */
    template <class Pixel>
    forcedinline void tween (const Pixel& src, const uint32 amount) noexcept
    {
        uint32 drb = getRB();
        drb += (((src.getRB() - drb) * amount) >> 8);
        drb &= 0x00ff00ff;

        uint32 dag = getAG();
        dag += (((src.getAG() - dag) * amount) >> 8);
        dag &= 0x00ff00ff;
        dag <<= 8;

        dag |= drb;
        argb = dag;
    }

    /** Copies another pixel colour over this one.

        This doesn't blend it - this colour is simply replaced by the other one.
    */
    template <class Pixel>
    forcedinline void set (const Pixel& src) noexcept
    {
        argb = src.getARGB();
    }

    /** Replaces the colour's alpha value with another one. */
    forcedinline void setAlpha (const uint8 newAlpha) noexcept
    {
        components.a = newAlpha;
    }

    /** Multiplies the colour's alpha value with another one. */
    forcedinline void multiplyAlpha (int multiplier) noexcept
    {
        ++multiplier;

        argb = ((multiplier * getAG()) & 0xff00ff00)
                | (((multiplier * getRB()) >> 8) & 0x00ff00ff);
    }

    forcedinline void multiplyAlpha (const float multiplier) noexcept
    {
        multiplyAlpha ((int) (multiplier * 255.0f));
    }

    /** Sets the pixel's colour from individual components. */
    void setARGB (const uint8 a, const uint8 r, const uint8 g, const uint8 b) noexcept
    {
        components.b = b;
        components.g = g;
        components.r = r;
        components.a = a;
    }

    /** Premultiplies the pixel's RGB values by its alpha. */
    forcedinline void premultiply() noexcept
    {
        const uint32 alpha = components.a;

        if (alpha < 0xff)
        {
            if (alpha == 0)
            {
                components.b = 0;
                components.g = 0;
                components.r = 0;
            }
            else
            {
                components.b = (uint8) ((components.b * alpha + 0x7f) >> 8);
                components.g = (uint8) ((components.g * alpha + 0x7f) >> 8);
                components.r = (uint8) ((components.r * alpha + 0x7f) >> 8);
            }
        }
    }

    /** Unpremultiplies the pixel's RGB values. */
    forcedinline void unpremultiply() noexcept
    {
        const uint32 alpha = components.a;

        if (alpha < 0xff)
        {
            if (alpha == 0)
            {
                components.b = 0;
                components.g = 0;
                components.r = 0;
            }
            else
            {
                components.b = (uint8) jmin ((uint32) 0xff, (components.b * 0xff) / alpha);
                components.g = (uint8) jmin ((uint32) 0xff, (components.g * 0xff) / alpha);
                components.r = (uint8) jmin ((uint32) 0xff, (components.r * 0xff) / alpha);
            }
        }
    }

    forcedinline void desaturate() noexcept
    {
        if (components.a < 0xff && components.a > 0)
        {
            const int newUnpremultipliedLevel = (0xff * ((int) components.r + (int) components.g + (int) components.b) / (3 * components.a));

            components.r = components.g = components.b
                = (uint8) ((newUnpremultipliedLevel * components.a + 0x7f) >> 8);
        }
        else
        {
            components.r = components.g = components.b
                = (uint8) (((int) components.r + (int) components.g + (int) components.b) / 3);
        }
    }

    //==============================================================================
    /** The indexes of the different components in the byte layout of this type of colour. */
   #if JUCE_BIG_ENDIAN
    enum { indexA = 0, indexR = 1, indexG = 2, indexB = 3 };
   #else
    enum { indexA = 3, indexR = 2, indexG = 1, indexB = 0 };
   #endif

private:
    //==============================================================================
    struct Components
    {
       #if JUCE_BIG_ENDIAN
        uint8 a : 8, r : 8, g : 8, b : 8;
       #else
        uint8 b, g, r, a;
       #endif
    } PACKED;

    union
    {
        uint32 argb;
        Components components;
    };
}
#ifndef DOXYGEN
 PACKED
#endif
;


//==============================================================================
/**
    Represents a 24-bit RGB pixel, and can perform compositing operations on it.

    This is used internally by the imaging classes.

    @see PixelARGB
*/
class JUCE_API  PixelRGB
{
public:
    /** Creates a pixel without defining its colour. */
    PixelRGB() noexcept {}
    ~PixelRGB() noexcept {}

    /** Creates a pixel from a 32-bit argb value.

        (The argb format is that used by PixelARGB)
    */
    PixelRGB (const uint32 argb) noexcept
    {
        r = (uint8) (argb >> 16);
        g = (uint8) (argb >> 8);
        b = (uint8) (argb);
    }

    forcedinline uint32 getARGB() const noexcept                { return 0xff000000 | b | (g << 8) | (r << 16); }
    forcedinline uint32 getUnpremultipliedARGB() const noexcept { return getARGB(); }

    forcedinline uint32 getRB() const noexcept      { return b | (uint32) (r << 16); }
    forcedinline uint32 getAG() const noexcept      { return (uint32) (0xff0000 | g); }

    forcedinline uint8 getAlpha() const noexcept    { return 0xff; }
    forcedinline uint8 getRed() const noexcept      { return r; }
    forcedinline uint8 getGreen() const noexcept    { return g; }
    forcedinline uint8 getBlue() const noexcept     { return b; }

    /** Blends another pixel onto this one.

        This takes into account the opacity of the pixel being overlaid, and blends
        it accordingly.
    */
    template <class Pixel>
    forcedinline void blend (const Pixel& src) noexcept
    {
        uint32 sargb = src.getARGB();
        const uint32 alpha = 0x100 - (sargb >> 24);

        sargb += 0x00ff00ff & ((getRB() * alpha) >> 8);
        sargb += 0x0000ff00 & (g * alpha);

        r = (uint8) (sargb >> 16);
        g = (uint8) (sargb >> 8);
        b = (uint8) sargb;
    }

    forcedinline void blend (const PixelRGB& src) noexcept
    {
        set (src);
    }

    /** Blends another pixel onto this one, applying an extra multiplier to its opacity.

        The opacity of the pixel being overlaid is scaled by the extraAlpha factor before
        being used, so this can blend semi-transparently from a PixelRGB argument.
    */
    template <class Pixel>
    forcedinline void blend (const Pixel& src, uint32 extraAlpha) noexcept
    {
        ++extraAlpha;
        const uint32 srb = (extraAlpha * src.getRB()) >> 8;
        const uint32 sag = extraAlpha * src.getAG();
        uint32 sargb = (sag & 0xff00ff00) | (srb & 0x00ff00ff);

        const uint32 alpha = 0x100 - (sargb >> 24);

        sargb += 0x00ff00ff & ((getRB() * alpha) >> 8);
        sargb += 0x0000ff00 & (g * alpha);

        b = (uint8) sargb;
        g = (uint8) (sargb >> 8);
        r = (uint8) (sargb >> 16);
    }

    /** Blends another pixel with this one, creating a colour that is somewhere
        between the two, as specified by the amount.
    */
    template <class Pixel>
    forcedinline void tween (const Pixel& src, const uint32 amount) noexcept
    {
        uint32 drb = getRB();
        drb += (((src.getRB() - drb) * amount) >> 8);

        uint32 dag = getAG();
        dag += (((src.getAG() - dag) * amount) >> 8);

        b = (uint8) drb;
        g = (uint8) dag;
        r = (uint8) (drb >> 16);
    }

    /** Copies another pixel colour over this one.

        This doesn't blend it - this colour is simply replaced by the other one.
        Because PixelRGB has no alpha channel, any alpha value in the source pixel
        is thrown away.
    */
    template <class Pixel>
    forcedinline void set (const Pixel& src) noexcept
    {
        b = src.getBlue();
        g = src.getGreen();
        r = src.getRed();
    }

    /** This method is included for compatibility with the PixelARGB class. */
    forcedinline void setAlpha (const uint8) noexcept {}

    /** Multiplies the colour's alpha value with another one. */
    forcedinline void multiplyAlpha (int) noexcept {}

    /** Multiplies the colour's alpha value with another one. */
    forcedinline void multiplyAlpha (float) noexcept {}

    /** Sets the pixel's colour from individual components. */
    void setARGB (const uint8, const uint8 r_, const uint8 g_, const uint8 b_) noexcept
    {
        r = r_;
        g = g_;
        b = b_;
    }

    /** Premultiplies the pixel's RGB values by its alpha. */
    forcedinline void premultiply() noexcept {}

    /** Unpremultiplies the pixel's RGB values. */
    forcedinline void unpremultiply() noexcept {}

    forcedinline void desaturate() noexcept
    {
        r = g = b = (uint8) (((int) r + (int) g + (int) b) / 3);
    }

    //==============================================================================
    /** The indexes of the different components in the byte layout of this type of colour. */
   #if JUCE_MAC
    enum { indexR = 0, indexG = 1, indexB = 2 };
   #else
    enum { indexR = 2, indexG = 1, indexB = 0 };
   #endif

private:
    //==============================================================================
   #if JUCE_MAC
    uint8 r, g, b;
   #else
    uint8 b, g, r;
   #endif

}
#ifndef DOXYGEN
 PACKED
#endif
;

forcedinline void PixelARGB::blend (const PixelRGB& src) noexcept
{
    set (src);
}

//==============================================================================
/**
    Represents an 8-bit single-channel pixel, and can perform compositing operations on it.

    This is used internally by the imaging classes.

    @see PixelARGB, PixelRGB
*/
class JUCE_API  PixelAlpha
{
public:
    /** Creates a pixel without defining its colour. */
    PixelAlpha() noexcept {}
    ~PixelAlpha() noexcept {}

    /** Creates a pixel from a 32-bit argb value.

        (The argb format is that used by PixelARGB)
    */
    PixelAlpha (const uint32 argb) noexcept
    {
        a = (uint8) (argb >> 24);
    }

    forcedinline uint32 getARGB() const noexcept                { return (((uint32) a) << 24) | (((uint32) a) << 16) | (((uint32) a) << 8) | a; }
    forcedinline uint32 getUnpremultipliedARGB() const noexcept { return (((uint32) a) << 24) | 0xffffff; }

    forcedinline uint32 getRB() const noexcept      { return (((uint32) a) << 16) | a; }
    forcedinline uint32 getAG() const noexcept      { return (((uint32) a) << 16) | a; }

    forcedinline uint8 getAlpha() const noexcept    { return a; }
    forcedinline uint8 getRed() const noexcept      { return 0; }
    forcedinline uint8 getGreen() const noexcept    { return 0; }
    forcedinline uint8 getBlue() const noexcept     { return 0; }

    /** Blends another pixel onto this one.

        This takes into account the opacity of the pixel being overlaid, and blends
        it accordingly.
    */
    template <class Pixel>
    forcedinline void blend (const Pixel& src) noexcept
    {
        const int srcA = src.getAlpha();
        a = (uint8) ((a * (0x100 - srcA) >> 8) + srcA);
    }

    /** Blends another pixel onto this one, applying an extra multiplier to its opacity.

        The opacity of the pixel being overlaid is scaled by the extraAlpha factor before
        being used, so this can blend semi-transparently from a PixelRGB argument.
    */
    template <class Pixel>
    forcedinline void blend (const Pixel& src, uint32 extraAlpha) noexcept
    {
        ++extraAlpha;
        const int srcAlpha = (int) ((extraAlpha * src.getAlpha()) >> 8);
        a = (uint8) ((a * (0x100 - srcAlpha) >> 8) + srcAlpha);
    }

    /** Blends another pixel with this one, creating a colour that is somewhere
        between the two, as specified by the amount.
    */
    template <class Pixel>
    forcedinline void tween (const Pixel& src, const uint32 amount) noexcept
    {
        a += ((src.getAlpha() - a) * amount) >> 8;
    }

    /** Copies another pixel colour over this one.

        This doesn't blend it - this colour is simply replaced by the other one.
    */
    template <class Pixel>
    forcedinline void set (const Pixel& src) noexcept
    {
        a = src.getAlpha();
    }

    /** Replaces the colour's alpha value with another one. */
    forcedinline void setAlpha (const uint8 newAlpha) noexcept
    {
        a = newAlpha;
    }

    /** Multiplies the colour's alpha value with another one. */
    forcedinline void multiplyAlpha (int multiplier) noexcept
    {
        ++multiplier;
        a = (uint8) ((a * multiplier) >> 8);
    }

    forcedinline void multiplyAlpha (const float multiplier) noexcept
    {
        a = (uint8) (a * multiplier);
    }

    /** Sets the pixel's colour from individual components. */
    forcedinline void setARGB (const uint8 a_, const uint8 /*r*/, const uint8 /*g*/, const uint8 /*b*/) noexcept
    {
        a = a_;
    }

    /** Premultiplies the pixel's RGB values by its alpha. */
    forcedinline void premultiply() noexcept {}

    /** Unpremultiplies the pixel's RGB values. */
    forcedinline void unpremultiply() noexcept {}

    forcedinline void desaturate() noexcept {}

    //==============================================================================
    /** The indexes of the different components in the byte layout of this type of colour. */
    enum { indexA = 0 };

private:
    //==============================================================================
    uint8 a : 8;
}
#ifndef DOXYGEN
 PACKED
#endif
;

#if JUCE_MSVC
 #pragma pack (pop)
#endif

#undef PACKED

#endif   // __JUCE_PIXELFORMATS_JUCEHEADER__
