#ifndef SOMEGLOBAL_H
#define SOMEGLOBAL_H

#ifdef _MSC_VER
#define SOME_INFER_DECL_EXPORT __declspec(dllexport)
#define SOME_INFER_DECL_IMPORT __declspec(dllimport)
#else
#define SOME_INFER_DECL_EXPORT __attribute__((visibility("default")))
#define SOME_INFER_DECL_IMPORT __attribute__((visibility("default")))
#endif

#ifndef SOME_INFER_EXPORT
#ifdef SOME_INFER_STATIC
#define SOME_INFER_EXPORT
#else
#ifdef SOME_INFER_LIBRARY
#define SOME_INFER_EXPORT SOME_INFER_DECL_EXPORT
#else
#define SOME_INFER_EXPORT SOME_INFER_DECL_IMPORT
#endif
#endif
#endif

#endif // SOMEGLOBAL_H
