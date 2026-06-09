#include <jni.h>
#include <string>
#include <sstream>
#include <mutex>
#include <deque>
#include <android/log.h>

#include "ams/activity_manager_service.h"

using namespace android::ams;

// ─── Global log ring-buffer (JNI side) ───────────────────────────────────────
static std::mutex         g_logMtx;
static std::deque<std::string> g_logBuf;
static constexpr size_t   MAX_LOG = 200;

static void appendLog(const std::string& line) {
    std::lock_guard lk(g_logMtx);
    g_logBuf.push_back(line);
    if (g_logBuf.size() > MAX_LOG) g_logBuf.pop_front();
}

static std::string flushLog() {
    std::lock_guard lk(g_logMtx);
    std::ostringstream ss;
    for (auto& l : g_logBuf) ss << l << "\n";
    return ss.str();
}

static std::string tailLog(int n) {
    std::lock_guard lk(g_logMtx);
    std::ostringstream ss;
    int start = std::max(0, (int)g_logBuf.size() - n);
    for (int i = start; i < (int)g_logBuf.size(); ++i) {
        ss << g_logBuf[i] << "\n";
    }
    return ss.str();
}

// ─── Helper: convert std::string to jstring ───────────────────────────────────
static jstring toJString(JNIEnv* env, const std::string& s) {
    return env->NewStringUTF(s.c_str());
}

