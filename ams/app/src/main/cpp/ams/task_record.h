#pragma once
#include <vector>
#include <memory>
#include <string>
#include <optional>
#include <atomic>
#include <algorithm>
#include "ams_types.h"
#include "activity_record.h"

namespace android::ams {

// ─── TaskRecord  ─────────────────────────────────────────────────────────────
// Represents one Android Task (back stack).  Activities are stored bottom→top.
class TaskRecord {
public:
    static std::atomic<TaskId> sNextTaskId;

    explicit TaskRecord(std::string affinity, UserId userId = 0)
        : taskId_(sNextTaskId.fetch_add(1, std::memory_order_relaxed)),
          affinity_(std::move(affinity)),
          userId_(userId),
          createTime_(std::chrono::steady_clock::now()) {}

    // ── Back-stack operations ──────────────────────────────────────────────
    void push(std::shared_ptr<ActivityRecord> act) {
        history_.push_back(std::move(act));
    }

    std::shared_ptr<ActivityRecord> pop() {
        if (history_.empty()) return nullptr;
        auto top = history_.back();
        history_.pop_back();
        return top;
    }

    [[nodiscard]] std::shared_ptr<ActivityRecord> top() const {
        return history_.empty() ? nullptr : history_.back();
    }

    [[nodiscard]] std::shared_ptr<ActivityRecord> root() const {
        return history_.empty() ? nullptr : history_.front();
    }

    // Find the topmost activity from the given component (for singleTop/singleTask)
    [[nodiscard]] std::shared_ptr<ActivityRecord>
    findActivity(const std::string& pkg, const std::string& cls) const {
        for (auto it = history_.rbegin(); it != history_.rend(); ++it) {
            if ((*it)->packageName() == pkg && (*it)->className() == cls)
                return *it;
        }
        return nullptr;
    }

    // Remove everything above (and including) a given activity
    void clearTopTo(const std::shared_ptr<ActivityRecord>& act) {
        auto it = std::find(history_.begin(), history_.end(), act);
        if (it != history_.end()) history_.erase(it + 1, history_.end());
    }

    // Remove all activities
    std::vector<std::shared_ptr<ActivityRecord>> clear() {
        auto old = std::move(history_);
        history_.clear();
        return old;
    }

    // ── Accessors ──────────────────────────────────────────────────────────
    [[nodiscard]] TaskId      taskId()      const { return taskId_;   }
    [[nodiscard]] UserId      userId()      const { return userId_;   }
    [[nodiscard]] const std::string& affinity() const { return affinity_; }
    [[nodiscard]] bool        isEmpty()     const { return history_.empty(); }
    [[nodiscard]] size_t      size()        const { return history_.size();  }
    [[nodiscard]] bool        isHome()      const { return isHome_;          }
    [[nodiscard]] bool        autoRemoveFromRecents() const { return autoRemove_; }
    [[nodiscard]] TimePoint   lastActiveTime() const { return lastActive_;   }

    void markActive()   { lastActive_ = std::chrono::steady_clock::now(); }
    void setHome(bool h){ isHome_ = h; }
    void setAutoRemoveFromRecents(bool v){ autoRemove_ = v; }

    [[nodiscard]] const std::vector<std::shared_ptr<ActivityRecord>>& history() const {
        return history_;
    }

    [[nodiscard]] std::string dump() const;

private:
    const TaskId  taskId_;
    const std::string affinity_;
    const UserId  userId_;
    const TimePoint createTime_;
    TimePoint     lastActive_{createTime_};
    bool          isHome_{false};
    bool          autoRemove_{false};
    std::vector<std::shared_ptr<ActivityRecord>> history_;
};

} // namespace android::ams
