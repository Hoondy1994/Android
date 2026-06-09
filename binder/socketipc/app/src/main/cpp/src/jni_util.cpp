#include "jni/jni_util.hpp"

namespace jniutil {

std::string to_string(JNIEnv* env, const jstring value) {
    if (value == nullptr) {
        return {};
    }
    const char* chars = env->GetStringUTFChars(value, nullptr);
    std::string result = chars == nullptr ? std::string{} : std::string{chars};
    if (chars != nullptr) {
        env->ReleaseStringUTFChars(value, chars);
    }
    return result;
}

jstring to_jstring(JNIEnv* env, const std::string& value) {
    return env->NewStringUTF(value.c_str());
}

std::vector<std::uint8_t> to_bytes(JNIEnv* env, const jbyteArray array) {
    if (array == nullptr) {
        return {};
    }
    const jsize length = env->GetArrayLength(array);
    std::vector<std::uint8_t> out(static_cast<std::size_t>(length));
    env->GetByteArrayRegion(array, 0, length, reinterpret_cast<jbyte*>(out.data()));
    return out;
}

jbyteArray to_jbyte_array(JNIEnv* env, const std::span<const std::uint8_t> data) {
    jbyteArray array = env->NewByteArray(static_cast<jsize>(data.size()));
    if (!data.empty()) {
        env->SetByteArrayRegion(array, 0, static_cast<jsize>(data.size()),
                                reinterpret_cast<const jbyte*>(data.data()));
    }
    return array;
}

}  // namespace jniutil
