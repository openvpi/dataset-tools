/// @file AudioUtilGlobal.h
/// @brief DLL export/import macro definitions for the audio-util library.

#ifndef AUDIO_UTIL_GLOBAL_H
#define AUDIO_UTIL_GLOBAL_H

#ifdef _MSC_VER
#  define AUDIO_UTIL_DECL_EXPORT __declspec(dllexport)  ///< Export symbol (MSVC).
#  define AUDIO_UTIL_DECL_IMPORT __declspec(dllimport)  ///< Import symbol (MSVC).
#else
#  define AUDIO_UTIL_DECL_EXPORT __attribute__((visibility("default")))
#  define AUDIO_UTIL_DECL_IMPORT __attribute__((visibility("default")))
#endif

#ifndef AUDIO_UTIL_EXPORT
#  ifdef AUDIO_UTIL_STATIC
#    define AUDIO_UTIL_EXPORT
#  else
#    ifdef AUDIO_UTIL_LIBRARY
#      define AUDIO_UTIL_EXPORT AUDIO_UTIL_DECL_EXPORT
#    else
#      define AUDIO_UTIL_EXPORT AUDIO_UTIL_DECL_IMPORT
#    endif
#  endif
#endif

#endif //AUDIO_UTIL_GLOBAL_H
