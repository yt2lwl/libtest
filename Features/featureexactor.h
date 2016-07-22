#ifndef FEATUREEXACTOR_H
#define FEATUREEXACTOR_H

#include "features_global.h"
#include <QObject>
#include <QString>
#include<QVector>



namespace qm{
class SiftGpuModule;
struct SiftFeature;
class V2I;
class Pt2dr;
typedef QPair<Pt2dr, Pt2dr> PointMatch;
class FEATURES_EXPORT FeatureExactor //: public QObject
{
	//Q_OBJECT

public:
	//FeatureExactor(QObject *parent);
	FeatureExactor();
	~FeatureExactor();
	bool featureExactSave(int chunk_size, size_t subcnt, bool buildRelation);
	bool featureMatchSave();
	void resultToHomol();
	void set_image_path_list(QVector<QString> &image_path_list){ image_path_list_ = image_path_list; }
private:
	QVector<QString> image_path_list_;
	SiftGpuModule *gpu_module_;
	bool featureExact(unsigned char *data, size_t width, size_t height,
		size_t chunk_size, size_t subcnt, SiftFeature &features);
	//±£´æÌØÕ÷
	bool featureSave(const std::string &file, SiftFeature &features);

	bool featureMatch(const QPair<SiftFeature, SiftFeature> &pairImg,
		QVector<PointMatch> &vecMatch, QVector<V2I> &vecIndex);

	bool featureMatchSave(const QVector<QPair<QString, QString>> &pairs_image, bool ransca = true);

	bool featureMatchSaves(const QPair<QString, QString> &a_pair, bool ransca = true);
};

}

#endif // FEATUREEXACTOR_H