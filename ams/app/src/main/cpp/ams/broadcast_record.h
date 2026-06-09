#pragma once
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <chrono>
#include "ams_types.h"
#include "intent.h"

namespace android::ams {

struct BroadcastReceiver {
    std::string              packageName;
    std::string              receiverClass;
    std::vector<std::string> actions;   // subscribed actions
    int32_t                  priority{0};
    std::function<void(const Intent&)> onReceive;
};

struct BroadcastRecord {
    inline static std::atomic<int32_t> sNextId{1};

    const int32_t id{sNextId.fetch_add(1, std::memory_order_relaxed)};
    Intent        intent;
    Uid           callerUid{SYSTEM_UID};
    TimePoint     enqueueTime{std::chrono::steady_clock::now()};
    BroadcastState state{BroadcastState::IDLE};

    // Receivers that will be dispatched (sorted by priority, descending)
    std::vector<BroadcastReceiver*> receivers;
    int32_t nextReceiver{0};

    explicit BroadcastRecord(Intent i, Uid uid = SYSTEM_UID)
        : intent(std::move(i)), callerUid(uid) {}

    [[nodiscard]] bool isOrdered() const { return ordered_; }
    void setOrdered(bool v) { ordered_ = v; }

    [[nodiscard]] std::string dump() const {
        return "Broadcast{id=" + std::to_string(id)
             + " action=" + intent.action()
             + " rcvrs=" + std::to_string(receivers.size()) + "}";
    }

private:
    bool ordered_{false};
};

} // namespace android::ams
