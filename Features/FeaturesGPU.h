#pragma once

#include <QList>
#include <QStringList>
#include "PointMatch.h"
//#include "SiftGPU.h"

class SiftGpuModule;

//points &&features descriptors
struct SiftFeature;

//index
class V2I;

namespace qm{
class FeatureExactator
{
public:
	FeatureExactator();
	~FeatureExactator();
	void set_image_list(QStringList &image_path_list){ image_path_list_ = image_path_list; }
	bool featureExactSave(int chunk_size, size_t subcnt, bool buildRelation);
private:
	QStringList image_path_list_;
	SiftGpuModule *gpu_module_;
	//QList<cv::Mat> image_list_;
	bool featureExact(unsigned char *data, size_t width, size_t height,
		size_t chunk_size, size_t subcnt, SiftFeature &features);
	//��������
	bool featureSave(const std::string &file, SiftFeature &features);
};
}


class CFeaturesGPU {
public:
	CFeaturesGPU(void);
	~CFeaturesGPU();
	// TODO:  �ڴ�������ķ�����

	//a file ������ȡ
	bool featureExactSave(const std::string &file, int chunk_size, size_t subcnt);
	bool featureExact(unsigned char *data, size_t width, size_t height,
		size_t chunk_size, size_t subcnt, SiftFeature &features);
	//��������
	bool featureSave(const std::string &file, SiftFeature &features);
	//a pair����ƥ��
	bool featureMatchSaves(const std::pair<std::string, std::string> &pairImg, bool ransca = true);
	bool featureMatch(const std::pair<SiftFeature, SiftFeature> &pairImg,
		std::vector<PointMatch> &vecMatch, std::vector<V2I> &vecIndex);
	//������ȡ�ͱ���
	bool featureExactSave(const std::vector<std::string> &vecFiles, int chunk_size, size_t subcnt, bool buildRelation = false);
	//����ƥ��ͱ���
	bool featureMatchSave(const std::vector<std::pair<std::string, std::string>> &pairsImg, bool ransca = true);
	//����Ӱ���ϵ
	bool buildRelations(std::vector<std::string> &vecImgFiles, const std::string &outfile, size_t less_cnt = 8);
private:
	SiftGpuModule *siftGpuModule;//siftgpuģ��
};



//yt added 2016.2.9
//
//save the SIFT result as a ANSCII/BINARY file yt added
//
bool write_siftPoint_list(const std::string &o_filename, std::vector<SiftKeypoint> &keyPoints, std::vector<float> &describe);

//yt added 2016.2.9
//
//read the SIFT result as a ANSCII/BINARY file yt added
//
bool read_siftPoint_list(const std::string &i_filename, std::vector<SiftKeypoint> &keyPoints, std::vector<float> &descriptor);

//write match result
void write_index_pair(const std::string &aFilename, const std::vector<V2I> &aMatches);




void writeSift(std::vector<std::string> &vecFiles);




