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


class CustomDirectWriteTextRenderer   : public ComBaseClassHelper <IDWriteTextRenderer>
{
public:
    CustomDirectWriteTextRenderer()
        : currentLine (-1),
          lastOriginY (-10000.0f)
    {
        DWriteCreateFactory (DWRITE_FACTORY_TYPE_SHARED, __uuidof (IDWriteFactory),
                             (IUnknown**) dwFactory.resetAndGetPointerAddress());

        dwFactory->GetSystemFontCollection (dwFontCollection.resetAndGetPointerAddress());

        resetReferenceCount();
    }

    JUCE_COMRESULT QueryInterface (REFIID refId, void** result)
    {
       #if ! JUCE_MINGW
        if (refId == __uuidof (IDWritePixelSnapping))   { AddRef(); *result = dynamic_cast <IDWritePixelSnapping*> (this); return S_OK; }
       #else
        jassertfalse; // need to find a mingw equivalent of __uuidof to make this possible
       #endif

        return ComBaseClassHelper<IDWriteTextRenderer>::QueryInterface (refId, result);
    }

    JUCE_COMRESULT IsPixelSnappingDisabled (void* /*clientDrawingContext*/, BOOL* isDisabled)
    {
        *isDisabled = FALSE;
        return S_OK;
    }

    JUCE_COMRESULT GetCurrentTransform (void*, DWRITE_MATRIX*)                                          { return S_OK; }
    JUCE_COMRESULT GetPixelsPerDip (void*, FLOAT*)                                                      { return S_OK; }
    JUCE_COMRESULT DrawUnderline (void*, FLOAT, FLOAT, DWRITE_UNDERLINE const*, IUnknown*)              { return S_OK; }
    JUCE_COMRESULT DrawStrikethrough (void*, FLOAT, FLOAT, DWRITE_STRIKETHROUGH const*, IUnknown*)      { return S_OK; }
    JUCE_COMRESULT DrawInlineObject (void*, FLOAT, FLOAT, IDWriteInlineObject*, BOOL, BOOL, IUnknown*)  { return E_NOTIMPL; }

    JUCE_COMRESULT DrawGlyphRun (void* clientDrawingContext, FLOAT baselineOriginX, FLOAT baselineOriginY, DWRITE_MEASURING_MODE,
                                 DWRITE_GLYPH_RUN const* glyphRun, DWRITE_GLYPH_RUN_DESCRIPTION const* runDescription, IUnknown* clientDrawingEffect)
    {
        GlyphLayout* const glyphLayout = static_cast<GlyphLayout*> (clientDrawingContext);

        if (baselineOriginY != lastOriginY)
        {
            lastOriginY = baselineOriginY;
            ++currentLine;

            // The x value is only correct when dealing with LTR text
            glyphLayout->getLine (currentLine)
                .setLineOrigin (Point<float> (baselineOriginX - glyphLayout->area.getX(),
                                              baselineOriginY - glyphLayout->area.getY()));
        }

        if (currentLine < 0)
            return S_OK;

        GlyphLayout::Line& glyphLine = glyphLayout->getLine (currentLine);

        DWRITE_FONT_METRICS dwFontMetrics;
        glyphRun->fontFace->GetMetrics (&dwFontMetrics);

        const float descent = (std::abs ((float) dwFontMetrics.descent) /  (float) dwFontMetrics.designUnitsPerEm) * glyphRun->fontEmSize;

        if (descent > glyphLine.getDescent())
            glyphLine.setDescent (descent);

        int styleFlags = 0;
        const String fontName (getFontName (glyphRun, styleFlags));

        GlyphLayout::Run* const glyphRunLayout = new GlyphLayout::Run (glyphRun->glyphCount,
                                                                       Range<int> (runDescription->textPosition,
                                                                                   runDescription->textPosition + runDescription->stringLength));
        glyphLine.addRun (glyphRunLayout);

        glyphRun->fontFace->GetMetrics (&dwFontMetrics);

        const float totalHeight = std::abs ((float) dwFontMetrics.ascent) + std::abs ((float) dwFontMetrics.descent);
        const float fontHeightToEmSizeFactor = (float) dwFontMetrics.designUnitsPerEm / totalHeight;

        glyphRunLayout->setFont (Font (fontName, glyphRun->fontEmSize / fontHeightToEmSizeFactor, styleFlags));
        glyphRunLayout->setColour (getColourOf (clientDrawingEffect));

        float x = baselineOriginX;

        for (UINT32 i = 0; i < glyphRun->glyphCount; ++i)
        {
            if ((glyphRun->bidiLevel & 1) != 0)
                x -= glyphRun->glyphAdvances[i];  // RTL text

            glyphRunLayout->addGlyph (new GlyphLayout::Glyph (glyphRun->glyphIndices[i], Point<float> (x, baselineOriginY)));

            if ((glyphRun->bidiLevel & 1) == 0)
                x += glyphRun->glyphAdvances[i];  // LTR text
        }

        return S_OK;
    }

private:
    UINT refCount;
    ComSmartPtr<IDWriteFactory> dwFactory;
    ComSmartPtr<IDWriteFontCollection> dwFontCollection;
    int currentLine;
    float lastOriginY;

