#ifndef FEATURES_H
#define FEATURES_H

#include "features_global.h"
#include <QStringList>

namespace qm{
class FEATURES_EXPORT Features
{
public:
	Features();
	~Features();

private:
	QStringList image_list;
};
}


#endif // FEATURES_H