// ─────────────────────────────────────────────────────────────────────────────
extern "C" {

// ── nativeInit ───────────────────────────────────────────────────────────────
JNIEXPORT void JNICALL
Java_com_example_amstest_MainActivity_nativeInit(JNIEnv*, jobject) {
    auto& ams = ActivityManagerService::getInstance();

    AMSListener listener;
    listener.onLog = [](const std::string& msg) {
        appendLog("[AMS] " + msg);
    };
    listener.onActivityCreated = [](ActivityRecord& act) {
        appendLog("[AMS] Activity CREATED: " + act.className());
    };
    listener.onActivityDestroyed = [](ActivityRecord& act) {
        appendLog("[AMS] Activity DESTROYED: " + act.className());
    };
    listener.onBroadcastSent = [](BroadcastRecord& bc) {
        appendLog("[AMS] Broadcast: " + bc.intent.action());
    };
    ams.setListener(std::move(listener));
    ams.start();
}

// ── nativeStartActivity ───────────────────────────────────────────────────────
JNIEXPORT jint JNICALL
Java_com_example_amstest_MainActivity_nativeStartActivity(
        JNIEnv* env, jobject,
        jstring jPkg, jstring jCls, jstring jAction, jint launchMode)
{
    const char* pkg    = env->GetStringUTFChars(jPkg,    nullptr);
    const char* cls    = env->GetStringUTFChars(jCls,    nullptr);
    const char* action = env->GetStringUTFChars(jAction, nullptr);

    Intent intent(action);
    intent.setComponent(pkg, cls);

    LaunchMode mode = static_cast<LaunchMode>(launchMode);
    auto result = ActivityManagerService::getInstance()
                      .startActivity(intent, mode);

    env->ReleaseStringUTFChars(jPkg,    pkg);
    env->ReleaseStringUTFChars(jCls,    cls);
    env->ReleaseStringUTFChars(jAction, action);

    return static_cast<jint>(result);
}

// ── nativeFinishActivity ──────────────────────────────────────────────────────
JNIEXPORT jint JNICALL
Java_com_example_amstest_MainActivity_nativeFinishActivity(
        JNIEnv* env, jobject, jstring jCls)
{
    const char* cls = env->GetStringUTFChars(jCls, nullptr);
    auto result = ActivityManagerService::getInstance()
                      .finishActivityByName(cls);
    env->ReleaseStringUTFChars(jCls, cls);
    return static_cast<jint>(result);
}

// ── nativeStartService ────────────────────────────────────────────────────────
JNIEXPORT jint JNICALL
Java_com_example_amstest_MainActivity_nativeStartService(
        JNIEnv* env, jobject, jstring jPkg, jstring jCls, jboolean foreground)
{
    const char* pkg = env->GetStringUTFChars(jPkg, nullptr);
    const char* cls = env->GetStringUTFChars(jCls, nullptr);

    AMSResult result;
    if (foreground) {
        result = ActivityManagerService::getInstance()
                     .startForegroundService(pkg, cls, "Service running");
    } else {
        result = ActivityManagerService::getInstance()
                     .startService(pkg, cls);
    }
    env->ReleaseStringUTFChars(jPkg, pkg);
    env->ReleaseStringUTFChars(jCls, cls);
    return static_cast<jint>(result);
}

// ── nativeStopService ─────────────────────────────────────────────────────────
JNIEXPORT jint JNICALL
Java_com_example_amstest_MainActivity_nativeStopService(
        JNIEnv* env, jobject, jstring jPkg, jstring jCls)
{
    const char* pkg = env->GetStringUTFChars(jPkg, nullptr);
    const char* cls = env->GetStringUTFChars(jCls, nullptr);
    auto result = ActivityManagerService::getInstance().stopService(pkg, cls);
    env->ReleaseStringUTFChars(jPkg, pkg);
    env->ReleaseStringUTFChars(jCls, cls);
    return static_cast<jint>(result);
}

// ── nativeSendBroadcast ───────────────────────────────────────────────────────
JNIEXPORT void JNICALL
Java_com_example_amstest_MainActivity_nativeSendBroadcast(
        JNIEnv* env, jobject, jstring jAction)
{
    const char* action = env->GetStringUTFChars(jAction, nullptr);
    ActivityManagerService::getInstance().sendBroadcast(Intent(action));
    env->ReleaseStringUTFChars(jAction, action);
}

// ── nativeGetTopActivity ──────────────────────────────────────────────────────
JNIEXPORT jstring JNICALL
Java_com_example_amstest_MainActivity_nativeGetTopActivity(JNIEnv* env, jobject)
{
    auto act = ActivityManagerService::getInstance().getTopActivity();
    if (!act) return toJString(env, "(none)");
    return toJString(env, act->packageName() + "/" + act->shortName()
                          + " [" + std::string(to_string(act->state())) + "]");
}

// ── nativeDumpState ───────────────────────────────────────────────────────────
JNIEXPORT jstring JNICALL
Java_com_example_amstest_MainActivity_nativeDumpState(JNIEnv* env, jobject)
{
    return toJString(env, ActivityManagerService::getInstance().dumpState());
}

// ── nativeGetRunningTasks ─────────────────────────────────────────────────────
JNIEXPORT jstring JNICALL
Java_com_example_amstest_MainActivity_nativeGetRunningTasks(
        JNIEnv* env, jobject, jint maxNum)
{
    auto tasks = ActivityManagerService::getInstance().getRunningTasks(maxNum);
    std::ostringstream ss;
    for (auto& t : tasks) {
        ss << "Task#" << t.id
           << " top=" << t.topActivity
           << " acts=" << t.numActivities << "\n";
    }
    return toJString(env, ss.str());
}

// ── nativeGetRunningProcesses ─────────────────────────────────────────────────
JNIEXPORT jstring JNICALL
Java_com_example_amstest_MainActivity_nativeGetRunningProcesses(JNIEnv* env, jobject)
{
    auto procs = ActivityManagerService::getInstance().getRunningAppProcesses();
    std::ostringstream ss;
    for (auto& p : procs) {
        ss << "pid=" << p.pid
           << " " << p.processName
           << " (" << to_string(p.importance) << ")"
           << " pss=" << p.pssKb << "KB\n";
    }
    return toJString(env, ss.str());
}

// ── nativeGetLogs ─────────────────────────────────────────────────────────────
JNIEXPORT jstring JNICALL
Java_com_example_amstest_MainActivity_nativeGetLogs(JNIEnv* env, jobject, jint lines)
{
    return toJString(env, tailLog(lines));
}

// ── nativeMoveTaskToFront ─────────────────────────────────────────────────────
JNIEXPORT jint JNICALL
Java_com_example_amstest_MainActivity_nativeMoveTaskToFront(
        JNIEnv*, jobject, jint taskId)
{
    return static_cast<jint>(
        ActivityManagerService::getInstance().moveTaskToFront(taskId));
}

// ── nativeShutdown ────────────────────────────────────────────────────────────
JNIEXPORT void JNICALL
Java_com_example_amstest_MainActivity_nativeShutdown(JNIEnv*, jobject)
{
    ActivityManagerService::getInstance().shutdown();
}

} // extern "C"