    static Colour getColourOf (IUnknown* clientDrawingEffect)
    {
        if (clientDrawingEffect == nullptr)
            return Colours::black;

        ID2D1SolidColorBrush* d2dBrush = static_cast<ID2D1SolidColorBrush*> (clientDrawingEffect);
        uint8 r = (uint8) (d2dBrush->GetColor().r * 255);
        uint8 g = (uint8) (d2dBrush->GetColor().g * 255);
        uint8 b = (uint8) (d2dBrush->GetColor().b * 255);
        uint8 a = (uint8) (d2dBrush->GetColor().a * 255);
        return Colour::fromRGBA(r, g, b, a);
    }

    String getFontName (DWRITE_GLYPH_RUN const* glyphRun, int& styleFlags) const
    {
        ComSmartPtr<IDWriteFont> dwFont;
        HRESULT hr = dwFontCollection->GetFontFromFontFace (glyphRun->fontFace, dwFont.resetAndGetPointerAddress());
        jassert (dwFont != nullptr);

        if (dwFont->GetWeight() == DWRITE_FONT_WEIGHT_BOLD) styleFlags &= Font::bold;
        if (dwFont->GetStyle() == DWRITE_FONT_STYLE_ITALIC) styleFlags &= Font::italic;

        ComSmartPtr<IDWriteFontFamily> dwFontFamily;
        hr = dwFont->GetFontFamily (dwFontFamily.resetAndGetPointerAddress());
        jassert (dwFontFamily != nullptr);

        // Get the Font Family Names
        ComSmartPtr<IDWriteLocalizedStrings> dwFamilyNames;
        hr = dwFontFamily->GetFamilyNames (dwFamilyNames.resetAndGetPointerAddress());
        jassert (dwFamilyNames != nullptr);

        UINT32 index = 0;
        BOOL exists = false;
        hr = dwFamilyNames->FindLocaleName (L"en-us", &index, &exists);
        if (! exists)
            index = 0;

        UINT32 length = 0;
        hr = dwFamilyNames->GetStringLength (index, &length);

        HeapBlock <wchar_t> name (length + 1);
        hr = dwFamilyNames->GetString (index, name, length + 1);

        return String (name);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomDirectWriteTextRenderer);
};


