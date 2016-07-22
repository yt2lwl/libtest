#include "siftgpumodule.h"

#include "SiftGPU.h"

#define  SIFTGPU_DLL_RUNTIME
#ifdef _WIN32
#ifdef SIFTGPU_DLL_RUNTIME
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define FREE_MYLIB FreeLibrary
#define GET_MYPROC GetProcAddress
#define THMODULE HMODULE
#else
//define this to get dll import definition for win32
#define SIFTGPU_DLL
#ifdef _DEBUG 
#pragma comment(lib, "siftgpu_d.lib")
#else
#pragma comment(lib, "siftgpu.lib")
#endif
#endif
#else
#ifdef SIFTGPU_DLL_RUNTIME
#include <dlfcn.h>
#define FREE_MYLIB dlclose
#define GET_MYPROC dlsym
#define THMODULE 
#endif
#endif
namespace qm{
SiftGpuModule::SiftGpuModule(QObject *parent)
	: QObject(parent)
{

}



SiftGpuModule::~SiftGpuModule()
{
	if (hsiftgpu)
	{
		FREE_MYLIB((THMODULE)hsiftgpu);
		hsiftgpu = nullptr;
	}
	if (sift)
	{
		delete sift;
		sift = nullptr;
	}
	if (matcher)
	{
		delete matcher;
		matcher = nullptr;
	}
}


bool SiftGpuModule::Init(size_t pcnt_per_sub)
{
	if (pcnt_per_sub == pps&& hsiftgpu!=nullptr)
		return true;
#ifdef SIFTGPU_DLL_RUNTIME
#ifdef _WIN32
#ifdef _DEBUG
	hsiftgpu = LoadLibrary(L"siftgpu_d.dll");
#else
	hsiftgpu = LoadLibrary(L"siftgpu.dll");
#endif
#else
	hsiftgpu = dlopen("libsiftgpu.so", RTLD_LAZY);
#endif
	if (hsiftgpu == nullptr)//≥ı ºªØ ß∞‹
		return false;

	SiftGPU* (*pCreateNewSiftGPU)(int) = nullptr;
	SiftMatchGPU* (*pCreateNewSiftMatchGPU)(int) = nullptr;
	pCreateNewSiftGPU = (SiftGPU* (*) (int)) GetProcAddress((THMODULE)hsiftgpu, "CreateNewSiftGPU");
	pCreateNewSiftMatchGPU = (SiftMatchGPU* (*)(int)) GetProcAddress((THMODULE)hsiftgpu, "CreateNewSiftMatchGPU");
	sift = pCreateNewSiftGPU(1);
	matcher = pCreateNewSiftMatchGPU(4096);
#else
	SiftGPU *sift=new SiftGPU(1);
	SiftMatchGPU *matcher = new SiftMatchGPU(4096);
#endif
#ifdef _DEBUG
	char * nargv[] = { "-fo", "-1", "-v", "1", "-cuda", "-tc", "100000" };
#else
	char * nargv[] = { "-fo", "-1", "-v", "0", "-cuda", "-tc", "100000" };
#endif // _DEBUG

	
	int nargc = sizeof(nargv) / sizeof(char*);
	if (pcnt_per_sub == 0)
	{
		nargc -= 2;
		pps = 100000;
	}
	else
	{
		char s[7];
		sprintf_s(s, "%u", pcnt_per_sub);
		nargv[nargc - 1] = s;
		pps = pcnt_per_sub;
	}
	//
	//-fo -1    staring from -1 octave 
	//-v 1      only print out # feature and overall time
	//-loweo    add a (.5, .5) offset
	//-tc <num> set a soft limit to number of detected features

	//NEW:  parameters for  GPU-selection
	//1. CUDA.                   Use parameter "-cuda", "[device_id]"
	//2. OpenGL.				 Use "-Display", "display_name" to select monitor/GPU (XLIB/GLUT)
	//   		                 on windows the display name would be something like \\.\DISPLAY4

	//////////////////////////////////////////////////////////////////////////////////////
	//You use CUDA for nVidia graphic cards by specifying
	//-cuda   : cuda implementation (fastest for smaller images)
	//          CUDA-implementation allows you to create multiple instances for multiple threads
	//          Checkout src\TestWin\MultiThreadSIFT
	/////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////////////
	////////////////////////Two Important Parameters///////////////////////////
	// First, texture reallocation happens when image size increases, and too many 
	// reallocation may lead to allocatoin failure.  You should be careful when using 
	// siftgpu on a set of images with VARYING imag sizes. It is recommended that you 
	// preset the allocation size to the largest width and largest height by using function
	// AllocationPyramid or prameter '-p' (e.g. "-p", "1024x768").

	// Second, there is a parameter you may not be aware of: the allowed maximum working
	// dimension. All the SIFT octaves that needs a larger texture size will be skipped.
	// The default prameter is 2560 for the unpacked implementation and 3200 for the packed.
	// Those two default parameter is tuned to for 768MB of graphic memory. You should adjust
	// it for your own GPU memory. You can also use this to keep/skip the small featuers.
	// To change this, call function SetMaxDimension or use parameter "-maxd".
	//
	// NEW: by default SiftGPU will try to fit the cap of GPU memory, and reduce the working 
	// dimension so as to not allocate too much. This feature can be disabled by -nomc
	//////////////////////////////////////////////////////////////////////////////////////



	sift->ParseParam(nargc, nargv);
	//sift->CreateContextGL();
	//sift->SetMaxDimension(chunk_size);
	///////////////////////////////////////////////////////////////////////
	//Only the following parameters can be changed after initialization (by calling ParseParam). 
	//-dw, -ofix, -ofix-not, -fo, -unn, -maxd, -b
	//to change other parameters at runtime, you need to first unload the dynamically loaded libaray
	//reload the libarary, then create a new siftgpu instance
	//matcher->CreateContextGL();

	return true;
}


SiftGPU *SiftGpuModule::GetSiftExactor()
{
	return sift;
}


SiftMatchGPU *SiftGpuModule::GetSiftMatcher()
{
	return matcher;
}

bool SiftGpuModule::IsInit()
{
	return hsiftgpu != nullptr;
}

bool SiftGpuModule::Reload(size_t pcnt_per_sub)
{
	if (pcnt_per_sub == pps && hsiftgpu!=nullptr)
		return true;
	if (sift)
	{
		delete sift;
		sift = nullptr;
	}

	if (matcher)
	{
		delete matcher;
		matcher = nullptr;
	}
	FREE_MYLIB((THMODULE)hsiftgpu);
	hsiftgpu = nullptr;
	return Init(pcnt_per_sub);
}
}

