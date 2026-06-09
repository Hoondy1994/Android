#include "activity_manager_service.h"
#include <algorithm>
#include <cassert>
#include <android/log.h>

#define AMS_TAG "AMS_Native"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO,  AMS_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN,  AMS_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, AMS_TAG, __VA_ARGS__)

namespace android::ams {

// ─── Singleton ───────────────────────────────────────────────────────────────
ActivityManagerService& ActivityManagerService::getInstance() {
    static ActivityManagerService instance;
    return instance;
}

ActivityManagerService::ActivityManagerService()
    : fullscreenStack_(std::make_unique<ActivityStack>(ActivityStack::StackId::FULLSCREEN)) {

    // Register a few built-in broadcast receivers
    BroadcastReceiver bootReceiver;
    bootReceiver.packageName   = "com.android.launcher3";
    bootReceiver.receiverClass = "com.android.launcher3.BootReceiver";
    bootReceiver.actions       = {IntentAction::BOOT_COMPLETED};
    bootReceiver.onReceive     = [this](const Intent& i) {
        log("[BootReceiver] Boot completed, action=" + i.action());
    };
    receivers_.push_back(std::move(bootReceiver));

    BroadcastReceiver battReceiver;
    battReceiver.packageName   = "com.android.settings";
    battReceiver.receiverClass = "com.android.settings.BatteryReceiver";
    battReceiver.actions       = {IntentAction::BATTERY_LOW};
    battReceiver.onReceive     = [this](const Intent& i) {
        log("[BatteryReceiver] Battery low warning!");
    };
    receivers_.push_back(std::move(battReceiver));
}

ActivityManagerService::~ActivityManagerService() {
    shutdown();
}

// ─── Lifecycle ────────────────────────────────────────────────────────────────
void ActivityManagerService::start() {
    if (started_.exchange(true)) return;

    looper_ = std::make_unique<Looper>([this](const Message& m){ handleMessage(m); });
    looper_->start();

    // Create home task / process
    {
        std::unique_lock lk(rwLock_);
        auto launcher = getOrCreateProcess("com.android.launcher3", SYSTEM_UID, 0);
        auto homeTask = fullscreenStack_->createTask("com.android.launcher3", 0);
        homeTask->setHome(true);

        Intent homeIntent(IntentAction::MAIN);
        homeIntent.setComponent("com.android.launcher3", "com.android.launcher3.Launcher");

        auto launcherAct = std::make_shared<ActivityRecord>(
            "com.android.launcher3",
            "com.android.launcher3.Launcher",
            homeIntent, LaunchMode::SINGLE_TASK, launcher);

        launcher->addActivity(launcherAct);
        homeTask->push(launcherAct);
        launcherAct->onCreate();
        launcherAct->onResume();
        launcherAct->setVisible(true);
    }

    log("ActivityManagerService started");

    // Send boot completed broadcast
    sendBroadcast(Intent(IntentAction::BOOT_COMPLETED));
}

void ActivityManagerService::shutdown() {
    if (!started_.exchange(false)) return;
    if (looper_) { looper_->stop(); looper_.reset(); }
    log("ActivityManagerService shut down");
}

// ─── Start Activity ───────────────────────────────────────────────────────────
AMSResult ActivityManagerService::startActivity(
    const Intent& intent, LaunchMode mode, Uid callerUid, UserId userId)
{
    std::unique_lock lk(rwLock_);
    return startActivityLocked(intent, mode, callerUid, userId);
}

AMSResult ActivityManagerService::startActivityLocked(
    const Intent& intent, LaunchMode mode, Uid callerUid, UserId userId)
{
    const auto& pkg = intent.pkg();
    const auto& cls = intent.cls();

    if (pkg.empty() || cls.empty()) {
        log("startActivity FAILED: empty component");
        return AMSResult::ERROR_INVALID_ARGUMENT;
    }

    log("startActivity: " + intent.toString() + " mode=" + std::to_string(static_cast<int>(mode)));

    // Resolve or create the host process
    auto proc = getOrCreateProcess(pkg, callerUid, userId);

    // ── Launch mode resolution ───────────────────────────────────────────
    switch (mode) {
        case LaunchMode::SINGLE_INSTANCE: {
            // Reuse existing task that holds this exact component
            if (auto existTask = fullscreenStack_->findTaskWithActivity(pkg, cls)) {
                auto existAct = existTask->findActivity(pkg, cls);
                fullscreenStack_->moveTaskToTop(existTask->taskId());
                existTask->markActive();
                resumeTopActivity();
                log("  >> SINGLE_INSTANCE: brought existing task #"
                    + std::to_string(existTask->taskId()) + " to front");
                return AMSResult::START_TASK_TO_FRONT;
            }
            // Create an isolated task
            auto task = fullscreenStack_->createTask(pkg + ".SINGLE_INSTANCE", userId);
            auto act  = std::make_shared<ActivityRecord>(pkg, cls, intent, mode, proc);
            proc->addActivity(act);
            task->push(act);
            task->markActive();
            act->onCreate();
            resumeTopActivity();
            return AMSResult::START_SUCCESS;
        }

        case LaunchMode::SINGLE_TASK: {
            // Reuse existing task with matching affinity
            if (auto existTask = fullscreenStack_->findTaskByAffinity(pkg)) {
                if (auto existAct = existTask->findActivity(pkg, cls)) {
                    // Clear top, bring task to front
                    existTask->clearTopTo(existAct);
                    fullscreenStack_->moveTaskToTop(existTask->taskId());
                    existTask->markActive();
                    resumeTopActivity();
                    log("  >> SINGLE_TASK: reused task #" + std::to_string(existTask->taskId()));
                    return AMSResult::START_TASK_TO_FRONT;
                }
                // Task exists but not this activity – push on top of that task
                auto act = std::make_shared<ActivityRecord>(pkg, cls, intent, mode, proc);
                proc->addActivity(act);
                existTask->push(act);
                existTask->markActive();
                fullscreenStack_->moveTaskToTop(existTask->taskId());
                act->onCreate();
                resumeTopActivity();
                return AMSResult::START_SUCCESS;
            }
            // New task
            auto task = fullscreenStack_->createTask(pkg, userId);
            auto act  = std::make_shared<ActivityRecord>(pkg, cls, intent, mode, proc);
            proc->addActivity(act);
            task->push(act);
            task->markActive();
            act->onCreate();
            resumeTopActivity();
            return AMSResult::START_SUCCESS;
        }

        case LaunchMode::SINGLE_TOP: {
            // If already at top, deliver new intent instead of creating
            if (auto topAct = fullscreenStack_->topActivity()) {
                if (topAct->packageName() == pkg && topAct->className() == cls) {
                    log("  >> SINGLE_TOP: delivered to existing top activity");
                    return AMSResult::START_DELIVERED_TO_TOP;
                }
            }
            [[fallthrough]];
        }

        case LaunchMode::STANDARD:
        default: {
            bool newTask = hasFlag(intent.flags(), LaunchFlag::NEW_TASK);
            std::shared_ptr<TaskRecord> task;
            if (newTask || fullscreenStack_->isEmpty()) {
                task = fullscreenStack_->createTask(pkg, userId);
            } else {
                task = fullscreenStack_->topTask();
            }

            // CLEAR_TOP: remove existing instance and everything above it
            if (hasFlag(intent.flags(), LaunchFlag::CLEAR_TOP)) {
                if (auto existAct = task->findActivity(pkg, cls)) {
                    task->clearTopTo(existAct);
                    task->pop(); // remove the found one (it'll be recreated)
                }
            }

            auto act = std::make_shared<ActivityRecord>(pkg, cls, intent, mode, proc);
            proc->addActivity(act);
            task->push(act);
            task->markActive();
            act->onCreate();
            resumeTopActivity();

            if (listener_.onActivityCreated) listener_.onActivityCreated(*act);
            return AMSResult::START_SUCCESS;
        }
    }
}

// ─── Resume Top Activity ──────────────────────────────────────────────────────
void ActivityManagerService::resumeTopActivity() {
    // Pause all visible activities below the new top
    bool foundTop = false;
    fullscreenStack_->forEachActivity([&](ActivityRecord& act) -> bool {
        if (!foundTop) {
            if (act.state() != ActivityState::RESUMED) {
                act.onResume();
                act.setVisible(true);
                log("  resumed: " + act.className());
            }
            foundTop = true;
        } else {
            if (act.state() == ActivityState::RESUMED) {
                act.onPause();
                act.setVisible(false);
                log("  paused:  " + act.className());
            }
        }
        return true; // continue visiting
    });
}

// ─── Finish Activity ──────────────────────────────────────────────────────────
AMSResult ActivityManagerService::finishActivity(ActivityId id) {
    std::unique_lock lk(rwLock_);

    std::shared_ptr<ActivityRecord> target;
    std::shared_ptr<TaskRecord>     parentTask;

    for (auto& task : fullscreenStack_->tasks()) {
        for (auto& act : task->history()) {
            if (act->id() == id) { target = act; parentTask = task; break; }
        }
        if (target) break;
    }

    if (!target) return AMSResult::ERROR_NOT_FOUND;

    log("finishActivity: " + target->className());
    target->finish();
    target->onDestroy();

    if (listener_.onActivityDestroyed) listener_.onActivityDestroyed(*target);

    // Remove from task
    bool wasTop = (parentTask->top() == target);
    parentTask->pop();

    if (auto proc = target->getProcess()) {
        proc->removeActivity(id);
    }

    if (parentTask->isEmpty()) {
        fullscreenStack_->removeTask(parentTask->taskId());
    }

    if (wasTop) resumeTopActivity();
    return AMSResult::SUCCESS;
}

AMSResult ActivityManagerService::finishActivityByName(const std::string& className) {
    ActivityId targetId = -1;
    fullscreenStack_->forEachActivity([&](ActivityRecord& act) -> bool {
        if (act.className() == className && !act.finishing()) {
            targetId = act.id();
            return false; // stop iteration
        }
        return true;
    });
    if (targetId < 0) return AMSResult::ERROR_NOT_FOUND;
    return finishActivity(targetId);
}

// ─── Process management ───────────────────────────────────────────────────────
std::shared_ptr<ProcessRecord> ActivityManagerService::getOrCreateProcess(
    const std::string& name, Uid uid, UserId userId)
{
    for (auto& [pid, proc] : procs_) {
        if (proc->processName() == name && !proc->killed()) return proc;
    }
    auto proc = std::make_shared<ProcessRecord>(name, uid, userId);
    procs_[proc->pid()] = proc;
    log("Created process: " + name + " pid=" + std::to_string(proc->pid()));
    return proc;
}

bool ActivityManagerService::killProcess(Pid pid, std::string_view reason) {
    std::unique_lock lk(rwLock_);
    auto it = procs_.find(pid);
    if (it == procs_.end()) return false;
    it->second->kill(reason);
    log("killProcess pid=" + std::to_string(pid) + " reason=" + std::string(reason));
    procs_.erase(it);
    return true;
}

void ActivityManagerService::killProcessesInPackage(
    const std::string& pkg, std::string_view reason)
{
    std::unique_lock lk(rwLock_);
    for (auto& [pid, proc] : procs_) {
        if (proc->processName() == pkg) {
            proc->kill(reason);
            log("kill pkg=" + pkg + " pid=" + std::to_string(pid));
        }
    }
}

// ─── Service management ───────────────────────────────────────────────────────
AMSResult ActivityManagerService::startService(
    const std::string& pkg, const std::string& cls, Uid callerUid)
{
    std::unique_lock lk(rwLock_);
    const std::string key = pkg + "/" + cls;
    if (auto it = services_.find(key); it != services_.end()) {
        auto& svc = it->second;
        svc->startId++;
        log("startService (re-deliver): " + key + " startId=" + std::to_string(svc->startId));
        return AMSResult::SUCCESS;
    }
    auto proc = getOrCreateProcess(pkg, callerUid);
    auto svc  = std::make_shared<ServiceRecord>(pkg, cls, callerUid);
    svc->state = ServiceState::CREATING;
    svc->process = proc;
    proc->addService(cls);
    svc->state   = ServiceState::STARTED;
    svc->startId = 1;
    services_[key] = svc;
    log("startService: " + key);
    return AMSResult::SUCCESS;
}

AMSResult ActivityManagerService::stopService(
    const std::string& pkg, const std::string& cls)
{
    std::unique_lock lk(rwLock_);
    const std::string key = pkg + "/" + cls;
    auto it = services_.find(key);
    if (it == services_.end()) return AMSResult::ERROR_NOT_FOUND;

    it->second->state = ServiceState::STOPPING;
    if (auto proc = it->second->process.lock()) {
        proc->removeService(cls);
    }
    it->second->state = ServiceState::DESTROYED;
    log("stopService: " + key);
    services_.erase(it);
    return AMSResult::SUCCESS;
}

AMSResult ActivityManagerService::startForegroundService(
    const std::string& pkg, const std::string& cls, const std::string& title)
{
    auto result = startService(pkg, cls, SYSTEM_UID);
    if (result != AMSResult::SUCCESS) return result;
    std::unique_lock lk(rwLock_);
    const std::string key = pkg + "/" + cls;
    if (auto it = services_.find(key); it != services_.end()) {
        it->second->foreground = true;
        it->second->foregroundNotificationTitle = title;
        log("startForegroundService: " + key + " [" + title + "]");
    }
    return AMSResult::SUCCESS;
}

// ─── Broadcast ────────────────────────────────────────────────────────────────
void ActivityManagerService::registerReceiver(BroadcastReceiver rcv) {
    std::unique_lock lk(rwLock_);
    receivers_.push_back(std::move(rcv));
}

void ActivityManagerService::unregisterReceiver(
    const std::string& pkg, const std::string& cls)
{
    std::unique_lock lk(rwLock_);
    receivers_.erase(
        std::remove_if(receivers_.begin(), receivers_.end(),
            [&](const BroadcastReceiver& r){
                return r.packageName == pkg && r.receiverClass == cls;
            }),
        receivers_.end());
}

void ActivityManagerService::sendBroadcast(Intent intent, Uid callerUid) {
    auto rec = std::make_shared<BroadcastRecord>(std::move(intent), callerUid);
    rec->state = BroadcastState::SCHEDULED;

    {
        std::shared_lock lk(rwLock_);
        for (auto& rcv : receivers_) {
            for (auto& act : rcv.actions) {
                if (act == rec->intent.action()) {
                    rec->receivers.push_back(&rcv);
                    break;
                }
            }
        }
    }

    log("sendBroadcast: " + rec->intent.action()
        + " receivers=" + std::to_string(rec->receivers.size()));

    // Dispatch on looper thread
    Message msg;
    msg.payload = MsgBroadcast{rec->intent.action()};
    msg.callback = [this, rec]() mutable { dispatchBroadcast(rec); };
    if (looper_) looper_->post(std::move(msg));
    else         dispatchBroadcast(rec);

    std::unique_lock lk(rwLock_);
    if (listener_.onBroadcastSent) listener_.onBroadcastSent(*rec);
    recentBroadcasts_.push_back(rec);
    if (recentBroadcasts_.size() > 32) recentBroadcasts_.erase(recentBroadcasts_.begin());
}

void ActivityManagerService::sendOrderedBroadcast(Intent intent, Uid callerUid) {
    auto rec = std::make_shared<BroadcastRecord>(std::move(intent), callerUid);
    rec->setOrdered(true);
    sendBroadcast(rec->intent, callerUid);
}

void ActivityManagerService::dispatchBroadcast(std::shared_ptr<BroadcastRecord> rec) {
    rec->state = BroadcastState::DELIVERING;
    // Sort receivers by priority (descending)
    std::sort(rec->receivers.begin(), rec->receivers.end(),
        [](auto* a, auto* b){ return a->priority > b->priority; });

    for (auto* rcv : rec->receivers) {
        if (rcv->onReceive) {
            rcv->onReceive(rec->intent);
            log("  dispatch to " + rcv->receiverClass);
        }
    }
    rec->state = BroadcastState::DELIVERED;
}

// ─── Queries ──────────────────────────────────────────────────────────────────
std::vector<RunningTaskInfo> ActivityManagerService::getRunningTasks(int32_t maxNum) const {
    std::shared_lock lk(rwLock_);
    std::vector<RunningTaskInfo> result;
    const auto& tasks = fullscreenStack_->tasks();
    for (auto it = tasks.rbegin(); it != tasks.rend() && (int32_t)result.size() < maxNum; ++it) {
        const auto& task = *it;
        if (task->isEmpty()) continue;
        RunningTaskInfo info;
        info.id           = task->taskId();
        info.numActivities = static_cast<int32_t>(task->size());
        info.lastActiveTime = std::chrono::duration_cast<Ms>(
            task->lastActiveTime().time_since_epoch()).count();
        if (auto top = task->top()) {
            info.topActivity = top->packageName() + "/" + top->shortName();
        }
        if (auto root = task->root()) {
            info.baseActivity = root->packageName() + "/" + root->shortName();
        }
        result.push_back(std::move(info));
    }
    return result;
}

std::vector<RunningAppProcessInfo> ActivityManagerService::getRunningAppProcesses() const {
    std::shared_lock lk(rwLock_);
    std::vector<RunningAppProcessInfo> result;
    for (auto& [pid, proc] : procs_) {
        if (proc->killed()) continue;
        RunningAppProcessInfo info;
        info.processName = proc->processName();
        info.pid         = proc->pid();
        info.uid         = proc->uid();
        info.importance  = proc->procState();
        info.oomAdj      = proc->oomAdj();
        info.pssKb       = proc->memInfo().pssKb;
        result.push_back(std::move(info));
    }
    // Sort by importance (lower OOM adj = more important)
    std::sort(result.begin(), result.end(),
        [](const auto& a, const auto& b){ return a.oomAdj < b.oomAdj; });
    return result;
}

std::shared_ptr<ActivityRecord> ActivityManagerService::getTopActivity() const {
    std::shared_lock lk(rwLock_);
    return fullscreenStack_->topActivity();
}

std::optional<RunningTaskInfo> ActivityManagerService::getForegroundTaskInfo() const {
    auto tasks = getRunningTasks(1);
    if (tasks.empty()) return std::nullopt;
    return tasks.front();
}

// ─── Move tasks ───────────────────────────────────────────────────────────────
AMSResult ActivityManagerService::moveTaskToFront(TaskId taskId) {
    std::unique_lock lk(rwLock_);
    fullscreenStack_->moveTaskToTop(taskId);
    resumeTopActivity();
    return AMSResult::SUCCESS;
}

AMSResult ActivityManagerService::moveTaskToBack(TaskId taskId) {
    std::unique_lock lk(rwLock_);
    auto& tasks = const_cast<std::vector<std::shared_ptr<TaskRecord>>&>(
        fullscreenStack_->tasks());
    auto it = std::find_if(tasks.begin(), tasks.end(),
        [taskId](const auto& t){ return t->taskId() == taskId; });
    if (it == tasks.end()) return AMSResult::ERROR_NOT_FOUND;
    std::rotate(tasks.begin(), it, it + 1);
    resumeTopActivity();
    return AMSResult::SUCCESS;
}

// ─── Dump ─────────────────────────────────────────────────────────────────────
std::string ActivityManagerService::dumpActivities() const {
    std::shared_lock lk(rwLock_);
    return fullscreenStack_->dump();
}

std::string ActivityManagerService::dumpProcesses() const {
    std::shared_lock lk(rwLock_);
    std::ostringstream ss;
    ss << "=== Running processes ===\n";
    auto procs = getRunningAppProcesses();
    for (auto& p : procs) {
        ss << "  pid=" << p.pid
           << " name=" << p.processName
           << " state=" << to_string(p.importance)
           << " oomAdj=" << p.oomAdj
           << " pss=" << p.pssKb << "KB\n";
    }
    return ss.str();
}

std::string ActivityManagerService::dumpServices() const {
    std::shared_lock lk(rwLock_);
    std::ostringstream ss;
    ss << "=== Running services ===\n";
    for (auto& [key, svc] : services_) {
        ss << "  " << svc->dump() << "\n";
    }
    return ss.str();
}

std::string ActivityManagerService::dumpBroadcasts() const {
    std::shared_lock lk(rwLock_);
    std::ostringstream ss;
    ss << "=== Recent broadcasts ===\n";
    for (auto it = recentBroadcasts_.rbegin(); it != recentBroadcasts_.rend(); ++it) {
        ss << "  " << (*it)->dump() << "\n";
    }
    ss << "=== Registered receivers (" << receivers_.size() << ") ===\n";
    for (auto& r : receivers_) {
        ss << "  " << r.packageName << "/" << r.receiverClass << " actions=[";
        for (size_t i = 0; i < r.actions.size(); ++i) {
            if (i) ss << ",";
            ss << r.actions[i];
        }
        ss << "]\n";
    }
    return ss.str();
}

std::string ActivityManagerService::dumpState() const {
    std::ostringstream ss;
    ss << "========== AMS State Dump ==========\n";
    ss << dumpActivities();
    ss << dumpProcesses();
    ss << dumpServices();
    ss << dumpBroadcasts();
    ss << "====================================\n";
    return ss.str();
}

// ─── Message handler ──────────────────────────────────────────────────────────
void ActivityManagerService::handleMessage(const Message& msg) {
    std::visit([this](const auto& payload) {
        using T = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<T, MsgStartActivity>) {
            Intent i(IntentAction::MAIN);
            i.setComponent(payload.pkg, payload.cls).setFlags(payload.flags);
            std::unique_lock lk(rwLock_);
            startActivityLocked(i, LaunchMode::STANDARD, SYSTEM_UID, 0);
        } else if constexpr (std::is_same_v<T, MsgDestroyActivity>) {
            finishActivity(payload.id);
        } else if constexpr (std::is_same_v<T, MsgBroadcast>) {
            // Already handled via callback
        } else if constexpr (std::is_same_v<T, MsgQuit>) {
            shutdown();
        }
    }, msg.payload);
}

void ActivityManagerService::log(std::string msg) const {
    ALOGI("%s", msg.c_str());
    if (listener_.onLog) listener_.onLog(std::move(msg));
}

} // namespace android::ams
