#pragma once

#ifdef LOOPBACKCAPTUREINTERFACE_EXPORTS
#define LOOPBACKCAPTUREINTERFACE_API __declspec(dllexport)
#else
#define LOOPBACKCAPTUREINTERFACE_API __declspec(dllimport)
#endif

#include "windows.h"
#include <atomic>

// in this test project, struct `LoopbackParam` is not used, just ignore it
struct LoopbackParam {
    BYTE* data = { nullptr };
    std::atomic<UINT32> len = { 0 };
    const UINT32 MAX_LEN = { 0x01 << 15 };
    std::atomic<bool> ready = { false };
    std::atomic<int> state = { 0 };
    int valid = { 0 };
    LoopbackParam() {
        data = new BYTE[MAX_LEN];
    }
    ~LoopbackParam() {
        if (data) {
            delete[] data;
            data = nullptr;
        }
    }
};

// this interface should be used in main thread
extern "C" LOOPBACKCAPTUREINTERFACE_API HRESULT LC_StartCaptureAsync();

extern "C" LOOPBACKCAPTUREINTERFACE_API HRESULT LC_StopCaptureAsync();

extern "C" LOOPBACKCAPTUREINTERFACE_API LoopbackParam* LC_GetParam();

