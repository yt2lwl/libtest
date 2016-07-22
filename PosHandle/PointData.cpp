#include "PointData.h"
using namespace std;

CPointData::CPointData()
{
}


CPointData::~CPointData()
{
}

ostream &operator<<(ostream &os, const CPointData &data)
{
	os << data.id << " " << data.x << " " << data.y << " " << data.z <<
		" " << data.omega << " " << data.phi << " " << data.kappa << endl;
	return os;
}

istream &operator>>(istream &is, CPointData &data)
{
	is >> data.id >> data.x >> data.y >> data.z >> data.omega >> data.phi >> data.kappa;
	return is;
}