#include "poshandle.h"
#include "PointData.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>

namespace qm{
PosHandle::PosHandle()
{
	pos_path_ = "";
}

PosHandle::~PosHandle()
{

}

void PosHandle::setPath(QString &path)
{
	pos_path_ = path;
}

void PosHandle::save()
{
	QFileInfo file_info(pos_path_);
	QDir dir = file_info.absoluteDir();
	dir.mkdir(dir.absolutePath() + "/Ori-Pos");
    for (auto bg = point_list_.begin(); bg != point_list_.end(); bg++)
	{
		QString xmlpath = dir.absolutePath() + "/Ori-Pos/Orientation-" + bg->id + ".xml";
		QFile xmlfile(xmlpath);
		if (!xmlfile.open(QFile::WriteOnly | QFile::Text|QFile::Truncate))
		{
			throw QString("can not create file:" + pos_path_);
		}
		QTextStream fout(&xmlfile);
		fout << "<? xml version = \"1.0\" ?>" << endl;
		fout << "<ExportAPERO>" << endl;
		fout << "<OrientationConique>" << endl;
		fout << "<TypeProj>eProjStenope</TypeProj>" << endl <<
			"<ZoneUtileInPixel>false</ZoneUtileInPixel>" << endl;
		//fout << "<FileInterne>Ori-NG/AutoCal_Foc-6000_Cam-GR_DIGITAL_3.xml</FileInterne>" << endl;
		fout << "<RelativeNameFI>true</RelativeNameFI>" << endl;
		fout << "<Externe>" << endl;
		fout << "<Time>0</Time>" << endl;
		fout << "<KnownConv>eConvApero_DistM2C</KnownConv>" << endl;

		//fout.setf(ios_base::fixed);
		//fout.precision(6);
		//fout.set
		//fout.setNumberFlags(fout.NumberFlag::ForcePoint);
		//fout.setRealNumberPrecision(12);
		fout.setRealNumberNotation(QTextStream::FixedNotation);
		fout << "<Centre>" << bg->x << " " << bg->y << " " << bg->z << "</Centre>" << endl;
		//fout << "<ParamRotation>"<<endl;
		//fout << "<CodageMatr>" << endl;
		/*fout << "<L1>-0.97766156803177073 0.20983641775811471 -0.012105212765634617</L1>" << endl;
		fout << "<L2>0.20792305008421669 0.95710641100321736 -0.20178038373493265</L2>" << endl;
		fout << "<L3>-0.030754896152249137 -0.19978987912049578 -0.97935598255367906</L3>" << endl;*/
		fout << "<VitesseFiable>true</VitesseFiable>" << endl;
		fout << "<ParamRotation>" << endl;
		fout << "<CodageAngulaire>" << bg->omega << " " << bg->phi << " " << bg->kappa << "</CodageAngulaire>" << endl;
		fout << "</ParamRotation>" << endl;
		//fout << "</CodageMatr>" << endl;
		//fout << "</ParamRotation>" << endl;
		fout << "</Externe>" << endl;
		fout << "<ConvOri>" << endl;
		fout << "<KnownConv>eConvApero_DistM2C</KnownConv>" << endl;
		fout << "</ConvOri>" << endl;
		fout /*<< bg->omega << " " << bg->phi << " " << bg->kappa*/ << "</OrientationConique>" << endl;
		fout << "</ExportAPERO>" << endl;

		//fout.unsetf(ios_base::fixed);
		//fout.precision(6);
		fout.flush();
		xmlfile.close();
	}
}

void PosHandle::read()
{
	QFile file(pos_path_);
	if (!file.open(QFile::ReadOnly | QFile::Text))
	{
		throw QString("can not open file:" + pos_path_);
	}
	QTextStream fin(&file);
	PointData pd;
	while (!(fin >> pd.id).atEnd())
	{
		fin >> pd.x >> pd.y >> pd.z
			>> pd.omega >> pd.phi >> pd.kappa;
		point_list_.push_back(pd);
	}
	file.close();
}
}
