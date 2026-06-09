#include "activity_stack.h"
#include "activity_record.h"
#include <sstream>

namespace android::ams {

std::string ActivityStack::dump() const {
    std::ostringstream ss;
    ss << "ActivityStack stackId=" << static_cast<int>(stackId_)
       << " tasks=" << tasks_.size() << "\n";
    for (auto it = tasks_.rbegin(); it != tasks_.rend(); ++it) {
        ss << (*it)->dump();
    }
    return ss.str();
}

} // namespace android::ams
