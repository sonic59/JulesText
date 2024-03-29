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

#ifdef __JUCE_VIDEO_JUCEHEADER__
 /* When you add this cpp file to your project, you mustn't include it in a file where you've
    already included any other headers - just put it inside a file on its own, possibly with your config
    flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
    header files that the compiler may be using.
 */
 #error "Incorrect use of JUCE cpp file"
#endif

// Your project must contain an AppConfig.h file with your project-specific settings in it,
// and your header search path must make it accessible to the module's files.
#include "AppConfig.h"

#include "../juce_core/native/juce_BasicNativeHeaders.h"
#include "../juce_gui_extra/juce_gui_extra.h"
#include "juce_video.h"

#if JUCE_MAC
 #if JUCE_QUICKTIME
  #define Point CarbonDummyPointName
  #define Component CarbonDummyCompName
  #import <QTKit/QTKit.h>
  #undef Point
  #undef Component
 #endif

//==============================================================================
#elif JUCE_WINDOWS
 #if JUCE_QUICKTIME
  /* If you've got an include error here, you probably need to install the QuickTime SDK and
     add its header directory to your include path.

     Alternatively, if you don't need any QuickTime services, just set the JUCE_QUICKTIME flag to 0.
  */
  #include <Movies.h>
  #include <QTML.h>
  #include <QuickTimeComponents.h>
  #include <MediaHandlers.h>
  #include <ImageCodec.h>

  /* If you've got QuickTime 7 installed, then these COM objects should be found in
     the "\Program Files\Quicktime" directory. You'll need to add this directory to
     your include search path to make these import statements work.
  */
  #import <QTOLibrary.dll>
  #import <QTOControl.dll>

  #if JUCE_MSVC && ! JUCE_DONT_AUTOLINK_TO_WIN32_LIBRARIES
   #pragma comment (lib, "QTMLClient.lib")
  #endif
 #endif

 #if JUCE_USE_CAMERA || JUCE_DIRECTSHOW
  /* If you're using the camera classes, you'll need access to a few DirectShow headers.

     These files are provided in the normal Windows SDK, but some Microsoft plonker
     didn't realise that qedit.h doesn't actually compile without the rest of the DirectShow SDK..
     Microsoft's suggested fix for this is to hack their qedit.h file! See:
     http://social.msdn.microsoft.com/Forums/en-US/windowssdk/thread/ed097d2c-3d68-4f48-8448-277eaaf68252
     .. which is a bit of a bodge, but a lot less hassle than installing the full DShow SDK.

     An alternative workaround is to create a dummy dxtrans.h file and put it in your include path.
     The dummy file just needs to contain the following content:
        #define __IDxtCompositor_INTERFACE_DEFINED__
        #define __IDxtAlphaSetter_INTERFACE_DEFINED__
        #define __IDxtJpeg_INTERFACE_DEFINED__
        #define __IDxtKey_INTERFACE_DEFINED__
    ..and that should be enough to convince qedit.h that you have the SDK!
  */
  #include <dshow.h>
  #include <qedit.h>
  #include <dshowasf.h>
 #endif

 #if JUCE_DIRECTSHOW && JUCE_MEDIAFOUNDATION
  #include <evr.h>
 #endif

 #if JUCE_USE_CAMERA && JUCE_MSVC && ! JUCE_DONT_AUTOLINK_TO_WIN32_LIBRARIES
  #pragma comment (lib, "Strmiids.lib")
  #pragma comment (lib, "wmvcore.lib")
 #endif

 #if JUCE_MEDIAFOUNDATION && JUCE_MSVC && ! JUCE_DONT_AUTOLINK_TO_WIN32_LIBRARIES
  #pragma comment (lib, "mfuuid.lib")
 #endif

 #if JUCE_DIRECTSHOW && JUCE_MSVC && ! JUCE_DONT_AUTOLINK_TO_WIN32_LIBRARIES
  #pragma comment (lib, "strmiids.lib")
 #endif
#endif

//==============================================================================
using namespace juce;

BEGIN_JUCE_NAMESPACE

#if JUCE_MAC || JUCE_IOS
 #include "../juce_core/native/juce_osx_ObjCHelpers.h"
 #include "../juce_core/native/juce_mac_ObjCSuffix.h"

 #if JUCE_USE_CAMERA
  #include "native/juce_mac_CameraDevice.mm"
 #endif

 #if JUCE_QUICKTIME
  #include "native/juce_mac_QuickTimeMovieComponent.mm"
 #endif

#elif JUCE_WINDOWS
 #include "../juce_core/native/juce_win32_ComSmartPtr.h"

 #if JUCE_USE_CAMERA
  #include "native/juce_win32_CameraDevice.cpp"
 #endif

 #if JUCE_DIRECTSHOW
  #include "native/juce_win32_DirectShowComponent.cpp"
 #endif

 #if JUCE_QUICKTIME
  #include "native/juce_win32_QuickTimeMovieComponent.cpp"
 #endif

#elif JUCE_LINUX

#elif JUCE_ANDROID
 #if JUCE_USE_CAMERA
  #include "native/juce_android_CameraDevice.cpp"
 #endif
#endif

END_JUCE_NAMESPACE
