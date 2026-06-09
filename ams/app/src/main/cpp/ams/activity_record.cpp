#include "activity_record.h"
#include "process_record.h"
#include <sstream>

namespace android::ams {

std::atomic<ActivityId> ActivityRecord::sNextId{1000};

std::string ActivityRecord::dump() const {
    std::ostringstream ss;
    ss << "  ActivityRecord #" << id_
       << " " << packageName_ << "/" << shortName()
       << " state=" << to_string(state())
       << " alive=" << formatDuration(aliveFor());
    if (finishing_) ss << " [FINISHING]";
    if (visible_)   ss << " [VISIBLE]";
    return ss.str();
}

} // namespace android::ams
