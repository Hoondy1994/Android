#include <jni.h>

#include <android/log.h>

#include <errno.h>
#include <fcntl.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <signal.h>
#include <string>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#define LOG_TAG "SandboxDemo"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

constexpr int kExitSeccompInstallFailed = 100;
constexpr int kExitOpenBlocked = 1;
constexpr int kExitOpenSuccess = 0;

int installOpenBlockFilter() {
    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) {
        return -1;
    }

    struct sock_filter filter[] = {
            BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr)),
            BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_openat, 0, 1),
            BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EPERM),
            BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
    };

    const unsigned short filterLen =
            static_cast<unsigned short>(sizeof(filter) / sizeof(filter[0]));
    struct sock_fprog prog = {
            .len = filterLen,
            .filter = filter,
    };

    return static_cast<int>(syscall(__NR_seccomp, SECCOMP_MODE_FILTER, &prog, nullptr));
}

std::string readFilePreview(const char *path) {
    int fd = openat(AT_FDCWD, path, O_RDONLY);
    if (fd < 0) {
        return std::string("open 失败: ") + strerror(errno);
    }

    char buffer[128] = {};
    ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    if (bytes < 0) {
        return std::string("read 失败: ") + strerror(errno);
    }

    return std::string(buffer, static_cast<size_t>(bytes));
}

int childTryOpen(const char *path) {
    if (installOpenBlockFilter() != 0) {
        return kExitSeccompInstallFailed;
    }

    int fd = openat(AT_FDCWD, path, O_RDONLY);
    if (fd < 0) {
        return kExitOpenBlocked;
    }

    close(fd);
    return kExitOpenSuccess;
}

std::string decodeChildStatus(int status) {
    if (WIFEXITED(status)) {
        switch (WEXITSTATUS(status)) {
            case kExitOpenSuccess:
                return "意外成功：Seccomp 未拦截 openat()";
            case kExitOpenBlocked:
                return "拦截成功：openat() 返回 EPERM，系统调用被 Seccomp 沙箱拒绝";
            case kExitSeccompInstallFailed:
                return "错误：子进程无法安装 Seccomp 过滤器";
            default:
                return "子进程异常退出，code=" + std::to_string(WEXITSTATUS(status));
        }
    }

    if (WIFSIGNALED(status)) {
        const int signal = WTERMSIG(status);
        if (signal == SIGSYS) {
            return "拦截成功：触发 SIGSYS，非法系统调用被内核终止";
        }
        return "子进程被信号终止: " + std::string(strsignal(signal));
    }

    return "未知子进程状态";
}

}  // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_san_SandboxNative_runNativeFileRead(
        JNIEnv *env,
        jobject /* this */,
        jstring jPath) {
    const char *path = env->GetStringUTFChars(jPath, nullptr);
    if (path == nullptr) {
        return env->NewStringUTF("路径为空");
    }

    const std::string preview = readFilePreview(path);
    env->ReleaseStringUTFChars(jPath, path);

    if (preview.rfind("open 失败", 0) == 0 || preview.rfind("read 失败", 0) == 0) {
        return env->NewStringUTF(preview.c_str());
    }

    const std::string result =
            "成功读取 " + std::to_string(preview.size()) + " 字节: \"" + preview + "\"";
    return env->NewStringUTF(result.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_san_SandboxNative_runSeccompBlockedRead(
        JNIEnv *env,
        jobject /* this */,
        jstring jPath) {
    const char *path = env->GetStringUTFChars(jPath, nullptr);
    if (path == nullptr) {
        return env->NewStringUTF("路径为空");
    }

    const pid_t pid = fork();
    if (pid < 0) {
        env->ReleaseStringUTFChars(jPath, path);
        return env->NewStringUTF("fork 失败，无法创建隔离子进程");
    }

    if (pid == 0) {
        _exit(childTryOpen(path));
    }

    int status = 0;
    waitpid(pid, &status, 0);
    env->ReleaseStringUTFChars(jPath, path);

    const std::string result = decodeChildStatus(status);
    return env->NewStringUTF(result.c_str());
}
