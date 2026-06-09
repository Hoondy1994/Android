#pragma once

#include <android/native_window.h>
#include <cstdint>

struct FlingerNativeStats {
    int64_t lastSwapNs = 0;
    int64_t lastDrawUs = 0;
    int32_t bufferWidth = 0;
    int32_t bufferHeight = 0;
    int32_t swapOk = 0;
};

bool flingerEglAttach(ANativeWindow* window);
void flingerEglDetach();
void flingerEglResize(int width, int height);
void flingerEglDraw(float timeSec, int stressLevel, int mode);
void flingerEglGetStats(FlingerNativeStats* out);
