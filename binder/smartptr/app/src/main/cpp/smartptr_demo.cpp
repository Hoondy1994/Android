#include "utils/StrongPointer.h"
#include "utils/WeakPointer.h"

#include <sstream>
#include <string>

namespace demo {

using android::RefBase;
using android::sp;
using android::wp;

// 模拟 Binder 里常见的 RefBase 派生对象
class MyService : public RefBase {
public:
    explicit MyService(const char* name) : mName(name) {
        mAliveCount++;
    }

    ~MyService() override {
        mAliveCount--;
        mDestroyed = true;
    }

    const char* name() const { return mName; }

    static int aliveCount() { return mAliveCount; }
    static bool wasDestroyed() { return mDestroyed; }

    void onLastStrongRef(const void* /*id*/) override {
        // 最后一处 sp 释放时可做清理（类似 Binder 对象析构路径）
        mLastStrongReleased = true;
    }

    static bool lastStrongReleased() { return mLastStrongReleased; }

    static void resetStatics() {
        mAliveCount = 0;
        mDestroyed = false;
        mLastStrongReleased = false;
    }

private:
    const char* mName;
    static inline int mAliveCount = 0;
    static inline bool mDestroyed = false;
    static inline bool mLastStrongReleased = false;
};

static std::string runDemo() {
    MyService::resetStatics();
    std::ostringstream out;

    auto line = [&](const std::string& s) {
        out << s << '\n';
    };

    line("=== Binder 风格智能指针 Demo (sp / wp) ===\n");

    // 1. sp 基本用法
    {
        sp<MyService> svc = new MyService("AudioService");
        line("[1] sp 持有对象: " + std::string(svc->name()));
        line("    strong=" + std::to_string(svc->getStrongCount()) +
             " alive=" + std::to_string(MyService::aliveCount()));
    }
    line("    离开作用域后 alive=" + std::to_string(MyService::aliveCount()) +
         " onLastStrongRef=" + (MyService::lastStrongReleased() ? "yes" : "no"));
    line("");

    MyService::resetStatics();

    // 2. sp 拷贝共享所有权
    {
        sp<MyService> a = new MyService("CameraService");
        sp<MyService> b = a;
        line("[2] 两个 sp 共享: strong(a)=" + std::to_string(a->getStrongCount()));
        b.clear();
        line("    b.clear() 后 strong(a)=" + std::to_string(a->getStrongCount()));
    }
    line("    作用域结束 alive=" + std::to_string(MyService::aliveCount()));
    line("");

    MyService::resetStatics();

    // 3. wp 在 sp 释放后 promote 失败
    wp<MyService> weak;
    {
        sp<MyService> svc = new MyService("TempService");
        weak = svc;
        line("[3] wp 观察中: strong=" + std::to_string(svc->getStrongCount()) +
             " weak=" + std::to_string(svc->getWeakCount()));
    }
    sp<MyService> promoted = weak.promote();
    line("    sp 已销毁, promote() " + std::string(promoted ? "成功(异常)" : "失败(预期)"));
    line("    对象 destroyed=" + std::string(MyService::wasDestroyed() ? "yes" : "no"));
    line("");

    MyService::resetStatics();

    // 4. wp 在对象仍存活时 promote 成功
    sp<MyService> holder = new MyService("LongLived");
    wp<MyService> weak2 = holder;
    sp<MyService> back = weak2.promote();
    line("[4] 对象仍存活时 promote() " + std::string(back ? "成功" : "失败"));
    if (back) {
        line("    名称=" + std::string(back->name()) +
             " strong=" + std::to_string(back->getStrongCount()));
    }
    line("");

    line("=== 要点 ===");
    line("sp<T> : 强引用，计数为 0 时销毁对象 (Binder/RefBase 核心)");
    line("wp<T> : 弱引用，不阻止销毁；promote() 可安全取回 sp");
    line("Binder 中 BpBinder/BBinder 等均基于 RefBase + sp/wp");

    return out.str();
}

}  // namespace demo

std::string runSmartPtrDemo() {
    return demo::runDemo();
}
