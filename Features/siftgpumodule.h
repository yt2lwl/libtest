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
	bool Init(size_t pcnt_per_sub = 0);//��ʼ��
	bool IsInit();//�Ƿ��ʼ��
	bool Reload(size_t pcnt_per_sub);//���¼���siftgpu
	SiftGPU *GetSiftExactor();//��ȡ������ȡ��
	SiftMatchGPU *GetSiftMatcher();//��ȡ����ƥ����
private:
	void* hsiftgpu = nullptr;
	SiftGPU *sift = nullptr;
	SiftMatchGPU *matcher = nullptr;
	size_t pps = 100000;

};
}


#endif // SIFTGPUMODULE_H
