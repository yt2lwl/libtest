#pragma once


#include <QList>
#include <QPair>
namespace qm{
typedef double Real_;

class Pt2dr
{
public:
	Real_ x;
	Real_ y;
	Pt2dr(Real_ xx, Real_ yy)
	{
		x = xx;
		y = yy;
	}
	Pt2dr()
	{
		x = 0;
		y = 0;
	}
};

class V2I
{
public:
	size_t x, y;
	V2I(){}
	V2I(size_t xx, size_t yy) :x(xx), y(yy){}
};

typedef QPair<Pt2dr, Pt2dr> PointMatch;

// read/write a list of match points
bool WriteMatchesFile(const QString &aFilename, const QVector<PointMatch> &aList, bool binary = true); // use last version of the format and processor's byte order

bool ReadMatchesFile(const QString &aFilename, QVector<PointMatch> &oVector, bool binary = true);

bool WriteIndexPair(const QString &aFilename, const QVector<V2I> &aMatches);
}




