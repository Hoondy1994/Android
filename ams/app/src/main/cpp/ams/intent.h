#pragma once
#include <string>
#include <unordered_map>
#include <variant>
#include <optional>
#include <vector>
#include "ams_types.h"

namespace android::ams {

using ExtraValue = std::variant<
    int32_t, int64_t, float, double, bool,
    std::string,
    std::vector<int32_t>,
    std::vector<std::string>
>;

class Intent {
public:
    explicit Intent(std::string action = "") : action_(std::move(action)) {}

    Intent& setAction(std::string a)    { action_ = std::move(a); return *this; }
    Intent& setFlags(LaunchFlag f)      { flags_  = f;            return *this; }
    Intent& addFlags(LaunchFlag f)      { flags_  = flags_ | f;   return *this; }

    Intent& setComponent(std::string pkg, std::string cls) {
        pkg_ = std::move(pkg); cls_ = std::move(cls); return *this;
    }

    template<typename T>
    Intent& putExtra(std::string key, T val) {
        extras_[std::move(key)] = std::move(val); return *this;
    }

    template<typename T>
    [[nodiscard]] std::optional<T> getExtra(const std::string& key) const {
        if (auto it = extras_.find(key); it != extras_.end()) {
            if (auto* v = std::get_if<T>(&it->second)) return *v;
        }
        return std::nullopt;
    }

    [[nodiscard]] const std::string& action()  const { return action_; }
    [[nodiscard]] const std::string& pkg()     const { return pkg_;    }
    [[nodiscard]] const std::string& cls()     const { return cls_;    }
    [[nodiscard]] LaunchFlag         flags()   const { return flags_;  }
    [[nodiscard]] bool hasExtra(const std::string& k) const { return extras_.count(k) > 0; }

    [[nodiscard]] std::string toString() const {
        return "Intent{act=" + action_ + " cmp=" + pkg_ + "/" + cls_ + "}";
    }

private:
    std::string  action_, pkg_, cls_;
    LaunchFlag   flags_{LaunchFlag::NONE};
    std::unordered_map<std::string, ExtraValue> extras_;
};

namespace IntentAction {
    inline constexpr const char* MAIN           = "android.intent.action.MAIN";
    inline constexpr const char* VIEW           = "android.intent.action.VIEW";
    inline constexpr const char* SEND           = "android.intent.action.SEND";
    inline constexpr const char* BOOT_COMPLETED = "android.intent.action.BOOT_COMPLETED";
    inline constexpr const char* SCREEN_ON      = "android.intent.action.SCREEN_ON";
    inline constexpr const char* BATTERY_LOW    = "android.intent.action.BATTERY_LOW";
    inline constexpr const char* PACKAGE_ADDED  = "android.intent.action.PACKAGE_ADDED";
}

} // namespace android::ams
