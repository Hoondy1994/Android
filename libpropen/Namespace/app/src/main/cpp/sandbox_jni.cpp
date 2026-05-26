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

#define LOG_TAG "SandboxDemo"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static std::string AppendLine(std::string &out, const std::string &line) {
    out += line;
    out += '\n';
    return out;
}

static bool WriteTextFile(const std::string &path, const char *content) {
    FILE *fp = fopen(path.c_str(), "w");
    if (fp == nullptr) {
        return false;
    }
    fprintf(fp, "%s", content);
    fclose(fp);
    return true;
}

static std::string FormatOpenResult(const char *label, int fd) {
    if (fd >= 0) {
        close(fd);
        return std::string(label) + " -> fd=" + std::to_string(fd) + " (成功，允许访问)";
    }
    return std::string(label) + " -> fd=" + std::to_string(fd) + " errno=" +
           std::to_string(errno) + " (失败，被沙箱拒绝)";
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_myapplication_MainActivity_runSandboxDemo(JNIEnv *env, jobject,
                                                           jstring baseDirJ) {
    const char *baseDirChars = env->GetStringUTFChars(baseDirJ, nullptr);
    if (baseDirChars == nullptr) {
        return env->NewStringUTF("错误: baseDir 为空");
    }

    const std::string baseDir(baseDirChars);
    env->ReleaseStringUTFChars(baseDirJ, baseDirChars);

    const std::string pluginADir = baseDir + "/plugin_a";
    const std::string pluginBDir = baseDir + "/plugin_b";
    const std::string fileA = pluginADir + "/secret_a.txt";
    const std::string fileB = pluginBDir + "/secret_b.txt";

    std::string result;
    AppendLine(result, "========== 插件文件沙箱演示 ==========");
    AppendLine(result, "");
    AppendLine(result, "场景: 同一 App 内两个 JS 插件 A/B，各自只能访问自己的虚拟目录");
    AppendLine(result, "Android 系统沙箱(UID)无法区分同进程内的插件，需要 Namespace 补一层");
    AppendLine(result, "");

    if (mkdir(baseDir.c_str(), 0755) != 0 && errno != EEXIST) {
        return env->NewStringUTF(("无法创建目录: " + baseDir).c_str());
    }
    if (mkdir(pluginADir.c_str(), 0755) != 0 && errno != EEXIST) {
        return env->NewStringUTF(("无法创建 plugin_a: " + pluginADir).c_str());
    }
    if (mkdir(pluginBDir.c_str(), 0755) != 0 && errno != EEXIST) {
        return env->NewStringUTF(("无法创建 plugin_b: " + pluginBDir).c_str());
    }

    if (!WriteTextFile(fileA, "PLUGIN-A-SECRET") ||
        !WriteTextFile(fileB, "PLUGIN-B-SECRET")) {
        return env->NewStringUTF("无法写入测试文件");
    }

    constexpr uint64_t kPluginAId = 1000;
    constexpr uint64_t kPluginBId = 2000;
    const char *kVirtualA = "/pluginA";
    const char *kVirtualB = "/pluginB";

    const int mapA = Seqmap::GetInstance().InsertMap(kPluginAId, pluginADir.c_str(), kVirtualA);
    const int mapB = Seqmap::GetInstance().InsertMap(kPluginBId, pluginBDir.c_str(), kVirtualB);

    AppendLine(result, "【1】宿主登记目录 (InsertMap)");
    AppendLine(result, "  插件A id=1000: " + pluginADir + " => " + kVirtualA +
                            " (ret=" + std::to_string(mapA) + ")");
    AppendLine(result, "  插件B id=2000: " + pluginBDir + " => " + kVirtualB +
                            " (ret=" + std::to_string(mapB) + ")");
    AppendLine(result, "");

    AppendLine(result, "【2】有沙箱: 插件通过 open_ + 虚拟路径访问");
    AppendLine(result, "  " + FormatOpenResult("插件A 读 /pluginA/secret_a.txt",
                                               open_(kPluginAId, "/pluginA/secret_a.txt", O_RDONLY)));
    AppendLine(result, "  " + FormatOpenResult("插件A 偷读 /pluginB/secret_b.txt",
                                               open_(kPluginAId, "/pluginB/secret_b.txt", O_RDONLY)));
    AppendLine(result, "  " + FormatOpenResult("插件B 读 /pluginB/secret_b.txt",
                                               open_(kPluginBId, "/pluginB/secret_b.txt", O_RDONLY)));
    AppendLine(result, "  " + FormatOpenResult("插件B 偷读 /pluginA/secret_a.txt",
                                               open_(kPluginBId, "/pluginA/secret_a.txt", O_RDONLY)));
    AppendLine(result, "");

    AppendLine(result, "【3】无沙箱: 直接用系统 open() 读真实绝对路径");
    AppendLine(result, "  (同一 App 进程内，Android 不会按插件隔离)");
    errno = 0;
    const int rawFd = open(fileB.c_str(), O_RDONLY);
    AppendLine(result, "  " + FormatOpenResult(("插件A 用 open(\"" + fileB + "\")").c_str(), rawFd));
    if (rawFd >= 0) {
        AppendLine(result, "  => 说明: 若不强制走 open_，插件可越权读取其它插件目录");
    }
    AppendLine(result, "");

    AppendLine(result, "【结论】");
    AppendLine(result, "  open_ + 虚拟路径 + serviceId = 进程内插件文件沙箱");
    AppendLine(result, "  未映射的虚拟路径访问失败 = 权限控制生效");

    ALOGI("%s", result.c_str());
    return env->NewStringUTF(result.c_str());
}
