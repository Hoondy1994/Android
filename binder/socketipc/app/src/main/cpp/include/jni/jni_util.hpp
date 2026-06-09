#pragma once

#include <jni.h>
#include <span>
#include <string>
#include <vector>

namespace jniutil {

[[nodiscard]] std::string to_string(JNIEnv* env, jstring value);
[[nodiscard]] jstring to_jstring(JNIEnv* env, const std::string& value);
[[nodiscard]] std::vector<std::uint8_t> to_bytes(JNIEnv* env, jbyteArray array);
[[nodiscard]] jbyteArray to_jbyte_array(JNIEnv* env, std::span<const std::uint8_t> data);

}  // namespace jniutil
