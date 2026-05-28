#include <jni.h>
#include "mmap_demo.h"
#include "mmap_utils.h"

#include <string>

extern "C" {

JNIEXPORT jstring JNICALL
Java_com_example_myapplication_MainActivity_runAllMmapDemos(
        JNIEnv* env,
        jobject /* this */,
        jstring filesDir) {
    const char* path = env->GetStringUTFChars(filesDir, nullptr);
    std::string result = runAllMmapDemos(path ? path : "");
    env->ReleaseStringUTFChars(filesDir, path);
    return env->NewStringUTF(result.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_example_myapplication_MainActivity_runSingleMmapDemo(
        JNIEnv* env,
        jobject /* this */,
        jstring filesDir,
        jint demoId) {
    const char* path = env->GetStringUTFChars(filesDir, nullptr);
    std::string result = runSingleMmapDemo(path ? path : "", demoId);
    env->ReleaseStringUTFChars(filesDir, path);
    return env->NewStringUTF(result.c_str());
}

}  // extern "C"
