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

#ifndef __JUCE_MEMORY_JUCEHEADER__
#define __JUCE_MEMORY_JUCEHEADER__

//==============================================================================
#if JUCE_MSVC || DOXYGEN
 /** This is a compiler-independent way of declaring a variable as being thread-local.

     E.g.
     @code
     juce_ThreadLocal int myVariable;
     @endcode
 */
 #define juce_ThreadLocal    __declspec(thread)
#else
 #define juce_ThreadLocal    __thread
#endif

//==============================================================================
#if JUCE_MINGW
 /** This allocator is not defined in mingw gcc. */
 #define alloca              __builtin_alloca
#endif

//==============================================================================
/** Fills a block of memory with zeros. */
inline void zeromem (void* memory, size_t numBytes) noexcept        { memset (memory, 0, numBytes); }

/** Overwrites a structure or object with zeros. */
template <typename Type>
inline void zerostruct (Type& structure) noexcept                   { memset (&structure, 0, sizeof (structure)); }

/** Delete an object pointer, and sets the pointer to null.

    Remember that it's not good c++ practice to use delete directly - always try to use a ScopedPointer
    or other automatic lieftime-management system rather than resorting to deleting raw pointers!
*/
template <typename Type>
inline void deleteAndZero (Type& pointer)                           { delete pointer; pointer = nullptr; }

/** A handy function which adds a number of bytes to any type of pointer and returns the result.
    This can be useful to avoid casting pointers to a char* and back when you want to move them by
    a specific number of bytes,
*/
template <typename Type>
inline Type* addBytesToPointer (Type* pointer, int bytes) noexcept  { return (Type*) (((char*) pointer) + bytes); }

/** A handy function which returns the difference between any two pointers, in bytes.
    The address of the second pointer is subtracted from the first, and the difference in bytes is returned.
*/
template <typename Type1, typename Type2>
inline int getAddressDifference (Type1* pointer1, Type2* pointer2) noexcept  { return (int) (((const char*) pointer1) - (const char*) pointer2); }


//==============================================================================
/* In a Windows DLL build, we'll expose some malloc/free functions that live inside the DLL, and use these for
   allocating all the objects - that way all juce objects in the DLL and in the host will live in the same heap,
   avoiding problems when an object is created in one module and passed across to another where it is deleted.
   By piggy-backing on the JUCE_LEAK_DETECTOR macro, these allocators can be injected into most juce classes.
*/
#if JUCE_MSVC && defined (JUCE_DLL) && ! DOXYGEN
 extern JUCE_API void* juceDLL_malloc (size_t);
 extern JUCE_API void  juceDLL_free (void*);

 #define JUCE_LEAK_DETECTOR(OwnerClass)  public:\
              static void* operator new (size_t sz)           { return juce::juceDLL_malloc ((int) sz); } \
              static void* operator new (size_t, void* p)     { return p; } \
              static void operator delete (void* p)           { juce::juceDLL_free (p); } \
              static void operator delete (void*, void*)      {}
#endif

//==============================================================================
/** (Deprecated) This was a Windows-specific way of checking for object leaks - now please
    use the JUCE_LEAK_DETECTOR instead.
*/
#ifndef juce_UseDebuggingNewOperator
 #define juce_UseDebuggingNewOperator
#endif


#endif   // __JUCE_MEMORY_JUCEHEADER__
