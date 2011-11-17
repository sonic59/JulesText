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

#ifndef __JUCE_GLYPHLAYOUT_JUCEHEADER__
#define __JUCE_GLYPHLAYOUT_JUCEHEADER__

#include "juce_Font.h"
#include "../contexts/juce_GraphicsContext.h"


//==============================================================================
/**
*/
class JUCE_API  GlyphLayout
{
public:
    /** */
    explicit GlyphLayout (const Rectangle<float>& area);

    /** Destructor. */
    ~GlyphLayout();

    //==============================================================================
    /**
    */
    class JUCE_API  Glyph
    {
    public:
        Glyph (int glyphCode, const Point<float>& anchor);
        ~Glyph();

        const int glyphCode;
        const Point<float> anchor;

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Glyph);
    };

    //==============================================================================
    /**
    */
    class JUCE_API  Run
    {
    public:
        Run();
        Run (int numGlyphs, const Range<int>& range);
        ~Run();

        int getNumGlyphs() const noexcept       { return glyphs.size(); }
        const Font& getFont() const noexcept    { return font; }
        const Colour& getColour() const         { return colour; }
        Glyph& getGlyph (int index) const;

        void setStringRange (const Range<int>& newStringRange) noexcept;
        void setFont (const Font& newFont);
        void setColour (const Colour& newColour) noexcept;

        void addGlyph (Glyph* glyph);
        void ensureStorageAllocated (int numGlyphsNeeded);

    private:
        OwnedArray<Glyph> glyphs;
        Range<int> stringRange;
        Font font;
        Colour colour;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Run);
    };

    //==============================================================================
    /**
    */
    class JUCE_API  Line
    {
    public:
        Line();
        Line (int numRuns, const Range<int>& stringRange,
              const Point<float>& lineOrigin,
              float ascent, float descent, float leading);
        ~Line();

        const Point<float>& getLineOrigin() const noexcept      { return lineOrigin; }

        float getAscent() const noexcept                        { return ascent; }
        float getDescent() const noexcept                       { return descent; }
        float getLeading() const noexcept                       { return leading; }

        int getNumRuns() const noexcept                         { return runs.size(); }
        Run& getRun (int index) const;

        void setStringRange (const Range<int>& newStringRange);
        void setLineOrigin (const Point<float>& newLineOrigin);
        void setDescent (float newDescent);

        void addRun (Run* run);

    private:
        OwnedArray<Run> runs;
        Range<int> stringRange;
        Point<float> lineOrigin;
        float ascent, descent, leading;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Line);
    };

    //==============================================================================
    /** */
    int getNumLines() const noexcept    { return lines.size(); }

    /** */
    float getTextHeight() const;
    /** */
    Line& getLine (int index) const;

    /** */
    void setText (const AttributedString& text);

    /** */
    void addLine (const Line* line);
    /** */
    void ensureStorageAllocated (int numLinesNeeded);

    /** */
    void draw (const Graphics& g) const;

    /** */
    const Rectangle<float> area;

private:
    OwnedArray<Line> lines;

    void createStandardLayout (const AttributedString&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GlyphLayout);
};


#endif   // __JUCE_GLYPHLAYOUT_JUCEHEADER__
