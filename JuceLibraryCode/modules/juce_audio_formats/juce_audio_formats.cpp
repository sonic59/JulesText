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

#ifdef __JUCE_AUDIO_FORMATS_JUCEHEADER__
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
#include "juce_audio_formats.h"

//==============================================================================
#if JUCE_MAC
 #define Point CarbonDummyPointName
 #define Component CarbonDummyCompName
 #import <QTKit/QTKit.h>
 #include <AudioToolbox/AudioToolbox.h>
 #undef Component
 #undef Point

#elif JUCE_IOS
 #import <AudioToolbox/AudioToolbox.h>
 #import <AVFoundation/AVFoundation.h>

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

//==============================================================================
#elif JUCE_ANDROID
 BEGIN_JUCE_NAMESPACE
 #include "../juce_core/native/juce_android_JNIHelpers.h"
 END_JUCE_NAMESPACE
 #undef JUCE_QUICKTIME
#endif

// START_AUTOINCLUDE format/*.cpp, sampler/*.cpp, codecs/*.cpp
#include "format/juce_AudioFormat.cpp"
#include "format/juce_AudioFormatManager.cpp"
#include "format/juce_AudioFormatReader.cpp"
#include "format/juce_AudioFormatReaderSource.cpp"
#include "format/juce_AudioFormatWriter.cpp"
#include "format/juce_AudioSubsectionReader.cpp"
#include "sampler/juce_Sampler.cpp"
#include "codecs/juce_AiffAudioFormat.cpp"
#include "codecs/juce_CoreAudioFormat.cpp"
#include "codecs/juce_FlacAudioFormat.cpp"
#include "codecs/juce_OggVorbisAudioFormat.cpp"
#include "codecs/juce_QuickTimeAudioFormat.cpp"
#include "codecs/juce_WavAudioFormat.cpp"
// END_AUTOINCLUDE
