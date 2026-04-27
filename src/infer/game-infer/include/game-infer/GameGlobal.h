#ifndef GAMEGLOBAL_H
#define GAMEGLOBAL_H

#ifdef _MSC_VER
#define GAME_INFER_DECL_EXPORT __declspec(dllexport)
#define GAME_INFER_DECL_IMPORT __declspec(dllimport)
#else
#define GAME_INFER_DECL_EXPORT __attribute__((visibility("default")))
#define GAME_INFER_DECL_IMPORT __attribute__((visibility("default")))
#endif

#ifndef GAME_INFER_EXPORT
#ifdef GAME_INFER_STATIC
#define GAME_INFER_EXPORT
#else
#ifdef GAME_INFER_LIBRARY
#define GAME_INFER_EXPORT GAME_INFER_DECL_EXPORT
#else
#define GAME_INFER_EXPORT GAME_INFER_DECL_IMPORT
#endif
#endif
#endif

#endif // GAMEGLOBAL_H
