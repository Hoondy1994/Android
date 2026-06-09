#include <jni.h>

#include "egl_renderer.h"

#include <android/native_window_jni.h>

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_example_surfacetest_native_SurfaceNativeBridge_nativeAttach(
        JNIEnv* env, jclass, jobject surface) {
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (!window) return JNI_FALSE;
    const bool ok = flingerEglAttach(window);
    ANativeWindow_release(window);
    return ok ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_example_surfacetest_native_SurfaceNativeBridge_nativeDetach(JNIEnv*, jclass) {
    flingerEglDetach();
}

JNIEXPORT void JNICALL
Java_com_example_surfacetest_native_SurfaceNativeBridge_nativeResize(
        JNIEnv*, jclass, jint width, jint height) {
    flingerEglResize(width, height);
}

JNIEXPORT void JNICALL
Java_com_example_surfacetest_native_SurfaceNativeBridge_nativeDraw(
        JNIEnv*, jclass, jfloat timeSec, jint stressLevel, jint mode) {
    flingerEglDraw(timeSec, stressLevel, mode);
}

JNIEXPORT jlongArray JNICALL
Java_com_example_surfacetest_native_SurfaceNativeBridge_nativeGetStats(JNIEnv* env, jclass) {
    FlingerNativeStats stats{};
    flingerEglGetStats(&stats);
    jlong buf[5] = {
        stats.lastSwapNs,
        stats.lastDrawUs,
        stats.bufferWidth,
        stats.bufferHeight,
        stats.swapOk,
    };
    jlongArray arr = env->NewLongArray(5);
    if (arr) env->SetLongArrayRegion(arr, 0, 5, buf);
    return arr;
}

}  // extern "C"
