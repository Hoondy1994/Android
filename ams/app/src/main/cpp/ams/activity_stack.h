#pragma once
#include <vector>
#include <memory>
#include <optional>
#include <string>
#include <functional>
#include "ams_types.h"
#include "task_record.h"

namespace android::ams {

class ActivityRecord;

// ─── ActivityStack ────────────────────────────────────────────────────────────
// One display stack (e.g. FULLSCREEN or FREEFORM).  Owns TaskRecords.
class ActivityStack {
public:
    enum class StackId : int32_t {
        HOME        = 0,
        FULLSCREEN  = 1,
        FREEFORM    = 2,
        PINNED      = 3,
        RECENTS     = 4,
    };

    explicit ActivityStack(StackId id) : stackId_(id) {}

    // ── Task management ────────────────────────────────────────────────────
    std::shared_ptr<TaskRecord> createTask(const std::string& affinity, UserId userId = 0) {
        auto task = std::make_shared<TaskRecord>(affinity, userId);
        tasks_.push_back(task);
        return task;
    }

    // Move task to top (focus)
    void moveTaskToTop(TaskId id) {
        auto it = std::find_if(tasks_.begin(), tasks_.end(),
            [id](const auto& t){ return t->taskId() == id; });
        if (it != tasks_.end() && it != tasks_.end() - 1) {
            std::rotate(it, it + 1, tasks_.end());
        }
    }

    void removeTask(TaskId id) {
        tasks_.erase(
            std::remove_if(tasks_.begin(), tasks_.end(),
                [id](const auto& t){ return t->taskId() == id; }),
            tasks_.end());
    }

    [[nodiscard]] std::shared_ptr<TaskRecord> topTask() const {
        return tasks_.empty() ? nullptr : tasks_.back();
    }

    [[nodiscard]] std::shared_ptr<ActivityRecord> topActivity() const {
        auto t = topTask();
        return t ? t->top() : nullptr;
    }

    // Find a task by affinity (for singleTask reuse)
    [[nodiscard]] std::shared_ptr<TaskRecord>
    findTaskByAffinity(const std::string& affinity) const {
        for (auto it = tasks_.rbegin(); it != tasks_.rend(); ++it) {
            if ((*it)->affinity() == affinity) return *it;
        }
        return nullptr;
    }

    // Find a task that contains a specific component (for singleInstance reuse)
    [[nodiscard]] std::shared_ptr<TaskRecord>
    findTaskWithActivity(const std::string& pkg, const std::string& cls) const {
        for (auto it = tasks_.rbegin(); it != tasks_.rend(); ++it) {
            if ((*it)->findActivity(pkg, cls)) return *it;
        }
        return nullptr;
    }

    // Visit all activities top→bottom across tasks
    void forEachActivity(const std::function<bool(ActivityRecord&)>& fn) const {
        for (auto it = tasks_.rbegin(); it != tasks_.rend(); ++it) {
            const auto& hist = (*it)->history();
            for (auto it2 = hist.rbegin(); it2 != hist.rend(); ++it2) {
                if (!fn(**it2)) return;
            }
        }
    }

    [[nodiscard]] StackId     stackId()    const { return stackId_;       }
    [[nodiscard]] size_t      taskCount()  const { return tasks_.size();  }
    [[nodiscard]] bool        isEmpty()    const { return tasks_.empty(); }

    [[nodiscard]] const std::vector<std::shared_ptr<TaskRecord>>& tasks() const {
        return tasks_;
    }

    [[nodiscard]] std::string dump() const;

private:
    const StackId stackId_;
    std::vector<std::shared_ptr<TaskRecord>> tasks_;
};

} // namespace android::ams
