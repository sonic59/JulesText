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

#ifndef __JUCE_ATTRIBUTEDSTRING_JUCEHEADER__
#define __JUCE_ATTRIBUTEDSTRING_JUCEHEADER__


//==============================================================================
/**
*/
class JUCE_API  AttributedString
{
public:
    /** */
    AttributedString();

    /** */
    explicit AttributedString (const String& text);

    /** Destructor. */
    ~AttributedString();

    //==============================================================================
    /** */
    const String& getText() const noexcept                  { return text; }

    /** */
    void setText (const String& newText);

    //==============================================================================
    /** */
    enum TextAlignment
    {
        left,
        right,
        center,
        justified,
    };

    /** */
    TextAlignment getTextAlignment() const noexcept         { return textAlignment; }
    /** */
    void setTextAlignment (TextAlignment newTextAlignment) noexcept;

    //==============================================================================
    /** */
    enum WordWrap
    {
        none,
        byWord,
        byChar,
    };

    /** */
    WordWrap getWordWrap() const noexcept                   { return wordWrap; }
    /** */
    void setWordWrap (WordWrap newWordWrap) noexcept;

    //==============================================================================
    /** */
    enum ReadingDirection
    {
        natural,
        leftToRight,
        rightToLeft,
    };

    /** */
    ReadingDirection getReadingDirection() const noexcept   { return readingDirection; }
    /** */
    void setReadingDirection (ReadingDirection newReadingDirection) noexcept;

    //==============================================================================
    /** */
    float getLineSpacing() const noexcept                   { return lineSpacing; }
    /** */
    void setLineSpacing (float newLineSpacing) noexcept;

    //==============================================================================
    /** */
    class JUCE_API  Attribute
    {
    public:
        Attribute (const Range<int>& range, const Colour& colour);
        Attribute (const Range<int>& range, const Font& font);
        ~Attribute();

        /** */
        const Font* getFont() const noexcept            { return font; }
        /** */
        const Colour* getColour() const noexcept        { return colour; }

        /** */
        const Range<int> range;

    private:
        ScopedPointer<Font> font;
        ScopedPointer<Colour> colour;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Attribute);
    };

    /** */
    int getNumAttributes() const noexcept                       { return attributes.size(); }
    /** */
    const Attribute* getAttribute (int index) const noexcept    { return attributes.getUnchecked (index); }

    //==============================================================================
    /** */
    void setColour (const Range<int>& range, const Colour& colour);

    /** */
    void setFont (const Range<int>& range, const Font& font);

    //==============================================================================
    /** */
    void draw (Graphics& g, const Rectangle<int>& area);

    /** */
    static void drawMultiple (Graphics& g, const Rectangle<int>& area, const AttributedString* const* strings, int numStrings);

private:
    String text;
    float lineSpacing;
    TextAlignment textAlignment;
    WordWrap wordWrap;
    ReadingDirection readingDirection;
    OwnedArray<Attribute> attributes;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AttributedString);
};


#endif   // __JUCE_ATTRIBUTEDSTRING_JUCEHEADER__
