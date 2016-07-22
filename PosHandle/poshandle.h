#ifndef POSHANDLE_H
#define POSHANDLE_H

#include "poshandle_global.h"
#include <QList>
#include <QString>

namespace qm{
//pos data
struct PointData
{
	QString id;
	double x;
	double y;
	double z;
	double omega;
	double phi;
	double kappa;
};
//transform pos.txt to xml
class POSHANDLE_EXPORT PosHandle
{
public:
	PosHandle();
	~PosHandle();
	void setPath(QString &path);//set path
	void read();//read pos file
	void save();//save to xml
private:
	QString pos_path_;//pos path
	QList<PointData> point_list_;//point list
};
}


#endif // POSHANDLE_H
