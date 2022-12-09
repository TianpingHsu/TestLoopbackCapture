#pragma once
#include <windows.h> 
#include "..\LoopbackCapture\include\LoopbackCaptureInterface.h"

#define START_CAPTURE() do { \
    LoopbackCaptureWrapper* pLCW = LoopbackCaptureWrapper::instance(); \
	if (pLCW) { \
		HRESULT hr = pLCW->StartCapture(); \
		printf("StartCapture hr: %d\n", hr); \
	} \
	else { \
		printf("empty pointer\n"); \
	} \
} while (0)

#define STOP_CAPTURE() do { \
    LoopbackCaptureWrapper* pLCW = LoopbackCaptureWrapper::instance(); \
	if (pLCW) { \
		HRESULT hr = pLCW->StopCapture(); \
		printf("StopCapture hr: %d\n", hr); \
	} \
	else { \
		printf("empty pointer\n"); \
	} \
} while (0)

class LoopbackCaptureWrapper {
public:
	static LoopbackCaptureWrapper* instance();
	HRESULT StartCapture();
	HRESULT StopCapture();
	LoopbackParam* GetParam();
private:
	LoopbackCaptureWrapper();
	LoopbackCaptureWrapper(const LoopbackCaptureWrapper& ) = delete;
	LoopbackCaptureWrapper& operator=(const LoopbackCaptureWrapper& ) = delete;
	~LoopbackCaptureWrapper();
	HINSTANCE hinstLib = { NULL };
	bool mSupportLoopbackCapture = { false };
	bool mLoadDLL = { false };
};
