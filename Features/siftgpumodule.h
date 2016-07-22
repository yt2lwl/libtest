#ifndef SIFTGPUMODULE_H
#define SIFTGPUMODULE_H

#include <QObject>
class SiftGPU;
class SiftMatchGPU;

namespace qm{
class SiftGpuModule : public QObject
{
	Q_OBJECT

public:
	SiftGpuModule(QObject *parent);
	~SiftGpuModule();
	bool Init(size_t pcnt_per_sub = 0);//初始化
	bool IsInit();//是否初始化
	bool Reload(size_t pcnt_per_sub);//重新加载siftgpu
	SiftGPU *GetSiftExactor();//获取特征提取类
	SiftMatchGPU *GetSiftMatcher();//获取特征匹配类
private:
	void* hsiftgpu = nullptr;
	SiftGPU *sift = nullptr;
	SiftMatchGPU *matcher = nullptr;
	size_t pps = 100000;

};
}


#endif // SIFTGPUMODULE_H
