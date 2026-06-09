#include <jni.h>

#include <string>

#include "hal/hal_registry.h"
#include "hal/hal_types.h"

using hal::HalRegistry;
using hal::HalError;

static HalRegistry& registry() { return HalRegistry::instance(); }

extern "C" {

JNIEXPORT jint JNICALL
Java_com_example_myapplication_hal_native_NativeHalBridge_nativeInitInternal(JNIEnv*, jclass) {
    return registry().init();
}

JNIEXPORT jint JNICALL
Java_com_example_myapplication_hal_native_NativeHalBridge_nativeShutdownInternal(JNIEnv*, jclass) {
    return registry().shutdown();
}

JNIEXPORT jstring JNICALL
Java_com_example_myapplication_hal_native_NativeHalBridge_nativeGetVersionInternal(JNIEnv* env, jclass) {
    return env->NewStringUTF("hal-native-cpp/1.0.0");
}

JNIEXPORT jlong JNICALL
Java_com_example_myapplication_hal_native_NativeHalBridge_nativeOpenModuleInternal(
        JNIEnv* env, jclass, jstring module_id) {
    const char* id = env->GetStringUTFChars(module_id, nullptr);
    const int64_t handle = registry().open_module(id);
    env->ReleaseStringUTFChars(module_id, id);
    return handle;
}

JNIEXPORT jint JNICALL
Java_com_example_myapplication_hal_native_NativeHalBridge_nativeCloseModuleInternal(
        JNIEnv*, jclass, jlong handle) {
    return registry().close_module(handle);
}

JNIEXPORT jfloatArray JNICALL
Java_com_example_myapplication_hal_native_NativeHalBridge_nativeSensorReadInternal(
        JNIEnv* env, jclass, jlong handle) {
    hal::SensorReading reading{};
    const int32_t rc = registry().sensor_read(handle, &reading);
    if (rc != static_cast<int32_t>(HalError::Ok)) return nullptr;
    jfloatArray arr = env->NewFloatArray(4);
    const jfloat values[4] = {reading.temperature_c, reading.humidity_pct, reading.pressure_hpa,
                              reading.lux};
    env->SetFloatArrayRegion(arr, 0, 4, values);
    return arr;
}

JNIEXPORT jint JNICALL
Java_com_example_myapplication_hal_native_NativeHalBridge_nativeGpioConfigureInternal(
        JNIEnv*, jclass, jlong handle, jint pin, jint direction, jboolean pull_up) {
    return registry().gpio_configure(handle, pin, direction, pull_up == JNI_TRUE);
}

JNIEXPORT jint JNICALL
Java_com_example_myapplication_hal_native_NativeHalBridge_nativeGpioReadInternal(
        JNIEnv*, jclass, jlong handle, jint pin) {
    return registry().gpio_read_pin(handle, pin);
}

JNIEXPORT jint JNICALL
Java_com_example_myapplication_hal_native_NativeHalBridge_nativeGpioWriteInternal(
        JNIEnv*, jclass, jlong handle, jint pin, jint level) {
    return registry().gpio_write_pin(handle, pin, level);
}

JNIEXPORT jint JNICALL
Java_com_example_myapplication_hal_native_NativeHalBridge_nativeGpioReadPortInternal(
        JNIEnv*, jclass, jlong handle, jint mask) {
    return registry().gpio_read_port(handle, mask);
}

JNIEXPORT jint JNICALL
Java_com_example_myapplication_hal_native_NativeHalBridge_nativePowerSetRailInternal(
        JNIEnv*, jclass, jlong handle, jint rail, jint state) {
    return registry().power_set_rail(handle, static_cast<hal::PowerRail>(rail),
                                     static_cast<hal::PowerState>(state));
}

JNIEXPORT jintArray JNICALL
Java_com_example_myapplication_hal_native_NativeHalBridge_nativePowerGetRailInternal(
        JNIEnv* env, jclass, jlong handle, jint rail) {
    int32_t state = 0;
    int32_t mw = 0;
    const int32_t rc =
            registry().power_get_rail(handle, static_cast<hal::PowerRail>(rail), &state, &mw);
    if (rc != static_cast<int32_t>(HalError::Ok)) return nullptr;
    jintArray arr = env->NewIntArray(2);
    const jint values[2] = {state, mw};
    env->SetIntArrayRegion(arr, 0, 2, values);
    return arr;
}

JNIEXPORT jint JNICALL
Java_com_example_myapplication_hal_native_NativeHalBridge_nativePowerWakeAllInternal(
        JNIEnv*, jclass, jlong handle) {
    return registry().power_wake_all(handle);
}

}  // extern "C"
