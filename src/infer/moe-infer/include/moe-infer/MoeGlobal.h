/// @file MoeGlobal.h
/// @brief DLL export/import macro definitions for the moe-infer library.

#pragma once
#ifdef _MSC_VER
#    define MOE_INFER_DECL_EXPORT __declspec(dllexport)
#    define MOE_INFER_DECL_IMPORT __declspec(dllimport)
#else
#    define MOE_INFER_DECL_EXPORT __attribute__((visibility("default")))
#    define MOE_INFER_DECL_IMPORT __attribute__((visibility("default")))
#endif

#ifndef MOE_INFER_EXPORT
#    ifdef MOE_INFER_STATIC
#        define MOE_INFER_EXPORT
#    else
#        ifdef MOE_INFER_LIBRARY
#            define MOE_INFER_EXPORT MOE_INFER_DECL_EXPORT
#        else
#            define MOE_INFER_EXPORT MOE_INFER_DECL_IMPORT
#        endif
#    endif
#endif
