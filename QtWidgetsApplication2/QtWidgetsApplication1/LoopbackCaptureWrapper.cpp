#include "LoopbackCaptureWrapper.h"
#include "qdebug.h"
#include "objbase.h"

typedef HRESULT (__cdecl *LC_INTERFACE)(); 
typedef LoopbackParam* (__cdecl *LC_GETPARAM)(); 

LoopbackCaptureWrapper* LoopbackCaptureWrapper::instance() {
	static LoopbackCaptureWrapper wrapper;
	return &wrapper;
}

LoopbackCaptureWrapper::LoopbackCaptureWrapper() {
	mSupportLoopbackCapture = true;  // this is used for test
	if (mSupportLoopbackCapture) {
		hinstLib = LoadLibrary(TEXT("LoopbackCapture.dll"));
		if (hinstLib != NULL) {
			mLoadDLL = true;
			qInfo() << "[LOOPBACKCAPTURE] load library OK";
		}
		else {
			qInfo() << "[LOOPBACKCAPTURE] failed to load library";
		}
	}
	else {
		qInfo() << "[LOOPBACKCAPTURE] platform not support loopback capture";
	}
}

LoopbackCaptureWrapper::~LoopbackCaptureWrapper() {
	if (mSupportLoopbackCapture && mLoadDLL) {
		int ret = FreeLibrary(hinstLib);
		qInfo() << "[LOOPBACKCAPTURE] free result:" << ret;
	}
	else {
		qInfo() << "[LOOPBACKCAPTURE]" << mSupportLoopbackCapture << mLoadDLL;
	}
}

HRESULT LoopbackCaptureWrapper::StartCapture() {
	if (mSupportLoopbackCapture && mLoadDLL) {
		LC_INTERFACE startCaptureAsync = (LC_INTERFACE)GetProcAddress(hinstLib, "LC_StartCaptureAsync");
		if (NULL != startCaptureAsync) {
			//HRESULT tmp = CoInitialize(NULL);
			//qInfo() << "[LOOPBACKCAPTURE] GetProcAddress OK: CoInitialize, call with state: " << tmp;
			HRESULT hr = startCaptureAsync();
			qInfo() << "[LOOPBACKCAPTURE] GetProcAddress OK: LC_StartCaptureAsync, call with state: " << hr;
			return hr;
		}
		else {
			qInfo() << "[LOOPBACKCAPTURE] GetProcAddress NOT OK: LC_StartCaptureAsync";
		}
	}
	else {
		qInfo() << "[LOOPBACKCAPTURE]" << mSupportLoopbackCapture << mLoadDLL;
	}
	return S_FALSE;
}

HRESULT LoopbackCaptureWrapper::StopCapture() {
	if (mSupportLoopbackCapture && mLoadDLL) {
		LC_INTERFACE stopCaptureAsync = (LC_INTERFACE)GetProcAddress(hinstLib, "LC_StopCaptureAsync");
		if (NULL != stopCaptureAsync) {
			HRESULT hr = stopCaptureAsync();
			qInfo() << "[LOOPBACKCAPTURE] GetProcAddress OK: LC_StopCaptureAsync, call with state: " << hr;
			return hr;
		}
		else {
			qInfo() << "[LOOPBACKCAPTURE] GetProcAddress NOT OK: LC_StopCaptureAsync";
		}
	}
	else {
		qInfo() << "[LOOPBACKCAPTURE]" << mSupportLoopbackCapture << mLoadDLL;
	}
	return S_FALSE;

}LoopbackParam* LoopbackCaptureWrapper::GetParam() {
	if (mSupportLoopbackCapture && mLoadDLL) {
		LC_GETPARAM getParam = (LC_GETPARAM)GetProcAddress(hinstLib, "LC_GetParam");
		if (NULL != getParam) {
			LoopbackParam* lparam = getParam();
			qInfo() << "[LOOPBACKCAPTURE] GetProcAddress OK: LC_GetParam, lparam: " << lparam;
			return lparam;
		}
		else {
			qInfo() << "[LOOPBACKCAPTURE] GetProcAddress NOT OK: LC_GetParam";
		}
	}
	else {
		qInfo() << "[LOOPBACKCAPTURE]" << mSupportLoopbackCapture << mLoadDLL;
	}
	return nullptr;
}