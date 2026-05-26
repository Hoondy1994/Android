//
// Created by 80244960 on 2023/1/5.
//

#ifndef NAMESPACE_SINGLETON_H
#define NAMESPACE_SINGLETON_H

#include <mutex>
#include <type_traits>

namespace pantanal {
namespace ns {

template<typename T>
class Singleton {
public:
    static T& GetInstance() {
        static_assert(std::is_base_of_v<Singleton, T> == true);
        static T sInstance;
        return sInstance;
    }

    Singleton(const Singleton&)   = delete;
    Singleton(Singleton&&)  = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton& operator=(Singleton&&) = delete;
protected:
    Singleton() {};
    ~Singleton() {}
};

}
}

#endif //NAMESPACE_SINGLETON_H
