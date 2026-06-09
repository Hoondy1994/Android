#include <jni.h>
#include <string>
#include <memory>
#include <android/log.h>
#include "audio/audio_engine.h"

#define TAG "AudioJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

using namespace android::audio;

static std::unique_ptr<AudioEngine> gEngine;

extern "C" {

// ─── 初始化 / 销毁 ─────────────────────────────────────────────────────────────
JNIEXPORT void JNICALL
Java_com_example_audiotest_MainActivity_nativeInit(JNIEnv* env, jobject /*thiz*/, jstring storageDir)
{
    const char* dir = env->GetStringUTFChars(storageDir, nullptr);
    gEngine = std::make_unique<AudioEngine>(std::string(dir));
    gEngine->setOnPlaybackComplete([]{ LOGI("Playback complete"); });
    env->ReleaseStringUTFChars(storageDir, dir);
    LOGI("AudioEngine initialized, dir=%s", dir);
}

JNIEXPORT void JNICALL
Java_com_example_audiotest_MainActivity_nativeDestroy(JNIEnv* /*env*/, jobject /*thiz*/)
{
    gEngine.reset();
    LOGI("AudioEngine destroyed");
}

// ─── 录音 ─────────────────────────────────────────────────────────────────────
JNIEXPORT jstring JNICALL
Java_com_example_audiotest_MainActivity_nativeStartRecording(JNIEnv* env, jobject /*thiz*/)
{
    if (!gEngine) return env->NewStringUTF("");
    auto res = gEngine->startRecording();
    if (!res) {
        LOGE("startRecording failed: %s", to_string(res.error()).data());
        return env->NewStringUTF("");
    }
    return env->NewStringUTF(res->c_str());
}

JNIEXPORT jboolean JNICALL
Java_com_example_audiotest_MainActivity_nativeStopRecording(JNIEnv* /*env*/, jobject /*thiz*/)
{
    if (!gEngine) return JNI_FALSE;
    return gEngine->stopRecording() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jlong JNICALL
Java_com_example_audiotest_MainActivity_nativeGetRecordingElapsedMs(JNIEnv* /*env*/, jobject /*thiz*/)
{
    return gEngine ? gEngine->recordingElapsedMs() : 0LL;
}

// ─── 播放 ─────────────────────────────────────────────────────────────────────
JNIEXPORT jboolean JNICALL
Java_com_example_audiotest_MainActivity_nativeStartPlayback(JNIEnv* env, jobject /*thiz*/, jstring path)
{
    if (!gEngine) return JNI_FALSE;
    const char* p = env->GetStringUTFChars(path, nullptr);
    bool ok = static_cast<bool>(gEngine->startPlayback(p));
    env->ReleaseStringUTFChars(path, p);
    return ok ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_example_audiotest_MainActivity_nativePausePlayback(JNIEnv* /*env*/, jobject /*thiz*/)
{
    return (gEngine && gEngine->pausePlayback()) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_example_audiotest_MainActivity_nativeResumePlayback(JNIEnv* /*env*/, jobject /*thiz*/)
{
    return (gEngine && gEngine->resumePlayback()) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_example_audiotest_MainActivity_nativeStopPlayback(JNIEnv* /*env*/, jobject /*thiz*/)
{
    if (gEngine) gEngine->stopPlayback();
}

JNIEXPORT void JNICALL
Java_com_example_audiotest_MainActivity_nativeSeekTo(JNIEnv* /*env*/, jobject /*thiz*/, jlong posMs)
{
    if (gEngine) gEngine->seekTo(posMs);
}

// ─── 状态查询 ─────────────────────────────────────────────────────────────────
JNIEXPORT jint JNICALL
Java_com_example_audiotest_MainActivity_nativeGetState(JNIEnv* /*env*/, jobject /*thiz*/)
{
    return gEngine ? static_cast<jint>(gEngine->engineState()) : 0;
}

JNIEXPORT jlong JNICALL
Java_com_example_audiotest_MainActivity_nativeGetPlayPositionMs(JNIEnv* /*env*/, jobject /*thiz*/)
{
    return gEngine ? gEngine->playPositionMs() : 0LL;
}

JNIEXPORT jlong JNICALL
Java_com_example_audiotest_MainActivity_nativeGetPlayDurationMs(JNIEnv* /*env*/, jobject /*thiz*/)
{
    return gEngine ? gEngine->playDurationMs() : 0LL;
}

// ─── 录音列表（JSON 字符串） ──────────────────────────────────────────────────
JNIEXPORT jstring JNICALL
Java_com_example_audiotest_MainActivity_nativeListRecordings(JNIEnv* env, jobject /*thiz*/)
{
    if (!gEngine) return env->NewStringUTF("[]");
    auto list = gEngine->listRecordings();
    std::string json = "[";
    for (std::size_t i = 0; i < list.size(); ++i) {
        const auto& r = list[i];
        if (i) json += ',';
        json += "{\"path\":\"" + r.filePath + "\""
              + ",\"name\":\"" + r.displayName + "\""
              + ",\"durationMs\":" + std::to_string(r.durationMs)
              + ",\"sizeBytes\":"  + std::to_string(r.sizeBytes)
              + "}";
    }
    json += "]";
    return env->NewStringUTF(json.c_str());
}

JNIEXPORT jboolean JNICALL
Java_com_example_audiotest_MainActivity_nativeDeleteRecording(JNIEnv* env, jobject /*thiz*/, jstring path)
{
    if (!gEngine) return JNI_FALSE;
    const char* p = env->GetStringUTFChars(path, nullptr);
    bool ok = gEngine->deleteRecording(p);
    env->ReleaseStringUTFChars(path, p);
    return ok ? JNI_TRUE : JNI_FALSE;
}

} // extern "C"
