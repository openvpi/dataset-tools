#ifndef RMVPEGLOBAL_H
#define RMVPEGLOBAL_H

#ifdef _MSC_VER
#define RMVPE_INFER_DECL_EXPORT __declspec(dllexport)
#define RMVPE_INFER_DECL_IMPORT __declspec(dllimport)
#else
#define RMVPE_INFER_DECL_EXPORT __attribute__((visibility("default")))
#define RMVPE_INFER_DECL_IMPORT __attribute__((visibility("default")))
#endif

#ifndef RMVPE_INFER_EXPORT
#ifdef RMVPE_INFER_STATIC
#define RMVPE_INFER_EXPORT
#else
#ifdef RMVPE_INFER_LIBRARY
#define RMVPE_INFER_EXPORT RMVPE_INFER_DECL_EXPORT
#else
#define RMVPE_INFER_EXPORT RMVPE_INFER_DECL_IMPORT
#endif
#endif
#endif

#endif // RMVPEGLOBAL_H
