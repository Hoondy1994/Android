// TODO: Add CopyRight

#ifndef ULE_NAPI_JNI_UTIL_H_
#define ULE_NAPI_JNI_UTIL_H_

#include <jni.h>

#include "utils/macros.h"

namespace ule {
namespace jni {

void InitJavaVM(JavaVM* vm);

void SetClassLoader(jobject obj);

NAPI_EXPORT JNIEnv* AttachCurrentThread();

NAPI_EXPORT jclass LoadClass(JNIEnv* env, const char* class_name);

NAPI_EXPORT jobject GetApplicationContext();

void DetachFromVM();

}  // namespace jni
}  // namespace ule

#endif
