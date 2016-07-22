#ifndef FEATURES_GLOBAL_H
#define FEATURES_GLOBAL_H

#include <QtCore/qglobal.h>

#ifdef FEATURES_LIB
# define FEATURES_EXPORT Q_DECL_EXPORT
#else
# define FEATURES_EXPORT Q_DECL_IMPORT
#endif

#endif // FEATURES_GLOBAL_H
