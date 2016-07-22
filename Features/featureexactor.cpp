#include "featureexactor.h"

#include <QVector>
#include <QString>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDomDocument>
#include <QFileInfo>
#include <QDirIterator>
#include <QDebug>
#include <QGL>


#include "opencv2/opencv.hpp"
#include "opencv2/cudafeatures2d.hpp"
#include "siftgpumodule.h"
#include "SiftGPU.h"
#include "methods.h"
#include "PointMatch.h"
#include "roiwindow.h"

#define SIFT_DESCRIPTOR_SIZE 128

namespace qm{
typedef unsigned char PixReal;
struct SiftFeature
{
	QVector<SiftKeypoint> keys;
	QVector<float> descriptors;
};


//load an image
void LoadImage(const QString &file, cv::Mat &img)
{
	img = cv::imread(file.toStdString(), 0);
}

//get feature file save name
inline QString GetFeatureSaveName(const QString &file)
{
	QFileInfo file_info(file);
	QString dir = file_info.absolutePath();
	QString file_name = file_info.fileName();
	//string dir = yt::getDir(file.toStdString());
	//string file_name=yt::getFileName(file.toStdString());
	//split_dir_file(file, dir, fname);
	dir += "/features";
	QDir qdir(dir);
	if (!qdir.exists())
		qdir.mkdir(dir);
	dir += "/";
	file_name.insert(0, dir);
	file_name += ".dat";
	return file_name;
}

//write sift point
bool WriteSiftPointList(const QString &o_filename, QVector<SiftKeypoint> &keyPoints, QVector<float> &describe)
{
	QFile f(o_filename);
	//ofstream f(o_filename.c_str(), ios::binary);
	if (!f.open(QIODevice::WriteOnly)) return false;
	size_t nbPoints = keyPoints.size(),
		dimension = SIFT_DESCRIPTOR_SIZE;
	f.write((char*)&nbPoints, sizeof(size_t));
	f.write((char*)&dimension, sizeof(size_t));
	float * pd = &(describe[0]);
	for (int i = 0; i < nbPoints; i++, pd += 128)
	{
		f.write((char*)(&keyPoints[i].x), sizeof(float));
		f.write((char*)(&keyPoints[i].y), sizeof(float));
		f.write((char*)(&keyPoints[i].s), sizeof(float));
		f.write((char*)(&keyPoints[i].o), sizeof(float));
		float uchar_desc[SIFT_DESCRIPTOR_SIZE];
		int j = SIFT_DESCRIPTOR_SIZE; const float *itReal = pd; float *it_uchar = uchar_desc;
		while (j--)
			//(*it_uchar++) = (unsigned char)(512 * (*itReal++));
			(*it_uchar++) = *itReal++;
		f.write((char*)uchar_desc, SIFT_DESCRIPTOR_SIZE * sizeof(float));
		//f.write((char*)(pd), 128 * sizeof(float));
	}
	f.close();
	return true;
}

bool WriteSiftPointList1(const QString &o_filename, SiftFeature &sf)
{
	return WriteSiftPointList(o_filename, sf.keys, sf.descriptors);
}

void clusterize_1d(int i_areaSize, int i_efficientSize, int i_overlap, QVector<RoiWindow_1d> &o_cluster)
{
#ifdef _DEBUG
	if (i_efficientSize <= i_overlap)
		cerr << "WARN: clusterize_1d: efficientSize=" << i_efficientSize << " <= overlap=" << i_overlap << endl;
#endif

	if (i_areaSize <= i_efficientSize)
	{
		o_cluster.clear();
		o_cluster.push_back(RoiWindow_1d(0, i_areaSize - 1, 0, i_areaSize - 1));
		return;
	}
	int nbFullLength = i_areaSize / i_efficientSize,
		remaining = i_areaSize%i_efficientSize;
	int nbWindows = (remaining == 0 ? nbFullLength : nbFullLength + 1);
	o_cluster.resize(nbWindows);
	RoiWindow_1d *itROI = o_cluster.data();
	// first window
	int i = std::min(i_efficientSize + i_overlap, i_areaSize); // clip overlaping zone on the right if needed
	(itROI++)->set(0, i - 1, 0, i_efficientSize - 1);
	// middle windows (with overlapping on each side)
	int x0 = i_efficientSize - i_overlap;
	if (nbWindows > 2)
	{
		int x1 = x0 + i_efficientSize + 2 * i_overlap - 1,
			roi_x1 = i_overlap + i_efficientSize - 1;
		i = nbWindows - 2;
		while (i--)
		{
			(itROI++)->set(x0, x1, i_overlap, roi_x1);
			x0 += i_efficientSize; x1 += i_efficientSize;
		}
		// the last middle window's right overlapping zone might out-range the area, clip it then
		if (remaining < i_overlap)
			itROI[-1].m_x1 = i_areaSize - 1;
	}
	// last window
	(itROI++)->set(x0, i_areaSize - 1, i_overlap, i_overlap + remaining - 1);
}


//
void clusterize_2d(const cv::Size &i_areaSize, const cv::Size &i_efficientSize, const cv::Size &i_overlap, QVector<RoiWindow_2d> &o_cluster)
{
	QVector<RoiWindow_1d> clusterX, clusterY;
	clusterize_1d(i_areaSize.width, i_efficientSize.width, i_overlap.width, clusterX);
	clusterize_1d(i_areaSize.height, i_efficientSize.height, i_overlap.height, clusterY);

	o_cluster.resize(clusterX.size()*clusterY.size());
	QVector<RoiWindow_2d>::iterator it2d = o_cluster.begin();
	QVector<RoiWindow_1d>::iterator itX = clusterX.begin(),
		itY;
	int x = (int)clusterX.size(),
		y;
	while (x--)
	{
		itY = clusterY.begin();
		y = (int)clusterY.size();
		while (y--)
		{
			it2d->set_along_y(*itY++);
			(it2d++)->set_along_x(*itX);
		}
		itX++;
	}
}

// retrieve a sub part of an image
void getWindow(const cv::Mat &i_image, const RoiWindow_2d &i_window, cv::Mat &o_image)
{
	int iwidth = i_image.cols,
		iheight = i_image.rows;

#ifdef _DEBUG
	if (i_window.m_x0 < 0 ||
		i_window.m_y0 < 0 ||
		i_window.m_x1 >= i_image.cols ||
		i_window.m_y1 >= i_image.rows)
		cerr << "ERRROR: getWindow(): window out of image's range" << endl;
	if (i_window.m_x0 >= i_window.m_x1 ||
		i_window.m_y0 >= i_window.m_y1)
		cerr << "ERROR: getWindow(): inconsistent window " << i_window.m_x0 << ',' << i_window.m_y0 << ' '
		<< i_window.m_x1 << ',' << i_window.m_y1 << endl;
#endif
	cv::Size size(i_window.m_x1 - i_window.m_x0 + 1, i_window.m_y1 - i_window.m_y0 + 1);
	o_image.create(size, i_image.depth());
	int owidth = o_image.cols,
		oheight = o_image.rows;
	//o_image.resize();
	const PixReal *itSrc = i_image.data + i_window.m_y0*iwidth + i_window.m_x0;
	PixReal *itDst = o_image.data;
	int j = oheight,
		srcOffset = iwidth - owidth,
		i;
	while (j--)
	{
		i = owidth;
		while (i--)
			(*itDst++) = (*itSrc++);
		itSrc += srcOffset;
	}
}


bool ReadSiftPointList(const QString &i_filename, QVector<SiftKeypoint> &keyPoints, QVector<float> &descriptor)
{
	//ifstream f(i_filename.c_str(), ios::binary);
	QFile f(i_filename);
	if (!f.open(QIODevice::ReadOnly)) return false;

	size_t nbPoints,
		dimension;
	f.read((char*)&nbPoints, sizeof(size_t));
	f.read((char*)&dimension, sizeof(size_t));

	keyPoints.resize(nbPoints);
	descriptor.resize(nbPoints*SIFT_DESCRIPTOR_SIZE);
	//float * pk = &(keyPoints[0]);

	float * pd = &(descriptor[0]);
	float *itReal = descriptor.data();
	for (int i = 0; i < nbPoints; i++, pd += 128)
	{
		f.read((char*)(&keyPoints[i].x), sizeof(float));
		f.read((char*)(&keyPoints[i].y), sizeof(float));
		f.read((char*)(&keyPoints[i].s), sizeof(float));
		f.read((char*)(&keyPoints[i].o), sizeof(float));

		float uchar_desc[SIFT_DESCRIPTOR_SIZE];
		f.read((char*)uchar_desc, SIFT_DESCRIPTOR_SIZE * sizeof(float));
		int j = SIFT_DESCRIPTOR_SIZE;  float *it_uchar = uchar_desc;
		while (j--) (*itReal++) = *it_uchar++;
	}
	f.close();
	return true;
}


//利用RANSAC进行误匹配点剔除
bool ransac(QVector<PointMatch> &vecMatch, QVector<V2I> &vecIndex)
{
	using namespace cv;
	vector<cv::Point2f > ps1, ps2;
	for (auto bg = vecMatch.begin(); bg != vecMatch.end(); ++bg)
	{
		ps1.push_back(cv::Point2f((float)bg->first.x, (float)bg->first.y));
		ps2.push_back(cv::Point2f((float)bg->second.x, (float)bg->second.y));
	}

	vector<DMatch> matches;
	for (auto bg = vecIndex.begin(); bg != vecIndex.end(); ++bg)
	{
		matches.push_back(DMatch((int)bg->x, (int)bg->y, 0));
	}

	cv::Mat status((int)vecMatch.size(), 1, CV_8UC1);
	cv::Mat fmat = cv::findFundamentalMat(ps1, ps2, status);

	int cnt = 0;

	//vector<KeyPoint> nk1, nk2;
	//vector<DMatch> nm;
	QVector<PointMatch> nms;
	QVector<V2I> nis;
	for (size_t i = 0; i < vecMatch.size(); ++i)
	{
		if (status.at<uchar>((int)i, 0) != 0)
		{
			nms.push_back(vecMatch[i]);
			nis.push_back(vecIndex[i]);
		}
	}
	vecMatch.swap(nms);
	vecIndex.swap(nis);
	return true;
}

//FeatureExactor::FeatureExactor(QObject *parent)
//	: QObject(parent)
//{
//	
//}

FeatureExactor::FeatureExactor()
{
	gpu_module_ = new SiftGpuModule(nullptr);
}

FeatureExactor::~FeatureExactor()
{
	if (!gpu_module_)
	{
		delete gpu_module_;
		gpu_module_ = nullptr;
	}
}

bool FeatureExactor::featureExactSave(int chunk_size, size_t subcnt, bool buildRelation)
{
#pragma region 特征提取及保存
	using namespace cv;
	const size_t bat_count = 64;
	vector<SiftFeature> vecFea(image_path_list_.size());//保存采样后特征
	size_t fileIndex = 0;//索引
	size_t cnt = image_path_list_.size() / bat_count;//将所有影像分成100张每份
	size_t rm = image_path_list_.size() - cnt * bat_count;
	for (size_t i = 0; i < cnt; ++i)
	{
		vector<Mat> vecImgs(bat_count);
		vector<SiftFeature> vecFeas(bat_count);
		QVector<QString> vecFile(bat_count);
		for (size_t j = 0; j < bat_count; ++j)
		{
			vecFile[j] = image_path_list_.at(i * bat_count + j);
		}
		yt::multiCall(LoadImage, vecFile.toStdVector(), vecImgs);//多线程载入影像

		if (buildRelation)//需要建立关系时的特征提取
		{
			QVector<Mat> downImgs(bat_count);
			auto it = vecImgs.begin();
			for (size_t j = 0; j < bat_count; ++j)
			{
				double scale = max(vecImgs[j].cols, vecImgs[j].rows) / 1024.0;
				Size newsize(vecImgs[j].cols, vecImgs[j].rows);
				if (scale > 1)
				{
					Size newsize((int)(vecImgs[j].cols / scale), (int)(vecImgs[j].rows / scale));
					resize(vecImgs[j], downImgs[j], newsize);
				}
				else
				{
					downImgs[j] = vecImgs[j];
				}
				featureExact(downImgs[j].data, downImgs[j].cols, downImgs[j].rows, 1200, 800, vecFea[fileIndex++]);
			}

		}
		auto it = vecFeas.begin();
		for (auto bg = vecImgs.begin(); bg != vecImgs.end(); ++bg)
		{
			if (!featureExact(bg->data, bg->cols, bg->rows, chunk_size, subcnt, *it++))//对单张影像进行特征提取
				return false;
		}
		for_each(vecFile.begin(), vecFile.end(), [](QString &s){s = GetFeatureSaveName(s); });
		yt::multiCall(WriteSiftPointList1, vecFile.toStdVector(), vecFeas);//多线程保存结果
	}
	//处理剩下的影像
	vector<cv::Mat> vecImgs(rm);
	vector<SiftFeature> vecFeas(rm);
	QVector<QString> vecFile(rm);
	for (size_t i = 0; i < rm;++i)
	{
		vecFile[i] = image_path_list_[bat_count * cnt + i];
	}
	yt::multiCall(LoadImage, vecFile.toStdVector(), vecImgs);

	if (buildRelation)
	{
		QVector<Mat> downImgs(vecFile.size());
		auto it = vecImgs.begin();
		for (size_t i = 0; i < vecFile.size(); ++i)
		{
			double scale = max(vecImgs[i].cols, vecImgs[i].rows) / 1024.0;
			Size newsize(vecImgs[i].cols, vecImgs[i].rows);
			if (scale > 1)
			{
				Size newsize((int)(vecImgs[i].cols / scale), (int)(vecImgs[i].rows / scale));
				resize(vecImgs[i], downImgs[i], newsize);
			}
			else
			{
				downImgs[i] = vecImgs[i];
			}
			featureExact(downImgs[i].data, downImgs[i].cols, downImgs[i].rows, 1200, 800, vecFea[fileIndex++]);
		}

	}
	auto it = vecFeas.begin();
	for (auto bg = vecImgs.begin(); bg != vecImgs.end(); ++bg)
	{
		if (!featureExact(bg->data, bg->cols, bg->rows, chunk_size, subcnt, *it++))
			return false;
	}

	for_each(vecFile.begin(), vecFile.end(), [](QString &s){s = GetFeatureSaveName(s); });
	yt::multiCall(WriteSiftPointList1, vecFile.toStdVector(), vecFeas);
#pragma endregion


	//for_each(vecImgs.begin(), vecImgs.end(), [](Mat &img){img.release(); });
	//建立影像之间的关系，resize后匹配点大于则认为是像对
	if (buildRelation)
	{
		size_t less_cnt = 8;
		typedef QPair<QString, QString> Pair;
		QVector<Pair> vecPair;
		QVector<PointMatch> vecPm;
		QVector<V2I> vecIndex;
		for (size_t i = 0; i < image_path_list_.size() - 1; ++i)
		{
			for (size_t j = i + 1; j < image_path_list_.size(); ++j)
			{
				featureMatch(qMakePair(vecFea[i], vecFea[j]), vecPm, vecIndex);
				if (vecPm.size() > less_cnt)
					vecPair.push_back(qMakePair(
					QFileInfo(image_path_list_[i]).fileName(), 
					QFileInfo(image_path_list_[j]).fileName()));
			}
		}

		QString file_path(QFileInfo(image_path_list_[0]).absolutePath() + "/GrapheHom.xml");
		QFile file(file_path);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text|QIODevice::Truncate))
		{
			throw new QString("can not create file:" + file.fileName());
		}
		QDomDocument doc;
		auto instruction=doc.createProcessingInstruction("xml", "version=\"1.0\"");
		doc.appendChild(instruction);
		QDomElement root = doc.createElement("SauvegardeNamedRel");
		doc.appendChild(root);
		//ofstream fout(yt::getDir(image_path_list_[0].toStdString()) + "\\GrapheHom.xml");
		QVector<QDomElement> cples(vecPair.size());
		QVector<QDomText> texts(vecPair.size());
		for (size_t i = 0; i < vecPair.size(); ++i)
		{
			cples[i] = doc.createElement("Cple");
			QString text(vecPair[i].first);
			text.append(" ");
			text.append(vecPair[i].second);
			texts[i] = doc.createTextNode(text);
			cples[i].appendChild(texts[i]);
			root.appendChild(cples[i]);
			//fout << "<Cple>" << vecPair[i].first << " " << vecPair[i].second << "</Cple>" << endl;
		}
		QTextStream fout(&file);
		doc.save(fout, 4, doc.EncodingFromTextStream);
		file.close();
	}
	return true;
}

