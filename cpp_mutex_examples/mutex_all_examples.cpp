// Complete C++ multithreading mutex examples (MSVC / C++17 friendly)
// ASCII-only source. Compile: cl /std:c++17 /EHsc mutex_all_examples.cpp
// Or g++: g++ -std=c++17 -pthread mutex_all_examples.cpp -o mutex_all_examples

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <random>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>

#if __cplusplus >= 202002L
#include <semaphore>
#define HAS_CPP20_SEMAPHORE 1
#endif

// ============================================================================
// 1. std::mutex + std::lock_guard + std::lock
//    Scenario: bank transfer between multiple accounts
// ============================================================================
class BankAccount {
    int balance_;
    mutable std::mutex mtx_;

public:
    explicit BankAccount(int balance) : balance_(balance) {}

    static bool transfer(BankAccount& from, BankAccount& to, int amount) {
        BankAccount* first = nullptr;
        BankAccount* second = nullptr;

        if (&from < &to) {
            first = &from;
            second = &to;
        } else {
            first = &to;
            second = &from;
        }

        std::lock(first->mtx_, second->mtx_);
        std::lock_guard<std::mutex> lk1(first->mtx_, std::adopt_lock);
        std::lock_guard<std::mutex> lk2(second->mtx_, std::adopt_lock);

        if (from.balance_ < amount) {
            return false;
        }

        from.balance_ -= amount;
        to.balance_ += amount;
        return true;
    }

    int getBalance() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return balance_;
    }
};

