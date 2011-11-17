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

#ifndef __JUCE_WIN32_DIRECTWRITE_JUCEHEADER__
#define __JUCE_WIN32_DIRECTWRITE_JUCEHEADER__


class SharedDirectWriteFactory  : public DeletedAtShutdown
{
public:
    SharedDirectWriteFactory()
    {
        D2D1CreateFactory (D2D1_FACTORY_TYPE_SINGLE_THREADED, d2dFactory.resetAndGetPointerAddress());
        DWriteCreateFactory (DWRITE_FACTORY_TYPE_SHARED, __uuidof (IDWriteFactory), (IUnknown**) directWriteFactory.resetAndGetPointerAddress());

        if (directWriteFactory != nullptr)
            directWriteFactory->GetSystemFontCollection (systemFonts.resetAndGetPointerAddress());
    }

    ~SharedDirectWriteFactory()
    {
        clearSingletonInstance();
    }

    juce_DeclareSingleton (SharedDirectWriteFactory, false);

    ComSmartPtr <ID2D1Factory> d2dFactory;
    ComSmartPtr <IDWriteFactory> directWriteFactory;
    ComSmartPtr <IDWriteFontCollection> systemFonts;
};

juce_ImplementSingleton (SharedDirectWriteFactory)

#endif   // __JUCE_WIN32_DIRECTWRITE_JUCEHEADER__
