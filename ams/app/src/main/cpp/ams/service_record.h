#pragma once
#include <string>
#include <atomic>
#include <memory>
#include <chrono>
#include <functional>
#include "ams_types.h"

namespace android::ams {

class ProcessRecord;

struct ServiceRecord {
    inline static std::atomic<ServiceId> sNextId{5000};

    const ServiceId   id{sNextId.fetch_add(1, std::memory_order_relaxed)};
    std::string       packageName;
    std::string       className;
    Uid               uid{0};
    ServiceState      state{ServiceState::IDLE};
    int32_t           startId{0};
    int32_t           bindCount{0};
    bool              foreground{false};
    std::string       foregroundNotificationTitle;
    TimePoint         startTime{std::chrono::steady_clock::now()};

    std::weak_ptr<ProcessRecord> process;
    std::function<void(ServiceRecord&)> onStarted;
    std::function<void(ServiceRecord&)> onStopped;

    ServiceRecord(std::string pkg, std::string cls, Uid u)
        : packageName(std::move(pkg)), className(std::move(cls)), uid(u) {}

    [[nodiscard]] std::string shortName() const {
        auto dot = className.rfind('.');
        return (dot != std::string::npos) ? className.substr(dot + 1) : className;
    }

    [[nodiscard]] Duration uptime() const {
        return std::chrono::steady_clock::now() - startTime;
    }

    [[nodiscard]] std::string dump() const {
        return "ServiceRecord{id=" + std::to_string(id)
             + " " + packageName + "/" + shortName()
             + " state=" + std::string(to_string(state))
             + " fg=" + (foreground ? "true" : "false")
             + " uptime=" + formatDuration(uptime()) + "}";
    }
};

} // namespace android::ams
