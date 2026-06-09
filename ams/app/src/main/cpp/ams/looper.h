#pragma once
#include <functional>
#include <variant>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <optional>
#include <string>
#include <chrono>
#include "ams_types.h"

namespace android::ams {

// ─── Message payload variants ─────────────────────────────────────────────────
struct MsgStartActivity  { std::string pkg; std::string cls; LaunchFlag flags; };
struct MsgPauseActivity  { ActivityId id; };
struct MsgResumeActivity { ActivityId id; };
struct MsgStopActivity   { ActivityId id; };
struct MsgDestroyActivity{ ActivityId id; };
struct MsgStartService   { std::string pkg; std::string svc; };
struct MsgStopService    { ServiceId id; };
struct MsgBroadcast      { std::string action; };
struct MsgQuit           {};

using MsgPayload = std::variant<
    MsgStartActivity,
    MsgPauseActivity,
    MsgResumeActivity,
    MsgStopActivity,
    MsgDestroyActivity,
    MsgStartService,
    MsgStopService,
    MsgBroadcast,
    MsgQuit
>;

struct Message {
    int64_t     whenMs{0};   // monotonic ms, 0 = ASAP
    int32_t     what{0};
    MsgPayload  payload;
    std::function<void()> callback;  // optional arbitrary callback

    bool operator>(const Message& o) const { return whenMs > o.whenMs; }
};

// ─── Thread-safe priority message queue ───────────────────────────────────────
class MessageQueue {
public:
    void post(Message msg) {
        std::lock_guard lk(mtx_);
        pq_.push(std::move(msg));
        cv_.notify_one();
    }

    // Block until a message is due; returns nullopt on quit
    std::optional<Message> next() {
        std::unique_lock lk(mtx_);
        cv_.wait(lk, [&]{ return quit_ || !pq_.empty(); });
        if (quit_ && pq_.empty()) return std::nullopt;
        // Wait until due time
        if (!pq_.empty()) {
            auto& top = pq_.top();
            auto now = nowMs();
            if (top.whenMs > now) {
                cv_.wait_for(lk, std::chrono::milliseconds(top.whenMs - now));
            }
        }
        if (pq_.empty()) return std::nullopt;
        auto msg = pq_.top(); pq_.pop();
        return msg;
    }

    void quit() {
        std::lock_guard lk(mtx_);
        quit_ = true;
        cv_.notify_all();
    }

    [[nodiscard]] bool empty() const {
        std::lock_guard lk(mtx_);
        return pq_.empty();
    }

private:
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::priority_queue<Message,
        std::vector<Message>,
        std::greater<Message>> pq_;
    bool quit_{false};
};

// ─── Looper: drives a MessageQueue on its own thread ─────────────────────────
class Looper {
public:
    using Handler = std::function<void(const Message&)>;

    explicit Looper(Handler h) : handler_(std::move(h)) {}
    ~Looper() { stop(); }

    void start() {
        running_ = true;
        thread_ = std::thread([this] { loop(); });
    }

    void stop() {
        if (running_.exchange(false)) {
            mq_.quit();
            if (thread_.joinable()) thread_.join();
        }
    }

    void post(Message msg) { mq_.post(std::move(msg)); }

    void postDelayed(Message msg, int64_t delayMs) {
        msg.whenMs = nowMs() + delayMs;
        mq_.post(std::move(msg));
    }

private:
    void loop() {
        while (running_) {
            auto opt = mq_.next();
            if (!opt) break;
            if (opt->callback) opt->callback();
            else               handler_(*opt);
        }
    }

    Handler              handler_;
    MessageQueue         mq_;
    std::thread          thread_;
    std::atomic<bool>    running_{false};
};

} // namespace android::ams
