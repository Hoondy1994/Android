#pragma once
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <shared_mutex>
#include <functional>
#include <optional>
#include <sstream>
#include <atomic>
#include "ams_types.h"
#include "intent.h"
#include "looper.h"
#include "activity_record.h"
#include "process_record.h"
#include "task_record.h"
#include "activity_stack.h"
#include "broadcast_record.h"
#include "service_record.h"

namespace android::ams {

// ─── RunningTaskInfo (mirrors ActivityManager.RunningTaskInfo) ─────────────
struct RunningTaskInfo {
    TaskId      id;
    std::string topActivity;
    std::string baseActivity;
    int32_t     numActivities;
    int64_t     lastActiveTime;
};

// ─── RunningAppProcessInfo ─────────────────────────────────────────────────
struct RunningAppProcessInfo {
    std::string  processName;
    Pid          pid;
    Uid          uid;
    ProcessState importance;
    int32_t      oomAdj;
    int64_t      pssKb;
};

// ─── AMS event listener ────────────────────────────────────────────────────
struct AMSListener {
    std::function<void(const std::string& log)> onLog;
    std::function<void(ActivityRecord&)>        onActivityCreated;
    std::function<void(ActivityRecord&)>        onActivityDestroyed;
    std::function<void(BroadcastRecord&)>       onBroadcastSent;
};

// ─── ActivityManagerService ────────────────────────────────────────────────
class ActivityManagerService {
public:
    static ActivityManagerService& getInstance();

    // Non-copyable / non-movable singleton
    ActivityManagerService(const ActivityManagerService&)            = delete;
    ActivityManagerService& operator=(const ActivityManagerService&) = delete;

    // ── Lifecycle ──────────────────────────────────────────────────────────
    void start();
    void shutdown();

    // ── Activity management ────────────────────────────────────────────────
    AMSResult startActivity(const Intent& intent,
                            LaunchMode mode   = LaunchMode::STANDARD,
                            Uid callerUid     = SYSTEM_UID,
                            UserId userId     = 0);

    AMSResult finishActivity(ActivityId id);
    AMSResult finishActivityByName(const std::string& className);
    AMSResult moveTaskToFront(TaskId taskId);
    AMSResult moveTaskToBack(TaskId taskId);

    // ── Process management ─────────────────────────────────────────────────
    std::shared_ptr<ProcessRecord> getOrCreateProcess(
        const std::string& name, Uid uid, UserId userId = 0);
    bool killProcess(Pid pid, std::string_view reason);
    void killProcessesInPackage(const std::string& pkg, std::string_view reason);

    // ── Service management ─────────────────────────────────────────────────
    AMSResult startService(const std::string& pkg, const std::string& cls,
                           Uid callerUid = SYSTEM_UID);
    AMSResult stopService(const std::string& pkg, const std::string& cls);
    AMSResult startForegroundService(const std::string& pkg, const std::string& cls,
                                     const std::string& notifTitle);

    // ── Broadcast ──────────────────────────────────────────────────────────
    void sendBroadcast(Intent intent, Uid callerUid = SYSTEM_UID);
    void sendOrderedBroadcast(Intent intent, Uid callerUid = SYSTEM_UID);
    void registerReceiver(BroadcastReceiver rcv);
    void unregisterReceiver(const std::string& pkg, const std::string& cls);

    // ── Queries ────────────────────────────────────────────────────────────
    [[nodiscard]] std::vector<RunningTaskInfo>       getRunningTasks(int32_t maxNum = 10) const;
    [[nodiscard]] std::vector<RunningAppProcessInfo> getRunningAppProcesses() const;
    [[nodiscard]] std::optional<RunningTaskInfo>     getForegroundTaskInfo() const;
    [[nodiscard]] std::shared_ptr<ActivityRecord>    getTopActivity() const;

    // ── Observer ───────────────────────────────────────────────────────────
    void setListener(AMSListener listener) { listener_ = std::move(listener); }

    // ── Dump state ─────────────────────────────────────────────────────────
    [[nodiscard]] std::string dumpState() const;
    [[nodiscard]] std::string dumpActivities() const;
    [[nodiscard]] std::string dumpProcesses() const;
    [[nodiscard]] std::string dumpServices() const;
    [[nodiscard]] std::string dumpBroadcasts() const;

private:
    ActivityManagerService();
    ~ActivityManagerService();

    // ── Internal helpers ───────────────────────────────────────────────────
    void handleMessage(const Message& msg);
    void dispatchBroadcast(std::shared_ptr<BroadcastRecord> rec);

    AMSResult startActivityLocked(const Intent& intent, LaunchMode mode,
                                  Uid callerUid, UserId userId);
    void resumeTopActivity();
    void pauseActivity(std::shared_ptr<ActivityRecord> act);

    void log(std::string msg) const;

    // ── State ──────────────────────────────────────────────────────────────
    mutable std::shared_mutex   rwLock_;
    std::unique_ptr<Looper>     looper_;
    std::unique_ptr<ActivityStack> fullscreenStack_;   // primary stack

    // pid → ProcessRecord
    std::unordered_map<Pid, std::shared_ptr<ProcessRecord>> procs_;
    // className → ServiceRecord
    std::unordered_map<std::string, std::shared_ptr<ServiceRecord>> services_;
    // registered receivers
    std::vector<BroadcastReceiver> receivers_;
    // recent broadcasts
    std::vector<std::shared_ptr<BroadcastRecord>> recentBroadcasts_;

    AMSListener listener_;
    std::atomic<bool> started_{false};
};

} // namespace android::ams
