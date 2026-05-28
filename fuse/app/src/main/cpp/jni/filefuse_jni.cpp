#include "../backends/local_backend.h"
#include "../backends/nfs/nfs_backend.h"
#include "../fuse/fuse_adapter.h"
#include "../vfs/vfs.h"

#include <android/log.h>
#include <jni.h>
#include <sys/stat.h>

#include <memory>
#include <sstream>
#include <string>

#define LOG_TAG "filefuse"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace {

filefuse::FuseAdapter& adapter() {
    static filefuse::FuseAdapter instance(filefuse::globalVfs());
    return instance;
}

jstring makeStatusString(JNIEnv* env) {
    std::ostringstream stream;
    stream << "backend=" << filefuse::globalVfs().backendName()
           << ", mounted=" << (filefuse::globalVfs().isMounted() ? "yes" : "no");
    return env->NewStringUTF(stream.str().c_str());
}

std::vector<uint8_t> readFileBytes(const std::string& filePath) {
    struct stat st {};
    if (adapter().getattr(filePath, &st) != 0 || !S_ISREG(st.st_mode)) {
        return {};
    }

    std::vector<uint8_t> buffer(static_cast<size_t>(st.st_size));
    const int readBytes =
            adapter().read(filePath, reinterpret_cast<char*>(buffer.data()), buffer.size(), 0);
    if (readBytes < 0) {
        return {};
    }
    buffer.resize(static_cast<size_t>(readBytes));
    return buffer;
}

}  // namespace

extern "C" JNIEXPORT jint JNICALL
Java_com_example_filefuse_FileFuseNative_nativeMountLocal(
        JNIEnv* env,
        jobject /* thiz */,
        jstring rootPath) {
    const char* pathChars = env->GetStringUTFChars(rootPath, nullptr);
    const std::string path(pathChars ? pathChars : "");
    env->ReleaseStringUTFChars(rootPath, pathChars);

    auto backend = std::make_unique<filefuse::LocalBackend>(path);
    const filefuse::VfsError result = filefuse::globalVfs().mount(std::move(backend));
    LOGI("mount local %s -> %d", path.c_str(), static_cast<int>(result));
    return static_cast<jint>(result);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_filefuse_FileFuseNative_nativeMountNfs(
        JNIEnv* env,
        jobject /* thiz */,
        jstring host,
        jstring exportPath,
        jint port) {
    const char* hostChars = env->GetStringUTFChars(host, nullptr);
    const char* exportChars = env->GetStringUTFChars(exportPath, nullptr);
    const std::string hostStr(hostChars ? hostChars : "");
    const std::string exportStr(exportChars ? exportChars : "");
    env->ReleaseStringUTFChars(host, hostChars);
    env->ReleaseStringUTFChars(exportPath, exportChars);

    auto backend = std::make_unique<filefuse::NfsBackend>(
            hostStr, exportStr, static_cast<uint16_t>(port <= 0 ? 2049 : port));
    const filefuse::VfsError result = filefuse::globalVfs().mount(std::move(backend));
    LOGI("mount nfs %s:%s -> %d", hostStr.c_str(), exportStr.c_str(), static_cast<int>(result));
    return static_cast<jint>(result);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_filefuse_FileFuseNative_nativeUnmount(
        JNIEnv* /* env */,
        jobject /* thiz */) {
    filefuse::globalVfs().unmount();
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_filefuse_FileFuseNative_nativeGetStatus(
        JNIEnv* env,
        jobject /* thiz */) {
    return makeStatusString(env);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_filefuse_FileFuseNative_nativeListDir(
        JNIEnv* env,
        jobject /* thiz */,
        jstring path) {
    const char* pathChars = env->GetStringUTFChars(path, nullptr);
    const std::string dirPath(pathChars ? pathChars : "/");
    env->ReleaseStringUTFChars(path, pathChars);

    std::vector<filefuse::FuseDirEntry> entries;
    const int result = adapter().readdir(dirPath, entries);
    if (result != 0) {
        return env->NewStringUTF("[]");
    }

    std::ostringstream json;
    json << "[";
    for (size_t i = 0; i < entries.size(); ++i) {
        if (i > 0) {
            json << ",";
        }
        json << "{\"name\":\"" << entries[i].name << "\",\"dir\":"
             << (entries[i].type == 4 ? "true" : "false") << "}";
    }
    json << "]";
    return env->NewStringUTF(json.str().c_str());
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_example_filefuse_FileFuseNative_nativeReadFile(
        JNIEnv* env,
        jobject /* thiz */,
        jstring path) {
    const char* pathChars = env->GetStringUTFChars(path, nullptr);
    const std::string filePath(pathChars ? pathChars : "/");
    env->ReleaseStringUTFChars(path, pathChars);

    const std::vector<uint8_t> buffer = readFileBytes(filePath);
    if (buffer.empty()) {
        return nullptr;
    }

    jbyteArray result = env->NewByteArray(static_cast<jsize>(buffer.size()));
    if (result != nullptr) {
        env->SetByteArrayRegion(result, 0, static_cast<jsize>(buffer.size()),
                                reinterpret_cast<const jbyte*>(buffer.data()));
    }
    return result;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_filefuse_FileFuseNative_nativeReadTextFile(
        JNIEnv* env,
        jobject /* thiz */,
        jstring path) {
    const char* pathChars = env->GetStringUTFChars(path, nullptr);
    const std::string filePath(pathChars ? pathChars : "/");
    env->ReleaseStringUTFChars(path, pathChars);

    const std::vector<uint8_t> buffer = readFileBytes(filePath);
    if (buffer.empty()) {
        return env->NewStringUTF("");
    }

    return env->NewStringUTF(
            std::string(buffer.begin(), buffer.end()).c_str());
}