bool FeatureExactor::featureExact(unsigned char *data, size_t width, size_t height,
	size_t chunk_size, size_t subcnt, SiftFeature &features)
{
	//static size_t i = 0;
	//i++; subcnt += i;
	if (gpu_module_->Reload(subcnt))
	{
		SiftGPU *sift = gpu_module_->GetSiftExactor();
		if (sift->CreateContextGL() != SiftGPU::SIFTGPU_FULL_SUPPORTED) return 0;
		//sift->VerifyContextGL();
		cv::Mat img((int)height, (int)width, CV_8UC1, data);
		if (img.data == nullptr)
			return false;
		QVector<SiftKeypoint> &siftPoints = features.keys;
		QVector<float> &descriptors = features.descriptors;
		if (chunk_size > 0)
		{
			sift->SetMaxDimension((int)chunk_size);

			QVector<SiftKeypoint> siftPoints_sub;
			QVector<float> descriptors_sub;

			// split image into sub area
			QVector<RoiWindow_2d> grid;

			clusterize_2d(cv::Size(img.cols, img.rows),
				cv::Size((int)chunk_size, (int)chunk_size),
				cv::Size(0, 0), // no overlap
				grid);
			// process all sub-images

			int iWin = (int)grid.size();
			QVector<RoiWindow_2d>::iterator itWindow = grid.begin();
			while (iWin--)
			{
				cv::Mat subimage;
				getWindow(img, *itWindow, subimage);
				siftPoints_sub.clear();
				sift->RunSIFT(subimage.cols, subimage.rows, subimage.data, GL_LUMINANCE, GL_UNSIGNED_BYTE);
				int iPoint = sift->GetFeatureNum();
				siftPoints_sub.resize(iPoint);
				descriptors_sub.resize(128 * iPoint);
				sift->GetFeatureVector(&siftPoints_sub[0], &descriptors_sub[0]);//(int)siftPoints_sub.size();
				// shift points into orignal image's coordinate system
				// and add points to the main list
				QVector<SiftKeypoint>::iterator itPoint;
				QVector<float>::iterator itDescriptor;
				itPoint = siftPoints_sub.begin();
				itDescriptor = descriptors_sub.begin();
				while (iPoint--)
				{
					itPoint->x += itWindow->m_x0;
					itPoint->y += itWindow->m_y0;
					siftPoints.push_back(*itPoint++);
					for (size_t i = 0; i < 128; ++i)
					{
						descriptors.push_back(*itDescriptor++);
					}
				}

				itWindow++;
			}
		}
		else
		{
			sift->SetMaxDimension(max(img.rows, img.cols));
			sift->RunSIFT(img.cols, img.rows, img.data, GL_LUMINANCE, GL_UNSIGNED_BYTE);
			int iPoint = iPoint = sift->GetFeatureNum();
			siftPoints.resize(iPoint);
			//sift->RunSIFT()
			//allocate memory
			descriptors.resize(128 * iPoint);

			//reading back feature vectors is faster than writing files
			//if you dont need keys or descriptors, just put NULLs here
			sift->GetFeatureVector(&siftPoints[0], &descriptors[0]);
			// process full length
			// process one image per file
		}
		return true;
	}
	return false;
}


