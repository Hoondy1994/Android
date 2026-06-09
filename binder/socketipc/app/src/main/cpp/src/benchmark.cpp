#include "ipc/benchmark.hpp"

#include "ipc/custom_codec.hpp"
#include "ipc/parcel_codec.hpp"
#include "ipc/socket_client.hpp"

#include <android/log.h>
#include <algorithm>
#include <unistd.h>

#define LOG_TAG "SocketIPC"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#include <chrono>
#include <mutex>
#include <numeric>
#include <thread>
#include <vector>

namespace ipc {

namespace {

using Clock = std::chrono::steady_clock;

[[nodiscard]] double to_ms(const Clock::duration duration) {
    return std::chrono::duration<double, std::milli>(duration).count();
}

[[nodiscard]] BenchmarkStats summarize(std::vector<double>& samples, std::size_t success,
                                       std::size_t failure) {
    BenchmarkStats stats;
    stats.success = success;
    stats.failure = failure;
    if (samples.empty()) {
        return stats;
    }

    std::sort(samples.begin(), samples.end());
    stats.min_ms = samples.front();
    stats.max_ms = samples.back();
    stats.avg_ms = std::accumulate(samples.begin(), samples.end(), 0.0) /
                   static_cast<double>(samples.size());
    const auto p95_index =
        std::min(samples.size() - 1, static_cast<std::size_t>(samples.size() * 0.95));
    stats.p95_ms = samples[p95_index];
    return stats;
}

void run_serialization_worker(const SerializationKind kind, const int iterations,
                              std::vector<double>& samples, std::mutex& mutex,
                              std::size_t& success, std::size_t& failure) {
    const auto payload =
        (kind == SerializationKind::kCustom)
            ? CustomCodec::encode(UserProfile::sample(42))
            : ParcelCodec::encode(UserProfile::sample(42));

    for (int i = 0; i < iterations; ++i) {
        const auto start = Clock::now();
        try {
            if (kind == SerializationKind::kCustom) {
                const auto profile = CustomCodec::decode(payload);
                (void)CustomCodec::encode(profile.transformed());
            } else {
                const auto profile = ParcelCodec::decode(payload);
                (void)ParcelCodec::encode(profile.transformed());
            }
            const auto elapsed = to_ms(Clock::now() - start);
            std::lock_guard lock(mutex);
            samples.push_back(elapsed);
            ++success;
        } catch (...) {
            std::lock_guard lock(mutex);
            ++failure;
        }
    }
}

void run_socket_worker(const SocketBenchmarkMode mode, const int iterations, SocketClient& client,
                       std::vector<double>& samples, std::mutex& mutex, std::size_t& success,
                       std::size_t& failure) {
    const auto custom_payload = CustomCodec::encode(UserProfile::sample(7));
    const auto parcel_payload = ParcelCodec::encode(UserProfile::sample(7));

    for (int i = 0; i < iterations; ++i) {
        const auto start = Clock::now();
        bool ok = false;
        switch (mode) {
            case SocketBenchmarkMode::kAdd:
                ok = client.add(i, i + 1) >= 0;
                break;
            case SocketBenchmarkMode::kCustom:
                ok = client.process_custom(custom_payload).has_value();
                break;
            case SocketBenchmarkMode::kParcel:
                ok = client.process_parcel(parcel_payload).has_value();
                break;
        }
        const auto elapsed = to_ms(Clock::now() - start);
        std::lock_guard lock(mutex);
        if (ok) {
            samples.push_back(elapsed);
            ++success;
        } else {
            ++failure;
        }
    }
}

[[nodiscard]] BenchmarkStats run_parallel(const int thread_count, const int iterations,
                                          const auto& worker) {
    std::vector<double> samples;
    samples.reserve(static_cast<std::size_t>(thread_count * iterations));
    std::mutex mutex;
    std::size_t success = 0;
    std::size_t failure = 0;

    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(thread_count));
    for (int t = 0; t < thread_count; ++t) {
        workers.emplace_back([&, t]() {
            (void)t;
            worker(iterations, samples, mutex, success, failure);
        });
    }
    for (auto& worker_thread : workers) {
        if (worker_thread.joinable()) {
            worker_thread.join();
        }
    }
    return summarize(samples, success, failure);
}

}  // namespace

BenchmarkStats run_serialization_benchmark(const SerializationKind kind, const int thread_count,
                                           const int iterations) {
    LOGI("[Flow:BenchSerialization] 开始 kind=%d, threads=%d, iterations=%d, pid=%d",
         static_cast<int>(kind), thread_count, iterations, getpid());
    const auto stats = run_parallel(thread_count, iterations,
                        [kind](const int loop, std::vector<double>& samples, std::mutex& mutex,
                               std::size_t& success, std::size_t& failure) {
                            run_serialization_worker(kind, loop, samples, mutex, success, failure);
                        });
    LOGI("[Flow:BenchSerialization] 完成 %s", stats.format().c_str());
    return stats;
}

BenchmarkStats run_socket_benchmark(const SocketBenchmarkMode mode, const int thread_count,
                                    const int iterations) {
    LOGI("[Flow:BenchSocket] 开始 mode=%d, threads=%d, iterations=%d, pid=%d",
         static_cast<int>(mode), thread_count, iterations, getpid());
    auto& client = global_socket_client();
    const auto stats = run_parallel(thread_count, iterations,
                        [mode, &client](const int loop, std::vector<double>& samples,
                                        std::mutex& mutex, std::size_t& success,
                                        std::size_t& failure) {
                            run_socket_worker(mode, loop, client, samples, mutex, success, failure);
                        });
    LOGI("[Flow:BenchSocket] 完成 %s", stats.format().c_str());
    return stats;
}

}  // namespace ipc
