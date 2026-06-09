// 死锁复现版：去掉地址排序和 std::lock，按 from→to 顺序加锁
// 编译: g++ -std=c++11 -pthread bank_account_transfer_deadlock.cpp -o deadlock_demo
// 预期: 程序卡在 join()，永远打印不出余额

#include <chrono>
#include <mutex>
#include <thread>
#include <vector>
#include <iostream>
#include <random>

class BankAccount {
    int balance_;
    mutable std::mutex mtx_;

public:
    explicit BankAccount(int balance) : balance_(balance) {}

    // 错误写法：始终先锁 from 再锁 to，反向转账时会死锁
    static bool transfer(BankAccount& from, BankAccount& to, int amount) {
        std::lock_guard<std::mutex> lk_from(from.mtx_);
        // 故意停顿，让另一个线程有机会拿到 to 的锁，死锁几乎必现
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        std::lock_guard<std::mutex> lk_to(to.mtx_);

        if (from.balance_ < amount) return false;
        from.balance_ -= amount;
        to.balance_   += amount;
        return true;
    }

    int getBalance() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return balance_;
    }
};

int main() {
    std::cout << "启动死锁复现 demo（可能会卡住）...\n";

    BankAccount alice(10000), bob(5000), charlie(3000);
    std::vector<std::thread> threads;

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(1, 500);

    for (int i = 0; i < 100; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 50; ++j) {
                int amt = dist(rng);
                switch (j % 3) {
                    case 0: BankAccount::transfer(alice, bob, amt); break;
                    case 1: BankAccount::transfer(bob, charlie, amt); break;
                    case 2: BankAccount::transfer(charlie, alice, amt); break;
                }
            }
        });
    }

    std::cout << "等待线程结束（若下面没有余额输出 = 已死锁）...\n";
    for (auto& t : threads) t.join();

    std::cout << "Alice: "   << alice.getBalance()   << "\n"
              << "Bob: "     << bob.getBalance()     << "\n"
              << "Charlie: " << charlie.getBalance() << "\n";

    return 0;
}