bool FeatureExactor::featureMatch(const QPair<SiftFeature, SiftFeature> &pairImg,
	QVector<PointMatch> &vecMatch, QVector<V2I> &vecIndex)
{
	vecIndex.clear();
	vecMatch.clear();
	const QVector<SiftKeypoint> &keys1 = pairImg.first.keys,
		&keys2 = pairImg.second.keys;
	const QVector<float> &d1 = pairImg.first.descriptors,
		&d2 = pairImg.second.descriptors;
	if (gpu_module_->IsInit() || gpu_module_->Init())
	{
		SiftMatchGPU *matcher = gpu_module_->GetSiftMatcher();
		//**********************GPU SIFT MATCHING*********************************
		//**************************select shader language*************************
		//SiftMatchGPU will use the same shader lanaguage as SiftGPU by default
		//Before initialization, you can choose between glsl, and CUDA(if compiled). 
		//matcher->SetLanguage(SiftMatchGPU::SIFTMATCH_GLSL); // +i for the (i+1)-th device

		//Verify current OpenGL Context and initialize the Matcher;
		//If you don't have an OpenGL Context, call 
		matcher->CreateContextGL();// instead;
		//matcher->VerifyContextGL(); //must call once

		   //Set descriptors to match, the first argument must be either 0 or 1
		   //if you want to use more than 4096 or less than 4096
		   //call matcher->SetMaxSift() to change the limit before calling setdescriptor
#pragma region match
		int num1 = (int)keys1.size(),
			num2 = (int)keys2.size();
		int max_num = max(num1, num2);
		int max_size = 10240 > max_num ? max_num + 1 : 10240;
		matcher->SetMaxSift(max_size);
		int sub1 = num1 / max_size;
		int sub2 = num2 / max_size;
		//matcher->SetDescriptors(0, num1, &d1[0]); //image 1
		//matcher->SetDescriptors(1, num2, &d2[0]); //image 2
		//vector<V2I> vecIndex;
		//vector<PointMatch> vecMatch;
		int(*mbuffer)[2] = new int[max_size][2];
		for (int k = 0; k < sub1; ++k)
		{
			matcher->SetDescriptors(0, max_size, &d1[k * max_size * 128]);
			for (int i = 0; i < sub2; ++i)
			{
				matcher->SetDescriptors(1, max_size, &d2[i * max_size * 128]);

				int match_cnt = matcher->GetSiftMatch(max_size, mbuffer);
				for (int j = 0; j < match_cnt; ++j)
				{
					//How to get the feature matches: 
					const SiftGPU::SiftKeypoint & key1 = keys1[mbuffer[j][0] + max_size*k];
					const SiftGPU::SiftKeypoint & key2 = keys2[mbuffer[j][1] + max_size * i];
					vecIndex.push_back(V2I(mbuffer[j][0] + max_size*k, mbuffer[j][1] + max_size * i));
					Pt2dr p1(key1.x, key1.y),
						p2(key2.x, key2.y);
					vecMatch.push_back(qMakePair(p1, p2));
					//key1 in the first image matches with key2 in the second image
				}
			}
			matcher->SetDescriptors(1, num2 - sub2*max_size, &d2[sub2*max_size]);
			int match_cnt = matcher->GetSiftMatch(num2 - sub2*max_size, mbuffer);
			for (int j = 0; j < match_cnt; ++j)
			{
				//How to get the feature matches: 
				const SiftGPU::SiftKeypoint & key1 = keys1[mbuffer[j][0] + max_size*k];
				const SiftGPU::SiftKeypoint & key2 = keys2[mbuffer[j][1] + max_size * sub2];
				vecIndex.push_back(V2I(mbuffer[j][0] + max_size*k, mbuffer[j][1] + max_size * sub2));
				Pt2dr p1(key1.x, key1.y),
					p2(key2.x, key2.y);
				vecMatch.push_back(qMakePair(p1, p2));
				//key1 in the first image matches with key2 in the second image
			}
		}

		matcher->SetDescriptors(0, num1 - max_size*sub1, &d1[max_size*sub1]);
		for (int i = 0; i < sub2; ++i)
		{
			matcher->SetDescriptors(1, max_size, &d2[i * max_size * 128]);

			int match_cnt = matcher->GetSiftMatch(num1 - max_size*sub1, mbuffer);
			for (int j = 0; j < match_cnt; ++j)
			{
				//How to get the feature matches: 
				const SiftGPU::SiftKeypoint & key1 = keys1[mbuffer[j][0] + max_size*sub1];
				const SiftGPU::SiftKeypoint & key2 = keys2[mbuffer[j][1] + max_size * i];
				vecIndex.push_back(V2I(mbuffer[j][0] + max_size*sub1, mbuffer[j][1] + max_size * i));
				Pt2dr p1(key1.x, key1.y),
					p2(key2.x, key2.y);
				vecMatch.push_back(qMakePair(p1, p2));
				//key1 in the first image matches with key2 in the second image
			}
		}
		matcher->SetDescriptors(1, num2 - sub2*max_size, &d2[sub2*max_size]);
		int match_cnt = matcher->GetSiftMatch(min(num1 - sub1*max_size, num2 - sub2*max_size), mbuffer);
		for (int j = 0; j < match_cnt; ++j)
		{
			//How to get the feature matches: 
			const SiftGPU::SiftKeypoint & key1 = keys1[mbuffer[j][0] + max_size*sub1];
			const SiftGPU::SiftKeypoint & key2 = keys2[mbuffer[j][1] + max_size * sub2];
			vecIndex.push_back(V2I(mbuffer[j][0] + max_size*sub1, mbuffer[j][1] + max_size * sub2));
			Pt2dr p1(key1.x, key1.y),
				p2(key2.x, key2.y);
			vecMatch.push_back(qMakePair(p1, p2));
			//key1 in the first image matches with key2 in the second image
		}
		delete[]mbuffer;
#pragma endregion

		return true;
	}
	else
	{
		return false;
	}
}


