#pragma once
#include <iostream>
#include <string>
class CPointData
{
public:
	std::string id;
	double x;
	double y;
	double z;
	double omega;
	double phi;
	double kappa;
	CPointData();
	~CPointData();
	friend std::ostream& operator <<(std::ostream &os, const CPointData &data);
	friend std::istream& operator>>(std::istream &is, CPointData &data);
};

