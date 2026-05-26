#include <jni.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <string>

#include <android/log.h>
#include <libseqmap.h>
#include <libc_wrapper.h>

#define LOG_TAG "PreopenDemo"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static constexpr uint64_t kDemoMapId = 1000;
static constexpr const char *kVirtualPrefix = "/vdemo";

JNIEXPORT jint JNI_OnLoad(JavaVM *, void *) {
    ALOGE("libpreopen_demo JNI_OnLoad OK");
    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_oplus_pantanal_namespace_demo_PreopenBridge_runDemo(JNIEnv *env, jclass,
                                                             jstring baseDirJ) {
    ALOGI("runDemo entered");
    const char *baseDirChars = env->GetStringUTFChars(baseDirJ, nullptr);
    if (baseDirChars == nullptr) {
        return env->NewStringUTF("错误: baseDir 为空");
    }

    const std::string baseDir(baseDirChars);
    env->ReleaseStringUTFChars(baseDirJ, baseDirChars);
    ALOGI("runDemo baseDir=%s", baseDir.c_str());

    const std::string realRoot = baseDir + "/real_root";
    const std::string testFile = realRoot + "/hello.txt";
    const std::string virtualAllowed = std::string(kVirtualPrefix) + "/hello.txt";
    const std::string virtualDenied = "/outside/no_permission.txt";

    if (mkdir(realRoot.c_str(), 0755) != 0 && errno != EEXIST) {
        ALOGE("mkdir failed realRoot=%s errno=%d", realRoot.c_str(), errno);
        return env->NewStringUTF(
            ("错误: 无法创建目录 " + realRoot + " errno=" + std::to_string(errno)).c_str());
    }
    ALOGI("mkdir ok realRoot=%s", realRoot.c_str());

    FILE *fp = fopen(testFile.c_str(), "w");
    if (fp == nullptr) {
        ALOGE("fopen failed testFile=%s errno=%d", testFile.c_str(), errno);
        return env->NewStringUTF(
            ("错误: 无法写入测试文件 " + testFile + " errno=" + std::to_string(errno)).c_str());
    }
    ALOGI("fopen ok testFile=%s", testFile.c_str());
    fprintf(fp, "preopen-demo-ok");
    fclose(fp);

    ALOGE("before InsertMap realRoot=%s virtualPrefix=%s mapId=%llu",
          realRoot.c_str(), kVirtualPrefix,
          static_cast<unsigned long long>(kDemoMapId));
    const int insertRet =
        Seqmap::GetInstance().InsertMap(kDemoMapId, realRoot.c_str(), kVirtualPrefix);
    ALOGE("after InsertMap insertRet=%d realRoot=%s", insertRet, realRoot.c_str());

    const int fdAllowed = open_(kDemoMapId, virtualAllowed.c_str(), O_RDONLY);
    char readBuf[64] = {};
    if (fdAllowed >= 0) {
        read(fdAllowed, readBuf, sizeof(readBuf) - 1);
        close(fdAllowed);
    }

    const int fdDenied = open_(kDemoMapId, virtualDenied.c_str(), O_RDONLY);
    const int accessAllowed = access_(kDemoMapId, virtualAllowed.c_str(), F_OK);
    const int accessDenied = access_(kDemoMapId, virtualDenied.c_str(), F_OK);

    std::string result;
    result += "=== libpreopen 权限控制 Demo ===\n\n";
    result += "真实目录: " + realRoot + "\n";
    result += "虚拟前缀: " + std::string(kVirtualPrefix) + "\n";
    result += "InsertMap 返回: " + std::to_string(insertRet) + " (0=成功)\n\n";

    result += "[允许] 虚拟路径 " + virtualAllowed + "\n";
    result += "  open_ fd=" + std::to_string(fdAllowed);
    result += fdAllowed >= 0 ? " (成功)\n" : " (失败)\n";
    result += "  读取内容: " + std::string(readBuf) + "\n";
    result += "  access_=" + std::to_string(accessAllowed) + "\n\n";

    result += "[拒绝] 未映射路径 " + virtualDenied + "\n";
    result += "  open_ fd=" + std::to_string(fdDenied);
    result += fdDenied < 0 ? " (失败，权限控制生效)\n" : " (意外成功)\n";
    result += "  access_=" + std::to_string(accessDenied);
    result += accessDenied != 0 ? " (不可访问)\n" : " (意外可访问)\n";

    ALOGI("%s", result.c_str());
    return env->NewStringUTF(result.c_str());
}