//bool ReadKeys(QString &i_filename, QVector<cv::KeyPoint> &keyPoints, cv::Mat &descriptor)
//{
//	QFile f(i_filename);
//	if (!f.open(QIODevice::ReadOnly)) return false;
//	size_t nbPoints,//= keyPoints.size(),
//		dimension;//= descriptor.size() / keyPoints.size();
//	QVector<float> desc;
//	f.read((char*)&nbPoints, sizeof(size_t));
//	f.read((char*)&dimension, sizeof(size_t));
//	keyPoints.resize(nbPoints);
//	desc.resize(nbPoints*dimension);
//	float * pd = &(desc[0]);
//	for (int i = 0; i < nbPoints; i++, pd += dimension)
//	{
//		f.read((char*)(&keyPoints[i].pt.x), sizeof(float));
//		f.read((char*)(&keyPoints[i].pt.y), sizeof(float));
//		f.read((char*)(&keyPoints[i].size), sizeof(float));
//		f.read((char*)(&keyPoints[i].angle), sizeof(float));
//		//float uchar_desc
//		//int j = SIFT_DESCRIPTOR_SIZE; const float *itReal = pd; float *it_uchar = uchar_desc;
//		//while (j--)
//		//	//(*it_uchar++) = (unsigned char)(512 * (*itReal++));
//		//	(*it_uchar++) = *itReal++;
//		f.read((char*)pd, dimension * sizeof(float));
//		//f.write((char*)(pd), 128 * sizeof(float));
//	}
//	f.close();
//	//cv::Mat t((int)nbPoints, (int)dimension, CV_32FC1, desc.data());
//	//descriptor.create(t.rows,t.cols, t.type());
//	//t.copyTo(descriptor);
//	descriptor.create((int)nbPoints, (int)dimension, CV_32FC1);
//	copy(desc.begin(), desc.end(), (float *)descriptor.data);
//	return true;
//}
void featureMatchCV(const QPair<SiftFeature, SiftFeature> &pairImg,
	QVector<PointMatch> &vecMatch, QVector<V2I> &vecIndex)
{
	using namespace cv;
	vecIndex.clear();
	vecMatch.clear();
	const QVector<SiftKeypoint> &keys1 = pairImg.first.keys,
		&keys2 = pairImg.second.keys;
	const QVector<float> &d1 = pairImg.first.descriptors,
		&d2 = pairImg.second.descriptors;
	Mat mat_d1(keys1.size(), SIFT_DESCRIPTOR_SIZE, CV_32FC1, (void *)d1.data());
	Mat mat_d2(keys2.size(), SIFT_DESCRIPTOR_SIZE, CV_32FC1, (void *)d2.data());
	cuda::GpuMat gpu_d1,gpu_d2;
	gpu_d1.upload(mat_d1);
	gpu_d2.upload(mat_d2);
	vector<vector<cv::DMatch>> vecsMatch;
	QVector<DMatch> matches;
	auto matcher = cv::cuda::DescriptorMatcher::createBFMatcher();
	matcher->knnMatch(gpu_d1,gpu_d2,vecsMatch,2);
	for (auto bg = vecsMatch.begin(); bg != vecsMatch.end(); ++bg)
	{
		if (bg->at(0).distance < 0.6*(bg->at(1).distance))
			matches.push_back(bg->at(0));
	}
	
	for (size_t i = 0; i < matches.size(); ++i)
	{
		Pt2dr p1,p2;
		p1.x = keys1[matches[i].queryIdx].x;
		p2.x = keys2[matches[i].trainIdx].x;
		p1.y = keys1[matches[i].queryIdx].y;
		p2.y = keys2[matches[i].trainIdx].y;
		vecMatch.push_back(qMakePair(p1, p2));
		vecIndex.push_back(V2I(matches[i].queryIdx, matches[i].trainIdx));
	}
}