static void demo_mutex_bank_transfer() {
    std::cout << "\n=== 1. std::mutex: bank transfer ===\n";

    BankAccount alice(10000);
    BankAccount bob(5000);
    BankAccount charlie(3000);

    std::vector<std::thread> threads;
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(1, 200);

    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 100; ++j) {
                const int amt = dist(rng);
                switch (j % 3) {
                    case 0: BankAccount::transfer(alice, bob, amt); break;
                    case 1: BankAccount::transfer(bob, charlie, amt); break;
                    case 2: BankAccount::transfer(charlie, alice, amt); break;
                    default: break;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    const int total = alice.getBalance() + bob.getBalance() + charlie.getBalance();
    std::cout << "Alice=" << alice.getBalance()
              << " Bob=" << bob.getBalance()
              << " Charlie=" << charlie.getBalance()
              << " Total=" << total << " (expected 18000)\n";
}

// ============================================================================
// 2. std::recursive_mutex
//    Scenario: recursive tree traversal with lazy loading
// ============================================================================
struct DirNode {
    std::string name;
    std::vector<std::shared_ptr<DirNode>> children;
};

class FileSystem {
    std::shared_ptr<DirNode> root_;
    std::recursive_mutex mtx_;
    int lazy_bonus_ = 0;

    void lazyLoadChildren(const DirNode& node) {
        std::lock_guard<std::recursive_mutex> lk(mtx_);
        lazy_bonus_ += 10;
        std::cout << "  lazy load under: " << node.name << ", bonus=" << lazy_bonus_ << "\n";
    }

    int countFilesRecursive(const DirNode& node, int depth) {
        std::lock_guard<std::recursive_mutex> lk(mtx_);

        int count = node.children.empty() ? 1 : 0;
        for (const auto& child : node.children) {
            count += countFilesRecursive(*child, depth + 1);
        }

        if (depth == 1 && node.name == "lazy") {
            lazyLoadChildren(node);
        }

        return count;
    }

public:
    explicit FileSystem(std::shared_ptr<DirNode> root) : root_(std::move(root)) {}

    int countAllFiles() {
        std::lock_guard<std::recursive_mutex> lk(mtx_);
        return countFilesRecursive(*root_, 0);
    }
};

static std::shared_ptr<DirNode> makeSampleTree() {
    auto root = std::make_shared<DirNode>(DirNode{"root", {}});
    auto docs = std::make_shared<DirNode>(DirNode{"docs", {}});
    auto lazy = std::make_shared<DirNode>(DirNode{"lazy", {}});
    docs->children.push_back(std::make_shared<DirNode>(DirNode{"a.txt", {}}));
    docs->children.push_back(std::make_shared<DirNode>(DirNode{"b.txt", {}}));
    lazy->children.push_back(std::make_shared<DirNode>(DirNode{"c.txt", {}}));
    root->children.push_back(docs);
    root->children.push_back(lazy);
    return root;
}

static void demo_recursive_mutex() {
    std::cout << "\n=== 2. std::recursive_mutex: tree traversal ===\n";

    FileSystem fs(makeSampleTree());
    std::cout << "file count=" << fs.countAllFiles() << "\n";
}

// ============================================================================
// 3. std::timed_mutex
//    Scenario: render pool with frame budget timeout
// ============================================================================
class RenderResourcePool {
    std::timed_mutex mtx_;
    std::vector<int> slots_;
    static constexpr int kMaxSlots = 4;

public:
    RenderResourcePool() : slots_(kMaxSlots, -1) {}

    bool acquireTexture(int frame_id, int timeout_ms) {
        if (!mtx_.try_lock_for(std::chrono::milliseconds(timeout_ms))) {
            std::cout << "  frame " << frame_id << " dropped (lock timeout)\n";
            return false;
        }

        for (int i = 0; i < kMaxSlots; ++i) {
            if (slots_[i] == -1) {
                slots_[i] = frame_id;
                mtx_.unlock();
                return true;
            }
        }

        mtx_.unlock();
        std::cout << "  frame " << frame_id << " dropped (pool full)\n";
        return false;
    }

    void releaseTexture(int frame_id) {
        std::lock_guard<std::timed_mutex> lk(mtx_);
        for (auto& slot : slots_) {
            if (slot == frame_id) {
                slot = -1;
            }
        }
    }
};

static void demo_timed_mutex() {
    std::cout << "\n=== 3. std::timed_mutex: render pool ===\n";

    RenderResourcePool pool;
    std::atomic<int> acquired{0};

    std::thread renderer([&]() {
        for (int f = 0; f < 20; ++f) {
            if (pool.acquireTexture(f, 8)) {
                ++acquired;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    });

    std::thread releaser([&]() {
        for (int f = 0; f < 20; f += 2) {
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
            pool.releaseTexture(f);
        }
    });

    renderer.join();
    releaser.join();
    std::cout << "acquired frames=" << acquired << "\n";
}

// ============================================================================
// 4. std::shared_mutex + shared_lock / unique_lock
//    Scenario: read-heavy cache
// ============================================================================
template <typename K, typename V>
class RWCache {
    std::map<K, V> data_;
    mutable std::shared_mutex mtx_;
    mutable std::atomic<uint64_t> hits_{0};
    mutable std::atomic<uint64_t> misses_{0};

public:
    std::optional<V> get(const K& key) const {
        std::shared_lock<std::shared_mutex> read_lk(mtx_);
        auto it = data_.find(key);
        if (it != data_.end()) {
            ++hits_;
            return it->second;
        }
        ++misses_;
        return std::nullopt;
    }

    void put(const K& key, const V& value) {
        std::unique_lock<std::shared_mutex> write_lk(mtx_);
        data_[key] = value;
    }

    V getOrCompute(const K& key, const std::function<V()>& compute) {
        {
            std::shared_lock<std::shared_mutex> read_lk(mtx_);
            auto it = data_.find(key);
            if (it != data_.end()) {
                ++hits_;
                return it->second;
            }
        }

        std::unique_lock<std::shared_mutex> write_lk(mtx_);
        auto it = data_.find(key);
        if (it != data_.end()) {
            return it->second;
        }

        V value = compute();
        data_[key] = value;
        ++misses_;
        return value;
    }

    void printStats() const {
        std::shared_lock<std::shared_mutex> read_lk(mtx_);
        std::cout << "  size=" << data_.size()
                  << " hits=" << hits_.load()
                  << " misses=" << misses_.load() << "\n";
    }
};

static void demo_shared_mutex() {
    std::cout << "\n=== 4. std::shared_mutex: RW cache ===\n";

    RWCache<std::string, int> cache;

    std::vector<std::thread> threads;
    for (int i = 0; i < 6; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 200; ++j) {
                cache.get("key_" + std::to_string(j % 20));
            }
        });
    }
    for (int i = 0; i < 2; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < 20; ++j) {
                cache.put("key_" + std::to_string(j + i * 10), j);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    cache.printStats();
}

// ============================================================================
// 5. std::unique_lock + std::condition_variable
//    Scenario: bounded producer-consumer queue
// ============================================================================
template <typename T>
class BoundedQueue {
    std::queue<T> queue_;
    size_t capacity_;
    std::mutex mtx_;
    std::condition_variable not_full_;
    std::condition_variable not_empty_;
    bool shutdown_ = false;

public:
    explicit BoundedQueue(size_t capacity) : capacity_(capacity) {}

    bool push(T item, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lk(mtx_);
        if (!not_full_.wait_for(lk, timeout, [&] {
                return queue_.size() < capacity_ || shutdown_;
            })) {
            return false;
        }

        if (shutdown_) {
            return false;
        }

        queue_.push(std::move(item));
        lk.unlock();
        not_empty_.notify_one();
        return true;
    }

    std::optional<T> pop(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lk(mtx_);
        if (!not_empty_.wait_for(lk, timeout, [&] {
                return !queue_.empty() || shutdown_;
            })) {
            return std::nullopt;
        }

        if (queue_.empty()) {
            return std::nullopt;
        }

        T item = std::move(queue_.front());
        queue_.pop();
        not_full_.notify_one();
        return item;
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            shutdown_ = true;
        }
        not_full_.notify_all();
        not_empty_.notify_all();
    }
};

