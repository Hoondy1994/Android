#include "task_record.h"
#include "activity_record.h"
#include <sstream>

namespace android::ams {

std::atomic<TaskId> TaskRecord::sNextTaskId{1};

std::string TaskRecord::dump() const {
    std::ostringstream ss;
    ss << "TaskRecord #" << taskId_
       << " affinity=" << affinity_
       << " userId=" << userId_
       << " size=" << history_.size();
    if (isHome_)    ss << " [HOME]";
    ss << "\n";
    for (auto it = history_.rbegin(); it != history_.rend(); ++it) {
        ss << (*it)->dump() << "\n";
    }
    return ss.str();
}

} // namespace android::ams