bool FeatureExactor::featureMatchSave()
{
	
	QString dir = QFileInfo(image_path_list_[0]).absolutePath();
	QDomDocument doc;
	QFile pair_xml(dir + "/GrapheHom.xml");
	if (!pair_xml.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		throw QString("can not open file:" + pair_xml.fileName());
	}
	if (!doc.setContent(&pair_xml))
		throw QString("can not open xml file:" + pair_xml.fileName());
	QDomNodeList pair_node = doc.documentElement().elementsByTagName("Cple");
	QVector<QPair<QString, QString>> image_pair_list;
	for (int i = 0; i < pair_node.size();++i)
	{
		QString &pair_text = pair_node.at(i).toElement().text();
		QStringList pair_text_2 = pair_text.split(' ');
		if (pair_text_2.size() != 2)
			throw QString("error pair xml file!");
		image_pair_list.push_back(qMakePair(dir + "/features/" + pair_text_2.at(0) + ".dat",
											dir + "/features/" + pair_text_2.at(1) + ".dat"));
	}
	return featureMatchSave(image_pair_list);
}

bool FeatureExactor::featureMatchSave(const QVector<QPair<QString, QString>> &pairs_image, bool ransca /* = true */)
{
	for (auto bg = pairs_image.begin(); bg != pairs_image.end(); ++bg)
	{
		if (!featureMatchSaves(*bg, ransca))
			return false;
	}
	return true;
}

