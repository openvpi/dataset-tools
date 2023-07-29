#ifndef QSMEDIAGLOBAL_H
#define QSMEDIAGLOBAL_H

#include <QtGlobal>

#ifndef QSMEDIA_API
#    ifdef QSMEDIA_STATIC
#        define QSMEDIA_API
#    else
#        ifdef QSMEDIA_LIBRARY
#            define QSMEDIA_API Q_DECL_EXPORT
#        else
#            define QSMEDIA_API Q_DECL_IMPORT
#        endif
#    endif
#endif

#define QSAPI_BEGIN_NAMESPACE namespace QsApi {
#define QSAPI_END_NAMESPACE   }
#define QSAPI_USING_NAMESPACE using namespace QsApi;

#define QSMEDIA_BEGIN_NAMESPACE namespace QsMedia {
#define QSMEDIA_END_NAMESPACE   }

#endif // QSMEDIAGLOBAL_H
