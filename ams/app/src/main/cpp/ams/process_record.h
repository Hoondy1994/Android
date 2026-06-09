#pragma once
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include "ams_types.h"

namespace android::ams {

class ActivityRecord;
class ServiceRecord;

struct MemInfo {
    int64_t pssKb{0};
    int64_t rssKb{0};
    int64_t ussKb{0};
};

// ─── ProcessRecord ────────────────────────────────────────────────────────────
class ProcessRecord : public std::enable_shared_from_this<ProcessRecord> {
public:
    static std::atomic<Pid> sNextPid;

    ProcessRecord(std::string processName, Uid uid, UserId userId)
        : pid_(sNextPid.fetch_add(1, std::memory_order_relaxed)),
          processName_(std::move(processName)),
          uid_(uid),
          userId_(userId),
          startTime_(std::chrono::steady_clock::now()),
          procState_(ProcessState::CACHED_EMPTY) {
        // Simulate some memory footprint
        memInfo_.pssKb = 20480 + (pid_ % 50) * 512;
        memInfo_.rssKb = memInfo_.pssKb + 4096;
        memInfo_.ussKb = memInfo_.pssKb - 2048;
    }

    // ── Activity tracking ──────────────────────────────────────────────────
    void addActivity(std::shared_ptr<ActivityRecord> act);
    bool removeActivity(ActivityId id);

    [[nodiscard]] std::vector<std::shared_ptr<ActivityRecord>> activities() const {
        std::lock_guard lk(mtx_);
        return activities_;
    }

    [[nodiscard]] size_t activityCount() const {
        std::lock_guard lk(mtx_);
        return activities_.size();
    }

    // ── Service tracking ───────────────────────────────────────────────────
    void addService(std::string svcName);
    bool removeService(const std::string& svcName);

    // ── OOM adj ───────────────────────────────────────────────────────────
    [[nodiscard]] int32_t oomAdj() const {
        switch (procState_) {
            case ProcessState::PERSISTENT:           return -17;
            case ProcessState::TOP:                  return 0;
            case ProcessState::FOREGROUND_SERVICE:   return 100;
            case ProcessState::IMPORTANT_FOREGROUND: return 150;
            case ProcessState::IMPORTANT_BACKGROUND: return 200;
            case ProcessState::SERVICE:              return 400;
            case ProcessState::RECEIVER:             return 500;
            case ProcessState::CACHED_RECENT:        return 700;
            case ProcessState::CACHED_EMPTY:         return 900;
            default:                                 return 999;
        }
    }

    // ── Accessors ──────────────────────────────────────────────────────────
    [[nodiscard]] Pid         pid()         const { return pid_;         }
    [[nodiscard]] Uid         uid()         const { return uid_;         }
    [[nodiscard]] UserId      userId()      const { return userId_;      }
    [[nodiscard]] ProcessState procState()  const { return procState_;   }
    [[nodiscard]] const MemInfo& memInfo()  const { return memInfo_;     }
    [[nodiscard]] const std::string& processName() const { return processName_; }
    [[nodiscard]] Duration    uptime()      const {
        return std::chrono::steady_clock::now() - startTime_;
    }
    [[nodiscard]] bool        killed()      const { return killed_;      }

    void kill(std::string_view reason) {
        killed_ = true;
        killReason_ = reason;
        procState_ = ProcessState::NONEXISTENT;
    }

    [[nodiscard]] std::string dump() const;

private:
    void updateProcState(); // implemented in process_record.cpp (needs ActivityRecord)

    const Pid         pid_;
    const std::string processName_;
    const Uid         uid_;
    const UserId      userId_;
    const TimePoint   startTime_;
    ProcessState      procState_;
    MemInfo           memInfo_;
    bool              killed_{false};
    std::string       killReason_;

    mutable std::mutex mtx_;
    std::vector<std::shared_ptr<ActivityRecord>> activities_;
    std::vector<std::string>                     services_;
};

} // namespace android::ams