bool FeatureExactor::featureMatchSaves(const QPair<QString, QString> &a_pair, bool ransca /* = true */)
{
	const QString &f1 = a_pair.first;
	const QString &f2 = a_pair.second;
	QVector<SiftKeypoint> keys1, keys2;
	QVector<float> d1, d2;
	ReadSiftPointList(f1, keys1, d1);
	ReadSiftPointList(f2, keys2, d2);

	SiftFeature sf1{ keys1, d1 }, sf2{ keys2, d2 };
	QVector<PointMatch> vecMatch;
	QVector<V2I> vecIndex;
	featureMatch(qMakePair(sf1, sf2), vecMatch, vecIndex);



	if (vecMatch.size() < 50)//小于50个点，认为不稳定
		return true;
	if (ransca)
	{
		//size_t total = vecMatch.size();
		ransac(vecMatch, vecIndex);
		if (vecMatch.size() < 20)//小于20个点，认为不稳定
			return true;
		//size_t remain = vecMatch.size();
		//cout << "准确率:" << double(remain) / total * 100 << "%" << endl;
		ransac(vecMatch, vecIndex);
		if (vecMatch.size() < 20)//小于20个点，认为不稳定
			return true;
	}

	QFileInfo f2_file(f2);
	QFileInfo f1_file(f1);

	QString f11 = f1_file.absolutePath() + "/" + f1_file.completeBaseName();
	QString f12(f2_file.completeBaseName());
	QDir().mkdir(f11);
	//_mkdir(f11.c_str());
	//erase_extention(f12);
	f11 += "/";
	f11 += f12;
	f11 += ".result";
	//cv::drawMatches(img1, k1, img2, k2, matches, img3);
	//imwrite("im3.jpg", img3);


	//draw_matchs(img1, img2, keys1, keys2, vecIndex, img3);
	//imwrite(dir + "\\" +"new"+ if11 + "-" + if12, img3);

	WriteMatchesFile(f11, vecMatch);
	WriteIndexPair(f11 + ".index", vecIndex);
	WriteMatchesFile(f11 + ".txt", vecMatch, false);
	// clean up..

	//string f21(f2), f22(get_fname(f1));
	//erase_extention(f21);
	QString f21(f2_file.absolutePath() + "/" + f2_file.completeBaseName());
	QString f22(f1_file.completeBaseName());
	QDir().mkdir(f21);
	//_mkdir(f21.c_str());
	//erase_extention(f22);
	f21 += "/";
	f21 += f22;
	f21 += ".result";
	QVector<PointMatch> nvecMatch(vecMatch.size());
	QVector<V2I> nvecIndex(vecIndex.size());
	auto itM = nvecMatch.begin();
	auto itI = nvecIndex.begin();
	for (size_t i = 0; i < vecMatch.size(); ++i)
	{
		itM->first = vecMatch[i].second;
		itM++->second = vecMatch[i].first;
		itI->x = vecIndex[i].y;
		itI++->y = vecIndex[i].x;
	}
	WriteMatchesFile(f21, nvecMatch);
	WriteIndexPair(f21 + ".index", nvecIndex);
	WriteMatchesFile(f21 + ".txt", nvecMatch, false);

	return true;
}

