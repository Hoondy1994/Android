// Bank transfer demo: std::mutex + std::lock (C++11 compatible)
// Keep this file ASCII-only to avoid MSVC code page 936 parse errors.

#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

class BankAccount {
    int balance_;
    mutable std::mutex mtx_;

public:
    explicit BankAccount(int balance) : balance_(balance) {}

    static bool transfer(BankAccount& from, BankAccount& to, int amount) {
        // Use pointers instead of "auto& first/second" for older MSVC parsers.
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

int main() {
    BankAccount alice(10000);
    BankAccount bob(5000);
    BankAccount charlie(3000);

    std::vector<std::thread> threads;
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(1, 500);

    for (int i = 0; i < 100; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 50; ++j) {
                const int amt = dist(rng);
                switch (j % 3) {
                    case 0:
                        BankAccount::transfer(alice, bob, amt);
                        break;
                    case 1:
                        BankAccount::transfer(bob, charlie, amt);
                        break;
                    case 2:
                        BankAccount::transfer(charlie, alice, amt);
                        break;
                    default:
                        break;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    const int total = alice.getBalance() + bob.getBalance() + charlie.getBalance();
    std::cout << "Alice:   " << alice.getBalance() << '\n';
    std::cout << "Bob:     " << bob.getBalance() << '\n';
    std::cout << "Charlie: " << charlie.getBalance() << '\n';
    std::cout << "Total:   " << total << " (expected 18000)\n";

    return 0;
}
