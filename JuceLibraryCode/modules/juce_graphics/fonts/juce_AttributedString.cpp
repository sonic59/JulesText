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
AttributedString::Attribute::Attribute (const Range<int>& range_, const Colour& colour_)
    : range (range_), colour (new Colour (colour_))
{
}

AttributedString::Attribute::Attribute (const Range<int>& range_, const Font& font_)
    : range (range_), font (new Font (font_))
{
}

AttributedString::Attribute::~Attribute() {}

//==============================================================================
AttributedString::AttributedString()
    : lineSpacing (0.0f),
      textAlignment (AttributedString::left),
      wordWrap (AttributedString::byWord),
      readingDirection (AttributedString::natural)
{
}

AttributedString::AttributedString (const String& newString)
    : text (newString),
      lineSpacing (0.0f),
      textAlignment (AttributedString::left),
      wordWrap (AttributedString::byWord),
      readingDirection (AttributedString::natural)
{
}

AttributedString::~AttributedString() {}

void AttributedString::setText (const String& other)
{
    text = other;
}

void AttributedString::setTextAlignment (TextAlignment newTextAlignment) noexcept
{
    textAlignment = newTextAlignment;
}

void AttributedString::setWordWrap (WordWrap newWordWrap) noexcept
{
    wordWrap = newWordWrap;
}

void AttributedString::setReadingDirection (ReadingDirection newReadingDirection) noexcept
{
    readingDirection = newReadingDirection;
}

void AttributedString::setLineSpacing (const float newLineSpacing) noexcept
{
    lineSpacing = newLineSpacing;
}

void AttributedString::setColour (const Range<int>& range, const Colour& colour)
{
    attributes.add (new Attribute (range, colour));
}

void AttributedString::setFont (const Range<int>& range, const Font& font)
{
    attributes.add (new Attribute (range, font));
}

static void drawAttributedString (Graphics& g, const Rectangle<int>& area,
                                  const AttributedString& text, float* const textHeight)
{
    if (! g.getInternalContext()->drawTextLayout (text, area, textHeight))
    {
        GlyphLayout layout (area.toFloat());
        layout.setText (text);
        layout.draw (g);

        if (textHeight != nullptr)
            *textHeight = layout.getTextHeight();
    }
}

void AttributedString::draw (Graphics& g, const Rectangle<int>& area)
{
    if (text.isNotEmpty() && g.clipRegionIntersects (area))
        drawAttributedString (g, area, *this, nullptr);
}

void AttributedString::drawMultiple (Graphics& g, const Rectangle<int>& area,
                                     const AttributedString* const* strings, int numStrings)
{
    if (numStrings > 0 && g.clipRegionIntersects (area))
    {
        int y = 0;

        for (int i = 0; i < numStrings && y < area.getHeight(); ++i)
        {
            if (strings[i]->getText().isEmpty())
            {
                y += 10;
            }
            else
            {
                float height = 0;
                drawAttributedString (g, area.withTop (area.getY() + y), *strings[i],
                                      (i < numStrings - 1) ? &height : nullptr);
                y += (int) height;
            }
        }
    }
}

END_JUCE_NAMESPACE