static void demo_condition_variable() {
    std::cout << "\n=== 5. condition_variable: bounded queue ===\n";

    BoundedQueue<int> queue(8);
    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};

    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    for (int i = 0; i < 3; ++i) {
        producers.emplace_back([&, i]() {
            for (int j = 0; j < 50; ++j) {
                if (queue.push(i * 100 + j, std::chrono::milliseconds(100))) {
                    ++produced;
                }
            }
        });
    }

    for (int i = 0; i < 2; ++i) {
        consumers.emplace_back([&]() {
            while (true) {
                auto item = queue.pop(std::chrono::milliseconds(200));
                if (!item) {
                    break;
                }
                ++consumed;
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
            }
        });
    }

    for (auto& t : producers) {
        t.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    queue.shutdown();

    for (auto& t : consumers) {
        t.join();
    }

    std::cout << "  produced=" << produced << " consumed=" << consumed << "\n";
}

// ============================================================================
// 6. std::scoped_lock (C++17)
//    Scenario: task scheduler locking multiple resources safely
// ============================================================================
class TaskScheduler {
    std::mutex resource_mtx_[4];
    std::map<int, std::string> tasks_;
    std::mutex tasks_mtx_;

    void lockResources(const std::vector<int>& resources) {
        if (resources == std::vector<int>{0, 2}) {
            std::scoped_lock lock(resource_mtx_[0], resource_mtx_[2]);
        } else if (resources == std::vector<int>{1, 3}) {
            std::scoped_lock lock(resource_mtx_[1], resource_mtx_[3]);
        } else if (resources.size() == 3) {
            std::scoped_lock lock(resource_mtx_[0], resource_mtx_[1], resource_mtx_[2]);
        }
    }

public:
    void executeTask(int task_id, const std::vector<int>& resources) {
        lockResources(resources);

        std::lock_guard<std::mutex> lk(tasks_mtx_);
        tasks_[task_id] = "done";
        std::cout << "  task " << task_id << " completed\n";
    }
};

static void demo_scoped_lock() {
    std::cout << "\n=== 6. std::scoped_lock: multi-resource tasks ===\n";

    TaskScheduler scheduler;
    std::vector<std::thread> threads;

    threads.emplace_back([&]() { scheduler.executeTask(1, {0, 2}); });
    threads.emplace_back([&]() { scheduler.executeTask(2, {1, 3}); });
    threads.emplace_back([&]() { scheduler.executeTask(3, {0, 1, 2}); });
    threads.emplace_back([&]() { scheduler.executeTask(4, {2, 0}); });

    for (auto& t : threads) {
        t.join();
    }
}

// ============================================================================
// 7. std::atomic (lock-free counters / state machine)
// ============================================================================
class LockFreeStats {
    std::atomic<uint64_t> total_requests_{0};
    std::atomic<uint64_t> total_errors_{0};
    std::atomic<int> state_{0};

    static constexpr int kStateInit = 0;
    static constexpr int kStateRunning = 1;
    static constexpr int kStateShutdown = 2;

public:
    bool tryStart() {
        int expected = kStateInit;
        return state_.compare_exchange_strong(
            expected, kStateRunning, std::memory_order_acq_rel, std::memory_order_acquire);
    }

