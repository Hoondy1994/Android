#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace android::ams {

// ─── Activity lifecycle states ────────────────────────────────────────────────
enum class ActivityState : int32_t {
    INITIALIZING = 0,
    RESUMED      = 1,
    PAUSING      = 2,
    PAUSED       = 3,
    STOPPING     = 4,
    STOPPED      = 5,
    FINISHING    = 6,
    DESTROYING   = 7,
    DESTROYED    = 8,
};

// ─── Launch modes ─────────────────────────────────────────────────────────────
enum class LaunchMode : int32_t {
    STANDARD        = 0,
    SINGLE_TOP      = 1,
    SINGLE_TASK     = 2,
    SINGLE_INSTANCE = 3,
};

// ─── Process importance / OOM adj bucket ──────────────────────────────────────
enum class ProcessState : int32_t {
    NONEXISTENT              = -1,
    PERSISTENT               = 0,
    PERSISTENT_UI            = 1,
    TOP                      = 2,
    FOREGROUND_SERVICE_BOUND = 3,
    FOREGROUND_SERVICE       = 4,
    IMPORTANT_FOREGROUND     = 5,
    IMPORTANT_BACKGROUND     = 6,
    TRANSIENT_BACKGROUND     = 7,
    BACKUP                   = 8,
    SERVICE                  = 9,
    RECEIVER                 = 10,
    CACHED_RECENT            = 11,
    CACHED_ACTIVITY_CLIENT   = 12,
    CACHED_ACTIVITY          = 13,
    CACHED_EMPTY             = 15,
};

// ─── Intent launch flags (bit mask) ───────────────────────────────────────────
enum class LaunchFlag : uint32_t {
    NONE              = 0x00000000,
    SINGLE_TOP        = 0x20000000,
    NEW_TASK          = 0x10000000,
    MULTIPLE_TASK     = 0x08000000,
    CLEAR_TOP         = 0x04000000,
    REORDER_TO_FRONT  = 0x00020000,
    NO_HISTORY        = 0x40000000,
    CLEAR_TASK        = 0x00008000,
    TASK_ON_HOME      = 0x00004000,
    EXCLUDE_FROM_RECENTS = 0x00800000,
};

constexpr LaunchFlag operator|(LaunchFlag a, LaunchFlag b) noexcept {
    return static_cast<LaunchFlag>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
constexpr bool hasFlag(LaunchFlag flags, LaunchFlag test) noexcept {
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(test)) != 0;
}

// ─── Service state ────────────────────────────────────────────────────────────
enum class ServiceState : int32_t {
    IDLE       = 0,
    CREATING   = 1,
    CREATED    = 2,
    STARTING   = 3,
    STARTED    = 4,
    BINDING    = 5,
    BOUND      = 6,
    STOPPING   = 7,
    DESTROYED  = 8,
};

// ─── Broadcast delivery state ─────────────────────────────────────────────────
enum class BroadcastState : int32_t {
    IDLE        = 0,
    SCHEDULED   = 1,
    DELIVERING  = 2,
    DELIVERED   = 3,
    FAILED      = 4,
};

// ─── AMS operation results ────────────────────────────────────────────────────
enum class AMSResult : int32_t {
    SUCCESS                       = 0,
    ERROR_GENERIC                 = -1,
    ERROR_NOT_FOUND               = -2,
    ERROR_ALREADY_EXISTS          = -3,
    ERROR_INVALID_ARGUMENT        = -4,
    ERROR_PERMISSION_DENIED       = -5,
    ERROR_DEAD_OBJECT             = -6,
    ERROR_PROCESS_DIED            = -7,
    START_SUCCESS                 = 0,
    START_RETURN_INTENT_TO_CALLER = 1,
    START_TASK_TO_FRONT           = 2,
    START_DELIVERED_TO_TOP        = 3,
    START_CANCELED                = 4,
    START_CLASS_NOT_FOUND         = 5,
    START_NOT_VOICE_COMPATIBLE    = 6,
};

// ─── Primitive type aliases ────────────────────────────────────────────────────
using Pid        = int32_t;
using Uid        = int32_t;
using UserId     = int32_t;
using TaskId     = int32_t;
using ActivityId = int32_t;
using ServiceId  = int32_t;
using TimePoint  = std::chrono::steady_clock::time_point;
using Duration   = std::chrono::steady_clock::duration;
using Ms         = std::chrono::milliseconds;

// ─── Well-known UIDs ──────────────────────────────────────────────────────────
inline constexpr Uid ROOT_UID   = 0;
inline constexpr Uid SYSTEM_UID = 1000;
inline constexpr Uid PHONE_UID  = 1001;
inline constexpr Uid SHELL_UID  = 2000;

// ─── Human-readable converters ────────────────────────────────────────────────
[[nodiscard]] constexpr std::string_view to_string(ActivityState s) noexcept {
    switch (s) {
        case ActivityState::INITIALIZING: return "INITIALIZING";
        case ActivityState::RESUMED:      return "RESUMED";
        case ActivityState::PAUSING:      return "PAUSING";
        case ActivityState::PAUSED:       return "PAUSED";
        case ActivityState::STOPPING:     return "STOPPING";
        case ActivityState::STOPPED:      return "STOPPED";
        case ActivityState::FINISHING:    return "FINISHING";
        case ActivityState::DESTROYING:   return "DESTROYING";
        case ActivityState::DESTROYED:    return "DESTROYED";
        default:                          return "UNKNOWN";
    }
}

[[nodiscard]] constexpr std::string_view to_string(ProcessState s) noexcept {
    switch (s) {
        case ProcessState::NONEXISTENT:              return "NONEXISTENT";
        case ProcessState::PERSISTENT:               return "PERSISTENT";
        case ProcessState::TOP:                      return "TOP";
        case ProcessState::FOREGROUND_SERVICE:       return "FG_SERVICE";
        case ProcessState::IMPORTANT_FOREGROUND:     return "IMPORTANT_FG";
        case ProcessState::IMPORTANT_BACKGROUND:     return "IMPORTANT_BG";
        case ProcessState::SERVICE:                  return "SERVICE";
        case ProcessState::RECEIVER:                 return "RECEIVER";
        case ProcessState::CACHED_RECENT:            return "CACHED_RECENT";
        case ProcessState::CACHED_EMPTY:             return "CACHED_EMPTY";
        default:                                     return "UNKNOWN";
    }
}

[[nodiscard]] constexpr std::string_view to_string(ServiceState s) noexcept {
    switch (s) {
        case ServiceState::IDLE:      return "IDLE";
        case ServiceState::CREATING:  return "CREATING";
        case ServiceState::CREATED:   return "CREATED";
        case ServiceState::STARTING:  return "STARTING";
        case ServiceState::STARTED:   return "STARTED";
        case ServiceState::BINDING:   return "BINDING";
        case ServiceState::BOUND:     return "BOUND";
        case ServiceState::STOPPING:  return "STOPPING";
        case ServiceState::DESTROYED: return "DESTROYED";
        default:                      return "UNKNOWN";
    }
}

// ─── Timestamp helper ─────────────────────────────────────────────────────────
[[nodiscard]] inline std::string formatDuration(Duration d) {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
    std::ostringstream ss;
    if (ms >= 60000) {
        ss << (ms / 60000) << "m " << ((ms % 60000) / 1000) << "s";
    } else if (ms >= 1000) {
        ss << std::fixed << std::setprecision(1) << (ms / 1000.0) << "s";
    } else {
        ss << ms << "ms";
    }
    return ss.str();
}

[[nodiscard]] inline int64_t nowMs() {
    return std::chrono::duration_cast<Ms>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

} // namespace android::ams
