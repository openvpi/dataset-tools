#ifndef ASYNC_TASK_WINDOW_GLOBAL_H
#define ASYNC_TASK_WINDOW_GLOBAL_H

#ifdef _MSC_VER
#  define ASYNC_TASK_WINDOW_DECL_EXPORT __declspec(dllexport)
#  define ASYNC_TASK_WINDOW_DECL_IMPORT __declspec(dllimport)
#else
#  define ASYNC_TASK_WINDOW_DECL_EXPORT __attribute__((visibility("default")))
#  define ASYNC_TASK_WINDOW_DECL_IMPORT __attribute__((visibility("default")))
#endif

#ifndef ASYNC_TASK_WINDOW_EXPORT
#  ifdef ASYNC_TASK_WINDOW_STATIC
#    define ASYNC_TASK_WINDOW_EXPORT
#  else
#    ifdef ASYNC_TASK_WINDOW_LIBRARY
#      define ASYNC_TASK_WINDOW_EXPORT ASYNC_TASK_WINDOW_DECL_EXPORT
#    else
#      define ASYNC_TASK_WINDOW_EXPORT ASYNC_TASK_WINDOW_DECL_IMPORT
#    endif
#  endif
#endif

#endif //ASYNC_TASK_WINDOW_GLOBAL_H
