#pragma once
#include <string>
#include <atomic>
#include <functional>
#include <memory>
#include <any>
#include <optional>
#include <vector>
#include "ams_types.h"
#include "intent.h"

namespace android::ams {

class ProcessRecord;

// Lifecycle callback set (mirrors ActivityLifecycleCallbacks)
struct LifecycleCallbacks {
    std::function<void()> onCreate;
    std::function<void()> onResume;
    std::function<void()> onPause;
    std::function<void()> onStop;
    std::function<void()> onDestroy;
};

// ─── ActivityRecord ───────────────────────────────────────────────────────────
class ActivityRecord {
public:
    static std::atomic<ActivityId> sNextId;

    ActivityRecord(std::string pkg, std::string cls,
                   Intent intent, LaunchMode mode,
                   std::weak_ptr<ProcessRecord> proc)
        : id_(sNextId.fetch_add(1, std::memory_order_relaxed)),
          packageName_(std::move(pkg)),
          className_(std::move(cls)),
          intent_(std::move(intent)),
          launchMode_(mode),
          process_(std::move(proc)),
          createTime_(std::chrono::steady_clock::now()),
          state_(ActivityState::INITIALIZING) {}

    // ── Lifecycle transitions ───────────────────────────────────────────────
    void onCreate() {
        setState(ActivityState::STOPPED);
        if (callbacks_.onCreate) callbacks_.onCreate();
    }
    void onResume() {
        setState(ActivityState::RESUMED);
        lastResumeTime_ = std::chrono::steady_clock::now();
        if (callbacks_.onResume) callbacks_.onResume();
    }
    void onPause() {
        setState(ActivityState::PAUSED);
        if (callbacks_.onPause) callbacks_.onPause();
    }
    void onStop() {
        setState(ActivityState::STOPPED);
        if (callbacks_.onStop) callbacks_.onStop();
    }
    void onDestroy() {
        setState(ActivityState::DESTROYING);
        if (callbacks_.onDestroy) callbacks_.onDestroy();
        setState(ActivityState::DESTROYED);
    }
    void finish() { finishing_ = true; setState(ActivityState::FINISHING); }

    // ── Observers ──────────────────────────────────────────────────────────
    [[nodiscard]] ActivityId         id()          const { return id_;           }
    [[nodiscard]] const std::string& packageName() const { return packageName_;  }
    [[nodiscard]] const std::string& className()   const { return className_;    }
    [[nodiscard]] const Intent&      intent()      const { return intent_;       }
    [[nodiscard]] LaunchMode         launchMode()  const { return launchMode_;   }
    [[nodiscard]] ActivityState      state()       const { return state_;        }
    [[nodiscard]] bool               finishing()   const { return finishing_;    }
    [[nodiscard]] bool               visible()     const { return visible_;      }
    [[nodiscard]] TimePoint          createTime()  const { return createTime_;   }

    [[nodiscard]] std::string shortName() const {
        auto dot = className_.rfind('.');
        return (dot != std::string::npos) ? className_.substr(dot + 1) : className_;
    }

    [[nodiscard]] Duration aliveFor() const {
        return std::chrono::steady_clock::now() - createTime_;
    }

    [[nodiscard]] std::optional<Duration> resumedFor() const {
        if (state_ != ActivityState::RESUMED) return std::nullopt;
        return std::chrono::steady_clock::now() - lastResumeTime_;
    }

    void setVisible(bool v)       { visible_ = v; }
    void setCallbacks(LifecycleCallbacks cb) { callbacks_ = std::move(cb); }
    void setUserData(std::any d)  { userData_ = std::move(d); }

    template<typename T>
    [[nodiscard]] std::optional<T> getUserData() const {
        if (!userData_.has_value()) return std::nullopt;
        if (auto* p = std::any_cast<T>(&userData_)) return *p;
        return std::nullopt;
    }

    [[nodiscard]] std::shared_ptr<ProcessRecord> getProcess() const {
        return process_.lock();
    }

    [[nodiscard]] std::string dump() const;

private:
    void setState(ActivityState s) { state_ = s; }

    const ActivityId        id_;
    const std::string       packageName_;
    const std::string       className_;
    Intent                  intent_;
    const LaunchMode        launchMode_;
    std::weak_ptr<ProcessRecord> process_;
    TimePoint               createTime_;
    TimePoint               lastResumeTime_{};
    std::atomic<ActivityState> state_;
    bool                    finishing_{false};
    bool                    visible_{false};
    LifecycleCallbacks      callbacks_;
    std::any                userData_;
};

} // namespace android::ams
