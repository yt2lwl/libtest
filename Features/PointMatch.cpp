#include "PointMatch.h"

#include <QTextStream>
#include <QFile>
#include <QVector>

namespace qm{
void writePointMatch_v0(QTextStream &aOutput, const QVector<PointMatch> &aList)
{
	const PointMatch *it = aList.data();
	size_t i = aList.size();
	while (i--)
	{
		const Pt2dr &p0 = it->first, &p1 = (*it++).second;
		aOutput << p0.x << ' ' << p0.y << ' ' << p1.x << ' ' << p1.y << endl;
	}
}

void writePointMatch_v1(QFile &aOutput, const QVector<PointMatch> &aList)
{

	const PointMatch *it = aList.data();
	//size_t i = aList.size();
	int i = (int)aList.size();
	/*aOutput.write((char*)&i, sizeof(size_t));*/
	aOutput.write((char*)&i, sizeof(int));
	while (i--)
	{
		const Pt2dr &p0 = it->first, &p1 = (*it++).second;
		aOutput.write((char *)&p0.x, sizeof(Real_));
		aOutput.write((char *)&p0.y, sizeof(Real_));
		aOutput.write((char *)&p1.x, sizeof(Real_));
		aOutput.write((char *)&p1.y, sizeof(Real_));
	}
	//aOutput.write((const char *)aList.data(), aList.size()*sizeof(Real_));
}

void readPointMatch_v0(QTextStream &aInput, QVector<PointMatch> &oVector)
{
	//QVector<PointMatch> readList;

	Real_ x0, y0, x1, y1;
	size_t nbMatches = 0;
	while (!(aInput>>x0).atEnd())
	{
		aInput >> y0 >> x1 >> y1;
		oVector.push_back(PointMatch(Pt2dr(x0, y0), Pt2dr(x1, y1)));
		nbMatches++;
	}

	//oVector.swap(readList);
}

void readPointMatch_v1(QFile &aInput, QVector<PointMatch> &oVector)
{
	//vector<PointMatch> readList;

	Real_ x0, y0, x1, y1;
	/*size_t nbMatches = 0;
	aInput.read((char *)&nbMatches, sizeof(size_t));*/
	int nbMatches = 0;
	aInput.read((char *)&nbMatches, sizeof(int));
	oVector.resize(nbMatches);
	auto it = oVector.begin();
	while (nbMatches--)
	{
		aInput.read((char *)&x0, sizeof(Real_));
		aInput.read((char *)&y0, sizeof(Real_));
		aInput.read((char *)&x1, sizeof(Real_));
		aInput.read((char *)&y1, sizeof(Real_));
		it->first.x = x0;
		it->first.y = y0;
		it->second.x = x1;
		(it++)->second.y = y1;
	}
}



bool WriteMatchesFile(const QString &aFilename, const QVector<PointMatch> &oVector, bool binary)
{
	if (binary)
	{
		QFile f(aFilename);
		if (!f.open(QIODevice::WriteOnly|QIODevice::Truncate))
			return false;
		writePointMatch_v1(f, oVector);
		f.close();
	}
	else
	{
		QFile file(aFilename);
		if (!file.open(QIODevice::WriteOnly|QIODevice::Truncate|QIODevice::Text))
			return false;
		QTextStream f(&file);
		writePointMatch_v0(f, oVector);
		file.close();
	}
	return true;
}

bool ReadMatchesFile(const QString &aFilename, QVector<PointMatch> &oVector, bool binary)
{
	if (binary)
	{
		QFile f(aFilename);
		if (!f.open(QIODevice::ReadOnly))
			return false;
		readPointMatch_v1(f, oVector);
		f.close();
	}
	else
	{
		QFile file(aFilename);
		if (!file.open(QIODevice::ReadOnly))
			return false;
		QTextStream f;
		readPointMatch_v0(f, oVector);
		file.close();
	}

	//readPointMatch_v1(f, reverseByteOrder, oVector); break;

	return true;
}

bool WriteIndexPair(const QString &aFilename, const QVector<V2I> &aMatches)
{
	QFile fout(aFilename);
	if (!fout.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return false;
	int count = (int)aMatches.size();
	fout.write((char*)&count, sizeof(int));
	auto itm = aMatches.begin();

	while (count--)
	{
		fout.write((char*)&(*itm++), sizeof(V2I));
	}
	fout.close();
	return true;
}
}