void FeatureExactor::resultToHomol()
{
	if (image_path_list_.size() == 0)
		throw QString("no image found!");
	QFileInfo file_info(image_path_list_[0]);
	QDir qdir(file_info.absoluteDir());
	if (!qdir.exists())
		throw QString("directory not exists!");
	//QStringList filter;
	//filter << "*.result";
	//qdir.entryInfoList(filter, QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);
	//判断路径是否存在


	//获取所选文件类型过滤器
	QStringList filters;
	filters << "*.result";

	//定义迭代器并设置过滤器
	QDirIterator dir_iterator(file_info.absolutePath(),
		filters,
		QDir::Files | QDir::NoSymLinks,
		QDirIterator::Subdirectories);
	QStringList string_list;
	while (dir_iterator.hasNext())
	{
		dir_iterator.next();
		QFileInfo file_info = dir_iterator.fileInfo();
		QString file_path = file_info.absoluteFilePath();
		string_list.append(file_path);
	}
	vector<QVector<PointMatch>> all_matches(string_list.size());
	yt::multiCall(ReadMatchesFile, string_list.toVector().toStdVector(), all_matches,true);
	QVector<QString> vec_output;
	QString homol_path=QFileInfo(image_path_list_[0]).absolutePath()+"/Homol";
	if (!QDir().exists(homol_path))
		QDir().mkdir(homol_path);
	for (int i = 0; i < string_list.size();++i)
	{
		QString file_name = string_list.at(i);
		file_name.replace(QString("features"), QString("Homol"));
		file_name.replace(QString(".result"), QString(".dat"));
		if (!QDir().exists(QFileInfo(file_name).absolutePath()))
			QDir().mkdir(QFileInfo(file_name).absolutePath());

		vec_output.push_back(file_name);
	}
	yt::multiCall(WriteMatchesFile, vec_output.toStdVector(), all_matches,true);
}
}