//==================================================================================================
namespace DirectWriteTypeLayout
{
    float getFontHeightToEmSizeFactor (const Font& font, IDWriteFontCollection& dwFontCollection)
    {
        BOOL fontFound = false;
        uint32 fontIndex;
        dwFontCollection.FindFamilyName (font.getTypefaceName().toWideCharPointer(), &fontIndex, &fontFound);

        if (! fontFound)
            fontIndex = 0;

        ComSmartPtr<IDWriteFontFamily> dwFontFamily;
        HRESULT hr = dwFontCollection.GetFontFamily (fontIndex, dwFontFamily.resetAndGetPointerAddress());

        ComSmartPtr<IDWriteFont> dwFont;
        hr = dwFontFamily->GetFirstMatchingFont (DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                                                 dwFont.resetAndGetPointerAddress());

        ComSmartPtr<IDWriteFontFace> dwFontFace;
        hr = dwFont->CreateFontFace (dwFontFace.resetAndGetPointerAddress());

        DWRITE_FONT_METRICS dwFontMetrics;
        dwFontFace->GetMetrics (&dwFontMetrics);

        const float totalHeight = (float) (std::abs (dwFontMetrics.ascent) + std::abs (dwFontMetrics.descent));
        return dwFontMetrics.designUnitsPerEm / totalHeight;
    }

