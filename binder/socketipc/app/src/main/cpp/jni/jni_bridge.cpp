#include <unistd.h>

#include "ipc/custom_codec.hpp"
#include "ipc/parcel_codec.hpp"
#include "ipc/socket_client.hpp"
#include "ipc/socket_server.hpp"
#include "ipc/benchmark.hpp"
#include "jni/jni_util.hpp"

#include <android/log.h>

#define LOG_TAG "IpcClient"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

[[nodiscard]] std::string describe_profile(const ipc::UserProfile& profile) {
    return "id=" + std::to_string(profile.id) + ", name=" + profile.name +
           ", checksum=" + std::to_string(profile.checksum());
}

template <typename Fn>
jstring run_payload_demo(JNIEnv* env, Fn&& fn, const char* label) {
    try {
        const auto encoded = ipc::CustomCodec::encode(ipc::UserProfile::sample(100));
        const auto processed = fn(encoded);
        if (!processed) {
            return jniutil::to_jstring(env, std::string(label) + " failed: server unavailable");
        }
        const auto profile = ipc::CustomCodec::decode(*processed);
        return jniutil::to_jstring(env, std::string(label) + " => " + describe_profile(profile));
    } catch (const std::exception& ex) {
        LOGE("%s failed: %s", label, ex.what());
        return jniutil::to_jstring(env, std::string(label) + " failed");
    }
}

}  // namespace

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_myapplication_MainActivity_nativeWaitForSocketServer(
        JNIEnv* /* env */,
        jobject /* this */,
        jint max_wait_ms) {
    LOGI("[Flow:SocketPing] nativeWaitForSocketServer max_wait_ms=%d, pid=%d", max_wait_ms,
         getpid());
    try {
        auto& client = ipc::global_socket_client();
        const int step_ms = 50;
        int waited = 0;
        while (waited <= max_wait_ms) {
            if (client.ping()) {
                LOGI("[Flow:SocketPing] ping 成功, waited=%d ms", waited);
                return JNI_TRUE;
            }
            usleep(static_cast<useconds_t>(step_ms * 1000));
            waited += step_ms;
        }
    } catch (const std::exception& ex) {
        LOGE("nativeWaitForSocketServer failed: %s", ex.what());
    }
    return JNI_FALSE;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_myapplication_MainActivity_nativeSocketAdd(
        JNIEnv* /* env */,
        jobject /* this */,
        jint a,
        jint b) {
    LOGI("[Flow:SocketAdd] nativeSocketAdd(%d, %d), pid=%d", a, b, getpid());
    try {
        return ipc::global_socket_client().add(a, b);
    } catch (...) {
        return -1;
    }
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_example_myapplication_MainActivity_nativeBuildCustomPayload(
        JNIEnv* env,
        jobject /* this */) {
    LOGI("[Flow:Payload] nativeBuildCustomPayload, pid=%d", getpid());
    const auto payload = ipc::CustomCodec::encode(ipc::UserProfile::sample(100));
    return jniutil::to_jbyte_array(env, payload);
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_example_myapplication_MainActivity_nativeBuildParcelPayload(
        JNIEnv* env,
        jobject /* this */) {
    LOGI("[Flow:Payload] nativeBuildParcelPayload, pid=%d", getpid());
    const auto payload = ipc::ParcelCodec::encode(ipc::UserProfile::sample(100));
    return jniutil::to_jbyte_array(env, payload);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_myapplication_MainActivity_nativeVerifyCustomPayload(
        JNIEnv* env,
        jobject /* this */,
        jbyteArray data) {
    try {
        const auto bytes = jniutil::to_bytes(env, data);
        (void)ipc::CustomCodec::decode(bytes);
        return JNI_TRUE;
    } catch (...) {
        return JNI_FALSE;
    }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_myapplication_MainActivity_nativeVerifyParcelPayload(
        JNIEnv* env,
        jobject /* this */,
        jbyteArray data) {
    try {
        const auto bytes = jniutil::to_bytes(env, data);
        (void)ipc::ParcelCodec::decode(bytes);
        return JNI_TRUE;
    } catch (...) {
        return JNI_FALSE;
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_myapplication_MainActivity_nativeSocketProcessCustom(
        JNIEnv* env,
        jobject /* this */) {
    LOGI("[Flow:SocketComplex] nativeSocketProcessCustom, pid=%d", getpid());
    return run_payload_demo(
        env,
        [](const std::vector<std::uint8_t>& payload) {
            return ipc::global_socket_client().process_custom(payload);
        },
        "Socket Custom");
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_myapplication_MainActivity_nativeSocketProcessParcel(
        JNIEnv* env,
        jobject /* this */) {
    LOGI("[Flow:SocketComplex] nativeSocketProcessParcel, pid=%d", getpid());
    try {
        const auto encoded = ipc::ParcelCodec::encode(ipc::UserProfile::sample(100));
        const auto processed = ipc::global_socket_client().process_parcel(encoded);
        if (!processed) {
            return jniutil::to_jstring(env, "Socket Parcel failed: server unavailable");
        }
        const auto profile = ipc::ParcelCodec::decode(*processed);
        return jniutil::to_jstring(env, std::string("Socket Parcel => ") + describe_profile(profile));
    } catch (const std::exception& ex) {
        LOGE("Socket Parcel failed: %s", ex.what());
        return jniutil::to_jstring(env, "Socket Parcel failed");
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_myapplication_MainActivity_nativeRunSerializationBenchmark(
        JNIEnv* env,
        jobject /* this */,
        jint thread_count,
        jint iterations,
        jboolean use_parcel) {
    LOGI("[Flow:BenchSerialization] threads=%d, iterations=%d, parcel=%d, pid=%d", thread_count,
         iterations, use_parcel, getpid());
    const auto stats = ipc::run_serialization_benchmark(
        use_parcel ? ipc::SerializationKind::kParcel : ipc::SerializationKind::kCustom,
        thread_count,
        iterations);
    const auto prefix = use_parcel ? "Serialization(Parcel)" : "Serialization(Custom)";
    return jniutil::to_jstring(env, prefix + std::string(": ") + stats.format());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_myapplication_MainActivity_nativeRunSocketBenchmark(
        JNIEnv* env,
        jobject /* this */,
        jint thread_count,
        jint iterations,
        jint mode) {
    LOGI("[Flow:BenchSocket] threads=%d, iterations=%d, mode=%d, pid=%d", thread_count,
         iterations, mode, getpid());
    const auto benchmark_mode = static_cast<ipc::SocketBenchmarkMode>(mode);
    const auto stats =
        ipc::run_socket_benchmark(benchmark_mode, thread_count, iterations);
    return jniutil::to_jstring(env, std::string("Socket Benchmark: ") + stats.format());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_myapplication_ipc_NativeIpc_nativeDescribePayload(
        JNIEnv* env,
        jclass /* clazz */,
        jbyteArray data,
        jboolean use_parcel) {
    try {
        const auto bytes = jniutil::to_bytes(env, data);
        const auto profile = use_parcel ? ipc::ParcelCodec::decode(bytes)
                                        : ipc::CustomCodec::decode(bytes);
        return jniutil::to_jstring(env, describe_profile(profile));
    } catch (const std::exception& ex) {
        return jniutil::to_jstring(env, ex.what());
    }
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_example_myapplication_ipc_NativeIpc_nativeProcessCustomPayload(
        JNIEnv* env,
        jclass /* clazz */,
        jbyteArray data) {
    LOGI("[Flow:BinderComplex] nativeProcessCustomPayload, pid=%d", getpid());
    const auto bytes = jniutil::to_bytes(env, data);
    const auto processed = ipc::CustomCodec::process(bytes);
    return jniutil::to_jbyte_array(env, processed);
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_example_myapplication_ipc_NativeIpc_nativeProcessParcelPayload(
        JNIEnv* env,
        jclass /* clazz */,
        jbyteArray data) {
    LOGI("[Flow:BinderComplex] nativeProcessParcelPayload, pid=%d", getpid());
    const auto bytes = jniutil::to_bytes(env, data);
    const auto processed = ipc::ParcelCodec::process(bytes);
    return jniutil::to_jbyte_array(env, processed);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_myapplication_ipc_SocketServerService_nativeStartSocketServer(
        JNIEnv* /* env */,
        jobject /* this */) {
    LOGI("[Flow:SocketServer] nativeStartSocketServer, pid=%d", getpid());
    const auto ok = ipc::global_socket_server().start("socketipc_calc");
    LOGI("[Flow:SocketServer] start 返回 %s", ok ? "true" : "false");
    return ok ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_myapplication_ipc_SocketServerService_nativeStopSocketServer(
        JNIEnv* /* env */,
        jobject /* this */) {
    LOGI("[Flow:SocketServer] nativeStopSocketServer, pid=%d", getpid());
    ipc::global_socket_server().stop();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_myapplication_ipc_SocketServerService_nativeGetProcessId(
        JNIEnv* /* env */,
        jobject /* this */) {
    return getpid();
}
