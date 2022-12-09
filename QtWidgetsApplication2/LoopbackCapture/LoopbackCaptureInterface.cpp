#include "LoopbackCaptureInterface.h"
#include "LoopbackCapture.h"
#include <cstdio>

static CLoopbackCapture s_LoopbackCapture;

HRESULT LC_StartCaptureAsync() {
	return s_LoopbackCapture.StartCaptureAsync();
}

HRESULT LC_StopCaptureAsync() {
	return s_LoopbackCapture.StopCaptureAsync();
}

LoopbackParam * LC_GetParam() {
	return &(s_LoopbackCapture.lparam);
}