    void createLayout (GlyphLayout& glyphLayout, const AttributedString& text)
    {
        ComSmartPtr<IDWriteFactory> dwFactory;
        HRESULT hr = DWriteCreateFactory (DWRITE_FACTORY_TYPE_SHARED, __uuidof (IDWriteFactory),
                                          (IUnknown**) dwFactory.resetAndGetPointerAddress());

        ComSmartPtr<IDWriteFontCollection> dwFontCollection;
        hr = dwFactory->GetSystemFontCollection (dwFontCollection.resetAndGetPointerAddress());

        // To add color to text, we need to create a D2D render target
        // Since we are not actually rendering to a D2D context we create a temporary GDI render target
        ComSmartPtr<ID2D1Factory> d2dFactory;
        hr = D2D1CreateFactory (D2D1_FACTORY_TYPE_SINGLE_THREADED, d2dFactory.resetAndGetPointerAddress());

        D2D1_RENDER_TARGET_PROPERTIES d2dRTProp = D2D1::RenderTargetProperties (D2D1_RENDER_TARGET_TYPE_SOFTWARE,
                                                                                D2D1::PixelFormat (DXGI_FORMAT_B8G8R8A8_UNORM,
                                                                                                   D2D1_ALPHA_MODE_IGNORE),
                                                                                0, 0,
                                                                                D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE,
                                                                                D2D1_FEATURE_LEVEL_DEFAULT);
        ComSmartPtr<ID2D1DCRenderTarget> d2dDCRT;
        hr = d2dFactory->CreateDCRenderTarget (&d2dRTProp, d2dDCRT.resetAndGetPointerAddress());

        Font defaultFont;
        const float defaultFontHeightToEmSizeFactor = getFontHeightToEmSizeFactor (defaultFont, *dwFontCollection);

        ComSmartPtr<IDWriteTextFormat> dwTextFormat;
        hr = dwFactory->CreateTextFormat (defaultFont.getTypefaceName().toWideCharPointer(),
                                          dwFontCollection,
                                          DWRITE_FONT_WEIGHT_REGULAR,
                                          DWRITE_FONT_STYLE_NORMAL,
                                          DWRITE_FONT_STRETCH_NORMAL,
                                          defaultFont.getHeight() * defaultFontHeightToEmSizeFactor,
                                          L"en-us",
                                          dwTextFormat.resetAndGetPointerAddress());

        if (text.getTextAlignment() == AttributedString::left)              dwTextFormat->SetTextAlignment (DWRITE_TEXT_ALIGNMENT_LEADING);
        if (text.getTextAlignment() == AttributedString::right)             dwTextFormat->SetTextAlignment (DWRITE_TEXT_ALIGNMENT_TRAILING);
        if (text.getTextAlignment() == AttributedString::center)            dwTextFormat->SetTextAlignment (DWRITE_TEXT_ALIGNMENT_CENTER);

        // DirectWrite cannot justify text, default to left alignment
        if (text.getTextAlignment() == AttributedString::justified)         dwTextFormat->SetTextAlignment (DWRITE_TEXT_ALIGNMENT_LEADING);

        if (text.getWordWrap() == AttributedString::none)                   dwTextFormat->SetWordWrapping (DWRITE_WORD_WRAPPING_NO_WRAP);
        if (text.getWordWrap() == AttributedString::byWord)                 dwTextFormat->SetWordWrapping (DWRITE_WORD_WRAPPING_WRAP);

        // DirectWrite does not support wrapping by character, default to wrapping by word
        if (text.getWordWrap() == AttributedString::byChar)                 dwTextFormat->SetWordWrapping (DWRITE_WORD_WRAPPING_WRAP);

        // DirectWrite does not automatically set reading direction
        // This must be set correctly and manually when using RTL Scripts (Hebrew, Arabic)
        if (text.getReadingDirection() == AttributedString::rightToLeft)    dwTextFormat->SetReadingDirection (DWRITE_READING_DIRECTION_RIGHT_TO_LEFT);

        const int textLen = text.getText().length();

        ComSmartPtr<IDWriteTextLayout> dwTextLayout;
        hr = dwFactory->CreateTextLayout (text.getText().toWideCharPointer(), textLen,
                                          dwTextFormat,
                                          glyphLayout.area.getWidth(),
                                          glyphLayout.area.getHeight(),
                                          dwTextLayout.resetAndGetPointerAddress());

        const int numAttributes = text.getNumAttributes();

        for (int i = 0; i < numAttributes; ++i)
        {
            const AttributedString::Attribute* const attr = text.getAttribute (i);

            if (attr->range.getStart() < textLen)
            {
                Range<int> range (attr->range);

                if (range.getEnd() > textLen)
                    range.setEnd (textLen);

                DWRITE_TEXT_RANGE dwRange;
                dwRange.startPosition = range.getStart();
                dwRange.length = range.getLength();

                if (attr->getFont() != nullptr)
                {
                    dwTextLayout->SetFontFamilyName (attr->getFont()->getTypefaceName().toWideCharPointer(), dwRange);

                    const float fontHeightToEmSizeFactor = getFontHeightToEmSizeFactor (*attr->getFont(), *dwFontCollection);
                    dwTextLayout->SetFontSize (attr->getFont()->getHeight() * fontHeightToEmSizeFactor, dwRange);
                }

                if (attr->getColour() != nullptr)
                {
                    ComSmartPtr<ID2D1SolidColorBrush> d2dBrush;
                    d2dDCRT->CreateSolidColorBrush (D2D1::ColorF (D2D1::ColorF (attr->getColour()->getFloatRed(),
                                                                                attr->getColour()->getFloatGreen(),
                                                                                attr->getColour()->getFloatBlue(),
                                                                                attr->getColour()->getFloatAlpha())),
                                                    d2dBrush.resetAndGetPointerAddress());

                    // We need to call SetDrawingEffect with a legimate brush to get DirectWrite to break text based on colours
                    dwTextLayout->SetDrawingEffect (d2dBrush, dwRange);
                }
            }
        }

        UINT32 actualLineCount = 0;
        hr = dwTextLayout->GetLineMetrics (nullptr, 0, &actualLineCount);

        glyphLayout.ensureStorageAllocated (actualLineCount);

        HeapBlock <DWRITE_LINE_METRICS> dwLineMetrics (actualLineCount);
        hr = dwTextLayout->GetLineMetrics (dwLineMetrics, actualLineCount, &actualLineCount);
        int lastLocation = 0;

        for (UINT32 i = 0; i < actualLineCount; ++i)
        {
            const Range<int> lineStringRange (lastLocation, (int) lastLocation + dwLineMetrics[i].length);
            lastLocation = dwLineMetrics[i].length;

            GlyphLayout::Line* glyphLine = new GlyphLayout::Line();
            glyphLayout.addLine (glyphLine);
            glyphLine->setStringRange (lineStringRange);
        }

        ComSmartPtr<CustomDirectWriteTextRenderer> textRenderer = new CustomDirectWriteTextRenderer();
        hr = dwTextLayout->Draw (&glyphLayout,
                                 textRenderer,
                                 glyphLayout.area.getX(),
                                 glyphLayout.area.getY());
    }
}

void GlyphLayout::setText (const AttributedString& text)
{
   #if 1
    DirectWriteTypeLayout::createLayout (*this, text);
   #else
    createStandardLayout (text);
   #endif
}
