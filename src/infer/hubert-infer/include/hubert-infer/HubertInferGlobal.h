/// @file HubertInferGlobal.h
/// @brief DLL export/import macro definitions for the hubert-infer library.

#pragma once
#ifdef _MSC_VER
#define HUBERT_INFER_DECL_EXPORT __declspec(dllexport)
#define HUBERT_INFER_DECL_IMPORT __declspec(dllimport)
#else
#define HUBERT_INFER_DECL_EXPORT __attribute__((visibility("default")))
#define HUBERT_INFER_DECL_IMPORT __attribute__((visibility("default")))
#endif

#ifndef HUBERT_INFER_EXPORT
#ifdef HUBERT_INFER_STATIC
#define HUBERT_INFER_EXPORT
#else
#ifdef HUBERT_INFER_LIBRARY
#define HUBERT_INFER_EXPORT HUBERT_INFER_DECL_EXPORT
#else
#define HUBERT_INFER_EXPORT HUBERT_INFER_DECL_IMPORT
#endif
#endif
#endif
