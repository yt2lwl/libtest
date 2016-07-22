#ifndef POSHANDLE_GLOBAL_H
#define POSHANDLE_GLOBAL_H

#include <QtCore/qglobal.h>

#ifdef POSHANDLE_LIB
# define POSHANDLE_EXPORT Q_DECL_EXPORT
#else
# define POSHANDLE_EXPORT Q_DECL_IMPORT
#endif

#endif // POSHANDLE_GLOBAL_H
