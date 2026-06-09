#include <jni.h>
#include <android/log.h>
#include <memory>
#include <span>
#include "view/view_engine.h"

#define TAG "ViewJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

using namespace android::viewdemo;

static std::unique_ptr<ViewEngine> gEngine;

namespace {

bool fillFloatArray(JNIEnv* env, jfloatArray arr, const auto& filler) {
    if (!gEngine || !arr) return false;
    const jsize len = env->GetArrayLength(arr);
    if (len <= 0) return false;

    jfloat* data = env->GetFloatArrayElements(arr, nullptr);
    if (!data) return false;

    const auto res = filler(std::span<float>{data, static_cast<std::size_t>(len)});
    env->ReleaseFloatArrayElements(arr, data, 0);
    if (!res) {
        LOGE("fill failed: %.*s", static_cast<int>(to_string(res.error()).size()),
             to_string(res.error()).data());
        return false;
    }
    return true;
}

LayoutMode modeFromJint(jint m) {
    switch (m) {
        case 1: return LayoutMode::Spiral;
        case 2: return LayoutMode::Heart;
        default: return LayoutMode::Circle;
    }
}

} // namespace

extern "C" {

JNIEXPORT void JNICALL
Java_com_example_viewtest_native_ViewNativeBridge_nativeInit(
        JNIEnv* /*env*/, jclass /*clazz*/, jint particleCount, jint waveSamples, jint orbitItems) {
    ViewEngineConfig cfg{};
    cfg.particleCount = particleCount;
    cfg.waveformSamples = waveSamples;
    cfg.orbitItemCount = orbitItems;
    gEngine = std::make_unique<ViewEngine>(cfg);
    LOGI("ViewEngine init particles=%d wave=%d orbit=%d", particleCount, waveSamples, orbitItems);
}

JNIEXPORT void JNICALL
Java_com_example_viewtest_native_ViewNativeBridge_nativeDestroy(JNIEnv* /*env*/, jclass /*clazz*/) {
    gEngine.reset();
    LOGI("ViewEngine destroyed");
}

JNIEXPORT void JNICALL
Java_com_example_viewtest_native_ViewNativeBridge_nativeResize(
        JNIEnv* /*env*/, jclass /*clazz*/, jfloat width, jfloat height) {
    if (gEngine) gEngine->resize(width, height);
}

JNIEXPORT void JNICALL
Java_com_example_viewtest_native_ViewNativeBridge_nativeSetTouch(
        JNIEnv* /*env*/, jclass /*clazz*/, jfloat x, jfloat y, jboolean active) {
    if (gEngine) gEngine->setTouch(x, y, active == JNI_TRUE);
}

JNIEXPORT void JNICALL
Java_com_example_viewtest_native_ViewNativeBridge_nativeTick(
        JNIEnv* /*env*/, jclass /*clazz*/, jfloat dtSec, jfloat phaseDelta) {
    if (!gEngine) return;
    gEngine->tick(dtSec);
    gEngine->advancePhase(phaseDelta);
}

JNIEXPORT jboolean JNICALL
Java_com_example_viewtest_native_ViewNativeBridge_nativeFillParticles(
        JNIEnv* env, jclass /*clazz*/, jfloatArray out) {
    return fillFloatArray(env, out, [](std::span<float> span) { return gEngine->copyParticles(span); });
}

JNIEXPORT jboolean JNICALL
Java_com_example_viewtest_native_ViewNativeBridge_nativeFillWaveform(
        JNIEnv* env, jclass /*clazz*/, jfloatArray out) {
    return fillFloatArray(env, out, [](std::span<float> span) { return gEngine->copyWaveform(span); });
}

JNIEXPORT jboolean JNICALL
Java_com_example_viewtest_native_ViewNativeBridge_nativeFillOrbit(
        JNIEnv* env, jclass /*clazz*/, jfloatArray out) {
    return fillFloatArray(env, out, [](std::span<float> span) { return gEngine->copyOrbitLayout(span); });
}

JNIEXPORT void JNICALL
Java_com_example_viewtest_native_ViewNativeBridge_nativeSetParticleCount(
        JNIEnv* /*env*/, jclass /*clazz*/, jint count) {
    if (gEngine) gEngine->setParticleCount(count);
}

JNIEXPORT void JNICALL
Java_com_example_viewtest_native_ViewNativeBridge_nativeSetHarmonics(
        JNIEnv* /*env*/, jclass /*clazz*/, jint count) {
    if (gEngine) gEngine->setHarmonics(count);
}

JNIEXPORT void JNICALL
Java_com_example_viewtest_native_ViewNativeBridge_nativeSetGravity(
        JNIEnv* /*env*/, jclass /*clazz*/, jfloat gravity) {
    if (gEngine) gEngine->setGravity(gravity);
}

JNIEXPORT void JNICALL
Java_com_example_viewtest_native_ViewNativeBridge_nativeSetLayoutMode(
        JNIEnv* /*env*/, jclass /*clazz*/, jint mode) {
    if (gEngine) gEngine->setLayoutMode(modeFromJint(mode));
}

JNIEXPORT jstring JNICALL
Java_com_example_viewtest_native_ViewNativeBridge_nativeGetStats(
        JNIEnv* env, jclass /*clazz*/) {
    if (!gEngine) return env->NewStringUTF("{}");
    const auto json = gEngine->statsJson();
    return env->NewStringUTF(json.c_str());
}

} // extern "C"
