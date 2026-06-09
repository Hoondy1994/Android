#include <jni.h>

#include <android/log.h>

#include "smartptr_demo.h"

#define LOG_TAG "SmartPtrDemo"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_myapplication_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    const std::string report = runSmartPtrDemo();

    // 同时打到 logcat，便于用 adb logcat 观察
    std::string line;
    for (char c : report) {
        if (c == '\n') {
            if (!line.empty()) {
                ALOGI("%s", line.c_str());
                line.clear();
            }
        } else {
            line += c;
        }
    }
    if (!line.empty()) {
        ALOGI("%s", line.c_str());
    }

    return env->NewStringUTF(report.c_str());
}