    void recordRequest(bool success) {
        total_requests_.fetch_add(1, std::memory_order_relaxed);
        if (!success) {
            total_errors_.fetch_add(1, std::memory_order_relaxed);
        }
    }

    bool tryShutdown() {
        int expected = kStateRunning;
        return state_.compare_exchange_strong(
            expected, kStateShutdown, std::memory_order_acq_rel, std::memory_order_acquire);
    }

    void print() const {
        std::cout << "  requests=" << total_requests_.load(std::memory_order_acquire)
                  << " errors=" << total_errors_.load(std::memory_order_acquire)
                  << " state=" << state_.load(std::memory_order_acquire) << "\n";
    }
};

static void demo_atomic() {
    std::cout << "\n=== 7. std::atomic: lock-free stats ===\n";

    LockFreeStats stats;
    stats.tryStart();

    std::vector<std::thread> workers;
    for (int i = 0; i < 8; ++i) {
        workers.emplace_back([&]() {
            for (int j = 0; j < 50000; ++j) {
                stats.recordRequest(j % 100 != 0);
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    stats.tryShutdown();
    stats.print();
}

// ============================================================================
// 8. std::call_once + std::once_flag
//    Scenario: thread-safe singleton initialization
// ============================================================================
class DatabaseConnectionPool {
    struct Impl {
        int max_connections = 0;
        std::vector<int> connections;

        Impl() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            max_connections = 5;
            for (int i = 0; i < max_connections; ++i) {
                connections.push_back(i);
            }
            std::cout << "  pool initialized, max=" << max_connections << "\n";
        }
    };

    static std::once_flag init_flag_;
    static std::unique_ptr<Impl> instance_;

    static void init() {
        instance_ = std::make_unique<Impl>();
    }

public:
    static Impl& getInstance() {
        std::call_once(init_flag_, init);
        return *instance_;
    }

    static int borrowConnection() {
        auto& pool = getInstance();
        static std::mutex borrow_mtx;
        std::lock_guard<std::mutex> lk(borrow_mtx);
        if (pool.connections.empty()) {
            return -1;
        }
        const int conn = pool.connections.back();
        pool.connections.pop_back();
        return conn;
    }
};

std::once_flag DatabaseConnectionPool::init_flag_;
std::unique_ptr<DatabaseConnectionPool::Impl> DatabaseConnectionPool::instance_;

static void demo_call_once() {
    std::cout << "\n=== 8. std::call_once: singleton pool ===\n";

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([i]() {
            const int conn = DatabaseConnectionPool::borrowConnection();
            std::cout << "  thread " << i << " conn=" << conn << "\n";
        });
    }

    for (auto& t : threads) {
        t.join();
    }
}

// ============================================================================
// 9. std::counting_semaphore (C++20, optional)
// ============================================================================
#if defined(HAS_CPP20_SEMAPHORE)

class RateLimiter {
    std::counting_semaphore<10> sem_;

public:
    RateLimiter() : sem_(10) {}

    void execute(const std::function<void()>& task) {
        sem_.acquire();
        task();
        sem_.release();
    }
};

static void demo_semaphore() {
    std::cout << "\n=== 9. std::semaphore: rate limiter ===\n";

    RateLimiter limiter;
    std::atomic<int> max_concurrent{0};
    std::atomic<int> current{0};

    std::vector<std::thread> threads;
    for (int i = 0; i < 30; ++i) {
        threads.emplace_back([&, i]() {
            limiter.execute([&]() {
                const int now = ++current;
                int prev = max_concurrent.load();
                while (now > prev && !max_concurrent.compare_exchange_weak(prev, now)) {
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                --current;
            });
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    std::cout << "  max concurrent=" << max_concurrent << " (should be <= 10)\n";
}

#else

static void demo_semaphore() {
    std::cout << "\n=== 9. std::semaphore: skipped (need C++20) ===\n";
}

#endif

// ============================================================================
// main
// ============================================================================
int main() {
    std::cout << "C++ mutex examples start\n";

    demo_mutex_bank_transfer();
    demo_recursive_mutex();
    demo_timed_mutex();
    demo_shared_mutex();
    demo_condition_variable();
    demo_scoped_lock();
    demo_atomic();
    demo_call_once();
    demo_semaphore();

    std::cout << "\nAll demos finished.\n";
    return 0;
}
