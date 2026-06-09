#include "process_record.h"
#include "activity_record.h"
#include <sstream>
#include <algorithm>

namespace android::ams {

std::atomic<Pid> ProcessRecord::sNextPid{10000};

void ProcessRecord::addActivity(std::shared_ptr<ActivityRecord> act) {
    std::lock_guard lk(mtx_);
    activities_.push_back(std::move(act));
    updateProcState();
}

bool ProcessRecord::removeActivity(ActivityId id) {
    std::lock_guard lk(mtx_);
    auto it = std::remove_if(activities_.begin(), activities_.end(),
        [id](const auto& a){ return a->id() == id; });
    bool found = (it != activities_.end());
    activities_.erase(it, activities_.end());
    updateProcState();
    return found;
}

void ProcessRecord::addService(std::string svcName) {
    std::lock_guard lk(mtx_);
    services_.push_back(std::move(svcName));
    updateProcState();
}

bool ProcessRecord::removeService(const std::string& svcName) {
    std::lock_guard lk(mtx_);
    auto it = std::find(services_.begin(), services_.end(), svcName);
    if (it == services_.end()) return false;
    services_.erase(it);
    updateProcState();
    return true;
}

void ProcessRecord::updateProcState() {
    if (killed_) { procState_ = ProcessState::NONEXISTENT; return; }
    bool hasResumed = false;
    for (auto& a : activities_) {
        if (a->state() == ActivityState::RESUMED) { hasResumed = true; break; }
    }
    if (hasResumed) {
        procState_ = ProcessState::TOP;
    } else if (!services_.empty()) {
        procState_ = activities_.empty()
            ? ProcessState::SERVICE
            : ProcessState::IMPORTANT_BACKGROUND;
    } else if (!activities_.empty()) {
        procState_ = ProcessState::CACHED_RECENT;
    } else {
        procState_ = ProcessState::CACHED_EMPTY;
    }
    memInfo_.pssKb = 20480
        + static_cast<int64_t>(activities_.size()) * 4096
        + static_cast<int64_t>(services_.size())  * 2048;
}

std::string ProcessRecord::dump() const {
    std::ostringstream ss;
    ss << "ProcessRecord {"
       << " pid=" << pid_
       << " name=" << processName_
       << " uid=" << uid_
       << " state=" << to_string(procState_)
       << " oomAdj=" << oomAdj()
       << " pss=" << memInfo_.pssKb << "KB"
       << " uptime=" << formatDuration(uptime());
    if (killed_) ss << " [KILLED:" << killReason_ << "]";
    ss << " acts=" << activities_.size()
       << " svcs=" << services_.size()
       << " }";
    return ss.str();
}

} // namespace android::ams
