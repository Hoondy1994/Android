# C++ 硬核笔试题 第 1-50 道（含代码 + 解析）

> 涵盖：模板元编程、内存管理、移动语义、并发原子、虚函数多态、类型系统

---

## 第一章：模板元编程（题目 1-30）

---

### 题目 1：编译期阶乘与溢出检测

```cpp
#include <iostream>
#include <limits>
#include <type_traits>

template<unsigned long long N>
struct Factorial {
    static_assert(N <= 20, "Factorial overflow for unsigned long long");
    static constexpr unsigned long long value = N * Factorial<N-1>::value;
};
template<>
struct Factorial<0> {
    static constexpr unsigned long long value = 1;
};

// 改进版：用 constexpr 函数
constexpr unsigned long long factorial(unsigned n) {
    return n == 0 ? 1 : n * factorial(n - 1);
}

// 编译期检测溢出
template<unsigned long long A, unsigned long long B>
struct SafeMul {
    static constexpr bool overflow = (B != 0) && (A > std::numeric_limits<unsigned long long>::max() / B);
    static constexpr unsigned long long value = overflow ? 0 : A * B;
};

int main() {
    std::cout << Factorial<10>::value << "\n";  // 3628800
    std::cout << Factorial<20>::value << "\n";  // 2432902008176640000
    // Factorial<21>::value;  // static_assert 失败
    
    constexpr auto v = factorial(12);
    static_assert(v == 479001600);
    std::cout << v << "\n";
}
```

**解析**：
- `Factorial<N>` 是经典模板递归，编译器展开为 `N * (N-1) * ... * 1`，所有计算在编译期完成。
- `static_assert(N <= 20)` 防止 `unsigned long long` 溢出（21! > 2^64）。
- `constexpr` 函数版本更灵活，支持运行期调用（非 constexpr 上下文）。
- `SafeMul` 演示了编译期溢出检测：用除法反向验证乘法不溢出。
- **关键点**：模板特化必须放在主模板之后；`constexpr` 递归函数在 C++11 中只能有一条 `return` 语句，C++14 放开此限制。

---

### 题目 2：类型列表与索引访问

```cpp
#include <iostream>
#include <type_traits>

// 类型列表
template<typename... Ts>
struct TypeList {
    static constexpr std::size_t size = sizeof...(Ts);
};

// 获取第 N 个类型
template<std::size_t N, typename List>
struct TypeAt;

template<std::size_t N, typename Head, typename... Tail>
struct TypeAt<N, TypeList<Head, Tail...>> {
    using type = typename TypeAt<N-1, TypeList<Tail...>>::type;
};

template<typename Head, typename... Tail>
struct TypeAt<0, TypeList<Head, Tail...>> {
    using type = Head;
};

// 查找类型的索引
template<typename T, typename List, std::size_t Idx = 0>
struct IndexOf;

template<typename T, typename Head, typename... Tail, std::size_t Idx>
struct IndexOf<T, TypeList<Head, Tail...>, Idx> {
    static constexpr std::size_t value =
        std::is_same_v<T, Head> ? Idx : IndexOf<T, TypeList<Tail...>, Idx+1>::value;
};

template<typename T, std::size_t Idx>
struct IndexOf<T, TypeList<>, Idx> {
    static constexpr std::size_t value = static_cast<std::size_t>(-1); // not found
};

// 类型是否包含在列表中
template<typename T, typename List>
struct Contains : std::bool_constant<IndexOf<T, List>::value != static_cast<std::size_t>(-1)> {};

using MyList = TypeList<int, double, char, float>;

static_assert(std::is_same_v<TypeAt<2, MyList>::type, char>);
static_assert(IndexOf<double, MyList>::value == 1);
static_assert(Contains<float, MyList>::value);
static_assert(!Contains<long, MyList>::value);

int main() {
    std::cout << "Size: " << MyList::size << "\n";  // 4
    std::cout << "Index of double: " << IndexOf<double, MyList>::value << "\n";  // 1
}
```

**解析**：
- `TypeList` 是 C++ 元编程的基础数据结构，类似 Haskell 的列表。
- `TypeAt` 用递归偏特化实现线性查找，时间复杂度 O(N)（实例化次数）。
- `IndexOf` 中使用三元运算符的编译期短路：若当前匹配则直接返回 `Idx`，否则递归。
- **陷阱**：`IndexOf` 遇到空列表时返回 `size_t(-1)` 而非编译错误，这是一种"软失败"策略。
- C++17 后可用 `if constexpr` 改写，更直观；C++20 可用 concept 约束。

---

### 题目 3：SFINAE 与函数重载决议

```cpp
#include <iostream>
#include <type_traits>
#include <vector>
#include <list>

// 检测类型是否有 begin()/end()（可迭代）
template<typename T, typename = void>
struct is_iterable : std::false_type {};

template<typename T>
struct is_iterable<T, std::void_t<
    decltype(std::begin(std::declval<T>())),
    decltype(std::end(std::declval<T>()))
>> : std::true_type {};

// 检测是否有 .size() 成员
template<typename T, typename = void>
struct has_size : std::false_type {};

template<typename T>
struct has_size<T, std::void_t<decltype(std::declval<T>().size())>>
    : std::true_type {};

// 根据类型特征选择不同实现
template<typename Container>
std::enable_if_t<is_iterable<Container>::value && has_size<Container>::value, std::size_t>
get_count(const Container& c) {
    std::cout << "[fast path] ";
    return c.size();
}

template<typename Container>
std::enable_if_t<is_iterable<Container>::value && !has_size<Container>::value, std::size_t>
get_count(const Container& c) {
    std::cout << "[slow path] ";
    return std::distance(std::begin(c), std::end(c));
}

int arr[] = {1, 2, 3, 4, 5};

int main() {
    std::vector<int> v{1,2,3};
    std::list<int> l{1,2,3,4};
    
    std::cout << get_count(v) << "\n";    // [fast path] 3
    std::cout << get_count(l) << "\n";    // [fast path] 4  (list 有 size())
    std::cout << get_count(arr) << "\n";  // [slow path] 5  (C数组无 .size())
    
    // 问：int 类型调用 get_count 会发生什么？
    // get_count(42);  // 编译错误：substitution failure，无匹配重载
}
```

**解析**：
- `std::void_t<expr...>` 是 SFINAE 的利器：若所有 `expr` 合法则为 `void`，否则替换失败。
- `std::enable_if_t` 控制函数是否参与重载决议，失败时不报错只是"不存在"。
- C 数组 `int[]` 支持 `std::begin/end`（特化版本），但无 `.size()` 成员。
- **关键陷阱**：`std::list` 在 C++11 后 `.size()` 是 O(1) 的（标准要求），所以走 fast path。
- C++20 用 `concept` + `requires` 替代 SFINAE，代码更清晰。

---

### 题目 4：变参模板折叠表达式

```cpp
#include <iostream>
#include <string>
#include <sstream>

// C++17 折叠表达式
template<typename... Args>
auto sum(Args&&... args) {
    return (... + args);  // 左折叠：((a + b) + c) + d
}

template<typename... Args>
auto sum_right(Args&&... args) {
    return (args + ...);  // 右折叠：a + (b + (c + d))
}

// 打印所有参数（带分隔符）
template<typename Sep, typename... Args>
void print_sep(Sep&& sep, Args&&... args) {
    bool first = true;
    ((std::cout << (first ? (first=false, "") : sep) << args), ...);
    std::cout << "\n";
}

// 编译期求最大值
template<typename T, typename... Ts>
constexpr T max_of(T first, Ts... rest) {
    if constexpr (sizeof...(rest) == 0) return first;
    else {
        auto m = max_of(rest...);
        return first > m ? first : m;
    }
}

// 类型包展开：同时操作多个容器
template<typename F, typename... Containers>
void for_each_container(F&& f, Containers&&... cs) {
    (f(std::forward<Containers>(cs)), ...);
}

int main() {
    std::cout << sum(1, 2, 3, 4, 5) << "\n";        // 15
    std::cout << sum(std::string("a"), "b", "c") << "\n"; // abc
    
    print_sep(", ", 1, 2.5, "hello", 'x');  // 1, 2.5, hello, x
    
    static_assert(max_of(3, 1, 4, 1, 5, 9, 2, 6) == 9);
    
    std::vector<int> v1{1,2}, v2{3,4}, v3{5,6};
    for_each_container([](auto& c){ c.push_back(99); }, v1, v2, v3);
    // v1={1,2,99}, v2={3,4,99}, v3={5,6,99}
    
    // 问：sum() 无参数时会怎样？
    // sum();  // 编译错误：空包 + 无初始值
    // 修复：(0 + ... + args)  使用初始值的折叠形式
    auto s = (0 + ... + std::vector<int>{1,2,3,4,5}); // 编译错误？为什么？
}
```

**解析**：
- 左折叠 `(... op pack)` vs 右折叠 `(pack op ...)`：对加法结果相同，对字符串拼接顺序相同，但对减法/除法结果不同。
- 空包折叠：`(... + args)` 空包时编译错误；`(0 + ... + args)` 空包时返回 0（带初始值的折叠）。
- `(f(cs), ...)` 是逗号折叠，对每个 `cs` 调用 `f`，保证左到右顺序（C++17 保证逗号运算符求值顺序）。
- **最后一行错误**：`std::vector<int>` 不支持 `operator+(int, vector)`，类型不匹配。
- `if constexpr` 在变参递归中避免为空包特化整个函数。

---

### 题目 5：模板特化的偏序规则

```cpp
#include <iostream>

template<typename T, typename U>
struct Foo { static void who() { std::cout << "Primary\n"; } };

template<typename T>
struct Foo<T, T> { static void who() { std::cout << "Same type\n"; } };

template<typename T>
struct Foo<T*, T*> { static void who() { std::cout << "Same pointer\n"; } };

template<typename T, typename U>
struct Foo<T*, U*> { static void who() { std::cout << "Different pointer\n"; } };

template<>
struct Foo<int*, int*> { static void who() { std::cout << "int* int*\n"; } };

int main() {
    Foo<int, double>::who();      // Primary
    Foo<int, int>::who();         // Same type
    Foo<int*, double*>::who();    // Different pointer
    Foo<int*, int*>::who();       // int* int*  （完全特化优先）
    Foo<double*, double*>::who(); // Same pointer 还是 Same type？
    
    // 分析：double* 与 double* 同时匹配：
    // - Foo<T,T>  with T=double*
    // - Foo<T*,T*> with T=double
    // 哪个更特殊？
}
```

**解析**：
- 模板偏特化的选择遵循**偏序规则**：更特殊的偏特化优先。
- `Foo<double*, double*>` 同时匹配 `Foo<T,T>` 和 `Foo<T*,T*>`。
- 偏序比较：`Foo<T*,T*>` 比 `Foo<T,T>` 更特殊（因为 `T*` 是 `T` 的子集），所以选 `Foo<T*,T*>`，输出 `Same pointer`。
- `Foo<int*,int*>` 是**完全特化**，优先级最高，直接选中。
- **规则总结**：完全特化 > 最特殊的偏特化 > 主模板。
- 若两个偏特化无法比较偏序（互不包含），编译器报"模糊"错误。

---

### 题目 6：std::tuple 实现原理

```cpp
#include <iostream>
#include <type_traits>

// 手写简化版 Tuple
template<typename... Ts>
struct Tuple;

template<>
struct Tuple<> {};

template<typename Head, typename... Tail>
struct Tuple<Head, Tail...> : Tuple<Tail...> {
    Head head;
    
    Tuple() = default;
    
    template<typename H, typename... T>
    Tuple(H&& h, T&&... t)
        : Tuple<Tail...>(std::forward<T>(t)...)
        , head(std::forward<H>(h)) {}
};

// get<N> 实现
template<std::size_t N, typename... Ts>
auto& get(Tuple<Ts...>& t) {
    if constexpr (N == 0) return t.head;
    else return get<N-1>(static_cast<typename Tuple<Ts...>::base&>(t));
}

// 更正确的实现用基类类型
template<std::size_t N, typename Head, typename... Tail>
struct TupleElement {
    using type = typename TupleElement<N-1, Tail...>::type;
};
template<typename Head, typename... Tail>
struct TupleElement<0, Head, Tail...> {
    using type = Head;
};

// 展开 tuple 调用函数（std::apply 的简化版）
template<typename F, typename Tuple, std::size_t... Is>
auto apply_impl(F&& f, Tuple&& t, std::index_sequence<Is...>) {
    return f(std::get<Is>(std::forward<Tuple>(t))...);
}

template<typename F, typename... Ts>
auto my_apply(F&& f, std::tuple<Ts...>&& t) {
    return apply_impl(std::forward<F>(f), std::move(t),
                      std::index_sequence_for<Ts...>{});
}

int main() {
    std::tuple<int, double, std::string> t{1, 3.14, "hello"};
    
    auto result = my_apply([](int i, double d, const std::string& s) {
        return std::to_string(i) + " " + std::to_string(d) + " " + s;
    }, std::move(t));
    
    std::cout << result << "\n";
    
    // 问：以下代码输出什么？
    auto t2 = std::make_tuple(1, 2, 3);
    auto t3 = std::make_tuple(1, 2, 3);
    std::cout << (t2 == t3) << "\n";  // ?
    std::cout << (t2 < std::make_tuple(1, 2, 4)) << "\n";  // ?
}
```

**解析**：
- `Tuple` 用递归继承实现：`Tuple<int,double,char>` 继承自 `Tuple<double,char>`，再继承自 `Tuple<char>`，最终继承 `Tuple<>`。
- 每层存储 `head`，通过继承链访问后续元素。
- `std::index_sequence` 是 C++14 引入的工具，用于生成 `0,1,2,...,N-1` 的编译期整数序列，配合包展开实现 `apply`。
- `std::tuple` 支持 `==`、`<` 等比较运算符（C++20 前需手动实现，C++20 后自动 spaceship）。
- `t2 == t3` 输出 `1`（相等）；`t2 < make_tuple(1,2,4)` 字典序比较输出 `1`（3 < 4）。

---

### 题目 7：Curiously Recurring Template Pattern (CRTP) 静态多态

```cpp
#include <iostream>
#include <chrono>

// CRTP 基类：提供静态多态的 "clone" 接口
template<typename Derived>
class Cloneable {
public:
    Derived* clone() const {
        return new Derived(static_cast<const Derived&>(*this));
    }
};

// CRTP 实现编译期多态（无虚函数开销）
template<typename Derived>
class Shape {
public:
    double area() const {
        return static_cast<const Derived*>(this)->area_impl();
    }
    void print() const {
        std::cout << "Area = " << area() << "\n";
    }
};

class Circle : public Shape<Circle>, public Cloneable<Circle> {
    double r_;
public:
    Circle(double r) : r_(r) {}
    double area_impl() const { return 3.14159 * r_ * r_; }
};

class Rectangle : public Shape<Rectangle>, public Cloneable<Rectangle> {
    double w_, h_;
public:
    Rectangle(double w, double h) : w_(w), h_(h) {}
    double area_impl() const { return w_ * h_; }
};

// CRTP Mixin：计数实例数量
template<typename Derived>
class InstanceCounter {
    static inline int count_ = 0;
public:
    InstanceCounter()  { ++count_; }
    ~InstanceCounter() { --count_; }
    static int count() { return count_; }
};

class Widget : public InstanceCounter<Widget> {
public:
    Widget() = default;
};

class Gadget : public InstanceCounter<Gadget> {};

int main() {
    Circle c(5.0);
    Rectangle r(3.0, 4.0);
    c.print();   // Area = 78.5398
    r.print();   // Area = 12
    
    auto* c2 = c.clone();
    c2->print();
    delete c2;
    
    { Widget w1, w2, w3; std::cout << Widget::count() << "\n"; }  // 3
    std::cout << Widget::count() << "\n";  // 0
    std::cout << Gadget::count() << "\n";  // 0（独立计数）
    
    // 问：若将 Shape<Circle> 指针赋给 Shape<Rectangle>* 会怎样？
    // Shape<Rectangle>* p = static_cast<Shape<Rectangle>*>(&c);  // 编译错误！类型不兼容
}
```

**解析**：
- CRTP 通过 `static_cast<Derived*>(this)` 调用子类方法，**零虚函数开销**，编译器可内联。
- 与虚函数相比：CRTP 在编译期绑定，无法运行期多态（不能用同一基类指针存储不同子类）。
- `InstanceCounter<Widget>` 和 `InstanceCounter<Gadget>` 是**不同的类**，各自有独立的 `count_` 静态成员（CRTP 的关键特性）。
- `static inline` 成员变量（C++17）可以在类内定义并初始化，无需类外定义。
- **陷阱**：`Shape<Circle>` 和 `Shape<Rectangle>` 是完全不同的类型，不能互相转换。

---

### 题目 8：模板模板参数

```cpp
#include <iostream>
#include <vector>
#include <list>
#include <deque>

// 接受容器模板作为模板参数
template<template<typename, typename> class Container, typename T>
class Stack {
    Container<T, std::allocator<T>> data_;
public:
    void push(const T& v) { data_.push_back(v); }
    
    void pop() {
        if (data_.empty()) return;
        data_.pop_back();
    }
    
    const T& top() const { return data_.back(); }
    bool empty() const { return data_.empty(); }
    std::size_t size() const { return data_.size(); }
};

// 更通用版本：可变参数模板模板
template<template<typename...> class Container, typename T>
class FlexStack {
    Container<T> data_;
public:
    void push(T v) { data_.push_back(std::move(v)); }
    T pop() {
        T v = std::move(data_.back());
        data_.pop_back();
        return v;
    }
};

// 策略模式：排序策略作为模板模板参数
template<template<typename> class SortPolicy, typename T>
class SortedContainer {
    std::vector<T> data_;
public:
    void insert(T val) {
        data_.push_back(std::move(val));
        SortPolicy<std::vector<T>>::sort(data_);
    }
    const std::vector<T>& data() const { return data_; }
};

template<typename C>
struct AscendingSort {
    static void sort(C& c) { std::sort(c.begin(), c.end()); }
};

int main() {
    Stack<std::vector, int> vs;
    Stack<std::deque, int> ds;
    
    vs.push(1); vs.push(2); vs.push(3);
    std::cout << vs.top() << "\n";  // 3
    
    FlexStack<std::list, std::string> ls;
    ls.push("hello");
    ls.push("world");
    std::cout << ls.pop() << "\n";  // world
    
    // 模板模板参数推导（C++17）
    // Stack s = Stack<std::vector, int>{};  // CTAD不适用于模板模板参数
}
```

**解析**：
- 模板模板参数语法：`template<typename, typename> class Container` 表示接受一个有两个类型参数的类模板。
- `std::vector` 实际有两个参数 `<T, Allocator>`，`std::list` 同理，所以用 `template<typename, typename>`。
- `std::set` 有三个参数 `<T, Compare, Allocator>`，不能传给只接受两参数的模板模板参数。
- C++17 放宽了模板模板参数匹配规则（P0522），允许有默认参数的模板匹配参数更少的模板模板形参。
- **实际应用**：策略模式、分配器感知容器、泛型适配器。

---

### 题目 9：编译期字符串哈希

```cpp
#include <iostream>
#include <cstdint>
#include <string_view>

// FNV-1a 哈希（编译期版本）
constexpr uint64_t FNV_PRIME  = 0x00000100000001B3ULL;
constexpr uint64_t FNV_OFFSET = 0xcbf29ce484222325ULL;

constexpr uint64_t fnv1a(std::string_view sv) {
    uint64_t hash = FNV_OFFSET;
    for (char c : sv) {
        hash ^= static_cast<uint64_t>(c);
        hash *= FNV_PRIME;
    }
    return hash;
}

// 用户定义字面量
consteval uint64_t operator"" _hash(const char* s, std::size_t n) {
    return fnv1a({s, n});
}

// 编译期 switch 替代字符串比较
void dispatch(std::string_view cmd) {
    switch (fnv1a(cmd)) {
        case "start"_hash:  std::cout << "Starting...\n"; break;
        case "stop"_hash:   std::cout << "Stopping...\n"; break;
        case "status"_hash: std::cout << "Running\n";     break;
        default:            std::cout << "Unknown: " << cmd << "\n";
    }
}

// 编译期字符串映射表
struct Entry {
    uint64_t hash;
    const char* name;
    int value;
};

constexpr Entry table[] = {
    {"alpha"_hash,   "alpha",   1},
    {"beta"_hash,    "beta",    2},
    {"gamma"_hash,   "gamma",   3},
};

constexpr int lookup(std::string_view key) {
    auto h = fnv1a(key);
    for (auto& e : table)
        if (e.hash == h) return e.value;
    return -1;
}

static_assert(lookup("alpha") == 1);
static_assert(lookup("gamma") == 3);
static_assert(lookup("delta") == -1);

int main() {
    dispatch("start");   // Starting...
    dispatch("pause");   // Unknown: pause
    
    // 哈希碰撞的概率有多大？如何检测？
    static_assert("start"_hash != "stop"_hash);  // 验证无碰撞
}
```

**解析**：
- `consteval` (C++20) 强制函数只在编译期求值，比 `constexpr` 更严格（不允许运行期调用）。
- `switch` 的 `case` 必须是整型常量表达式，`uint64_t` 哈希值满足此要求，实现"字符串switch"。
- FNV-1a 是常用的非加密哈希，碰撞概率约 1/2^64，对命令分发场景足够。
- **陷阱**：若两个字符串哈希碰撞，`switch` 会报"重复 case 值"编译错误，反而成为了碰撞检测机制。
- `constexpr` 循环（C++14 后支持）使得编译期字符串遍历成为可能。

---

### 题目 10：Concepts（C++20 概念约束）

```cpp
#include <iostream>
#include <concepts>
#include <vector>
#include <list>
#include <ranges>

// 定义自定义 concept
template<typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

template<typename T>
concept Printable = requires(T t, std::ostream& os) {
    { os << t } -> std::same_as<std::ostream&>;
};

template<typename C>
concept SizedRange = std::ranges::range<C> && requires(C c) {
    { c.size() } -> std::convertible_to<std::size_t>;
};

template<typename C>
concept RandomAccessRange = SizedRange<C> &&
    std::ranges::random_access_range<C>;

// 根据 concept 选择实现
template<RandomAccessRange C>
void process(const C& c) {
    std::cout << "[RandomAccess] size=" << c.size() 
              << " last=" << c[c.size()-1] << "\n";
}

template<SizedRange C>  // 更弱的约束（fallback）
void process(const C& c) {
    std::cout << "[SizedRange] size=" << c.size() << "\n";
}

// 约束的偏序：RandomAccessRange 比 SizedRange 更强，优先选择
// Abbreviated function template（C++20 简化语法）
auto add(Arithmetic auto a, Arithmetic auto b) {
    return a + b;
}

// requires 子句
template<typename T>
    requires Printable<T> && (!std::is_pointer_v<T>)
void safe_print(const T& val) {
    std::cout << val << "\n";
}

int main() {
    std::vector<int> v{1,2,3,4,5};
    std::list<int>   l{1,2,3};
    
    process(v);  // [RandomAccess] size=5 last=5
    process(l);  // [SizedRange] size=3
    
    std::cout << add(1, 2.5) << "\n";  // 3.5
    // add("a", "b");  // 编译错误：const char* 不满足 Arithmetic
    
    safe_print(42);        // OK
    safe_print(3.14);      // OK
    // safe_print(new int); // 编译错误：指针不满足约束
    
    // concept 可用于 if constexpr
    auto describe = []<typename T>(T) {
        if constexpr (RandomAccessRange<T>) return "random access";
        else if constexpr (SizedRange<T>)   return "sized";
        else                                 return "other";
    };
    std::cout << describe(v) << "\n";  // random access
    std::cout << describe(l) << "\n";  // sized
}
```

**解析**：
- `concept` 是 C++20 最重要的特性之一，将 SFINAE 黑魔法转化为可读的约束声明。
- 约束满足性遵循**偏序**：更特化（约束更强）的模板优先，如同偏特化规则。
- `requires` 有三种用法：`requires` 子句、`requires` 表达式、`requires requires` 嵌套（验证表达式合法性）。
- `{ expr } -> concept<...>` 同时检查表达式合法性和返回类型。
- **关键**：concept 失败是"软失败"（SFINAE 语义），不是硬错误，允许重载决议继续查找。

---

### 题目 11：模板递归与尾递归优化

```cpp
#include <iostream>
#include <type_traits>

// 编译期 GCD
template<long long A, long long B>
struct GCD {
    static constexpr long long value = GCD<B, A % B>::value;
};
template<long long A>
struct GCD<A, 0> {
    static constexpr long long value = A;
};

// 编译期有理数
template<long long Num, long long Den>
struct Rational {
    static constexpr long long g   = GCD<Num < 0 ? -Num : Num, Den < 0 ? -Den : Den>::value;
    static constexpr long long num = Num / g;
    static constexpr long long den = Den / g;
    
    template<long long N2, long long D2>
    using Add = Rational<num * D2 + N2 * den, den * D2>;
    
    template<long long N2, long long D2>
    using Mul = Rational<num * N2, den * D2>;
    
    static void print() {
        std::cout << num << "/" << den << "\n";
    }
};

// 编译期计算 e 的近似值（1 + 1/1! + 1/2! + ... + 1/N!）
template<long long N, long long Scale = 1000000>
struct EApprox {
    // 用整数近似：每项 = Scale / N!
    static constexpr long long factorial = []() {
        long long f = 1;
        for (long long i = 1; i <= N; ++i) f *= i;
        return f;
    }();
    static constexpr long long value = Scale / factorial + EApprox<N-1, Scale>::value;
};
template<long long Scale>
struct EApprox<0, Scale> {
    static constexpr long long value = Scale;  // 1 * Scale
};

int main() {
    static_assert(GCD<48, 18>::value == 6);
    static_assert(GCD<100, 75>::value == 25);
    
    using Half   = Rational<1, 2>;
    using Third  = Rational<1, 3>;
    using Result = Half::Add<1, 3>;  // 1/2 + 1/3 = 5/6
    Result::print();  // 5/6
    
    // e ≈ 2.71828...，Scale=1000000 时
    constexpr auto e_approx = EApprox<10>::value;
    std::cout << "e ≈ " << e_approx / 1000000.0 << "\n";
    
    // 问：GCD<0, 0> 会怎样？无限递归？
    // GCD<0,0>::value  // 编译错误：GCD<0,0%0> = GCD<0,0>，无限实例化
}
```

**解析**：
- `GCD<A,B>` 实现编译期辗转相除法，每次实例化减少一次递归，深度为 O(log(min(A,B)))。
- 编译期有理数加法/乘法：结果类型即计算结果，整个运算在类型系统中完成。
- `constexpr lambda`（C++17）可在 `static constexpr` 初始化中使用，实现复杂的编译期计算。
- **陷阱**：`GCD<0,0>` 触发 `0 % 0` 未定义行为（实际上在模板实例化时编译器会无限递归，直到触发深度限制）。
- 实际工程中应用：单位制计算（km/s * s = km），编译期验证量纲。

---

### 题目 12：可变参数模板递归打印

```cpp
#include <iostream>
#include <tuple>
#include <type_traits>

// 递归打印 tuple
template<std::size_t I = 0, typename... Ts>
void print_tuple(const std::tuple<Ts...>& t) {
    if constexpr (I < sizeof...(Ts)) {
        if constexpr (I > 0) std::cout << ", ";
        std::cout << std::get<I>(t);
        print_tuple<I+1>(t);
    }
}

// 编译期 map（对每个类型应用变换）
template<template<typename> class F, typename List>
struct Map;

template<template<typename> class F, typename... Ts>
struct Map<F, std::tuple<Ts...>> {
    using type = std::tuple<typename F<Ts>::type...>;
};

template<typename T> struct AddPointer { using type = T*; };
template<typename T> struct RemoveRef  { using type = std::remove_reference_t<T>; };

// 编译期 filter（保留满足谓词的类型）
template<template<typename> class Pred, typename In, typename Out = std::tuple<>>
struct Filter;

template<template<typename> class Pred, typename Out>
struct Filter<Pred, std::tuple<>, Out> { using type = Out; };

template<template<typename> class Pred, typename Head, typename... Tail, typename... Out>
struct Filter<Pred, std::tuple<Head, Tail...>, std::tuple<Out...>> {
    using next_out = std::conditional_t<Pred<Head>::value,
                                         std::tuple<Out..., Head>,
                                         std::tuple<Out...>>;
    using type = typename Filter<Pred, std::tuple<Tail...>, next_out>::type;
};

template<typename T> struct IsIntegral : std::is_integral<T> {};

int main() {
    auto t = std::make_tuple(1, 3.14, "hello", 'x', true);
    std::cout << "(";
    print_tuple(t);
    std::cout << ")\n";  // (1, 3.14, hello, x, 1)
    
    using Types  = std::tuple<int, double, char, float, long>;
    using PTypes = Map<AddPointer, Types>::type;
    // PTypes = tuple<int*, double*, char*, float*, long*>
    static_assert(std::is_same_v<std::tuple_element_t<0, PTypes>, int*>);
    
    using IntTypes = Filter<IsIntegral, Types>::type;
    // IntTypes = tuple<int, char, long>（double, float 被过滤）
    static_assert(std::is_same_v<std::tuple_element_t<0, IntTypes>, int>);
    static_assert(std::tuple_size_v<IntTypes> == 3);
    
    std::cout << "Integral types count: " << std::tuple_size_v<IntTypes> << "\n";
}
```

**解析**：
- `if constexpr` 替代模板特化实现编译期分支，避免为基础情况写额外的特化。
- 编译期 `Map`：利用包展开 `typename F<Ts>::type...` 对每个类型应用变换，一行实现 map。
- 编译期 `Filter`：累积输出类型 `Out`，逐个判断是否满足谓词 `Pred`。
- `std::conditional_t` 是编译期 if-else，选择两个类型之一。
- **性能**：所有这些操作在编译期完成，运行期零开销，但编译时间随类型数量线性增长。

---

### 题目 13：非类型模板参数（NTTP）高级用法

```cpp
#include <iostream>
#include <array>
#include <algorithm>

// C++20: 支持浮点数、字面量类作为NTTP
template<double V>  // C++20 起合法
struct FloatConst {
    static constexpr double value = V;
};

// 编译期排序的数组
template<std::size_t N>
consteval std::array<int, N> sort_array(std::array<int, N> arr) {
    std::sort(arr.begin(), arr.end());
    return arr;
}

template<std::array<int, 5> Sorted>
struct SortedTable {
    static bool contains(int v) {
        // 二分查找（数组已排序）
        auto it = std::lower_bound(Sorted.begin(), Sorted.end(), v);
        return it != Sorted.end() && *it == v;
    }
};

// 字符串作为NTTP（C++20 FixedString）
template<std::size_t N>
struct FixedStr {
    char data[N]{};
    constexpr FixedStr(const char (&s)[N]) {
        std::copy_n(s, N, data);
    }
    constexpr bool operator==(const FixedStr&) const = default;
};

template<FixedStr S>
struct StringConst {
    static constexpr auto value = S;
    static void print() { std::cout << S.data << "\n"; }
};

// 编译期位集合
template<unsigned long long Bits>
struct BitSet {
    static constexpr bool test(int pos) { return (Bits >> pos) & 1; }
    static constexpr int count() { return __builtin_popcountll(Bits); }
    
    template<int Pos>
    using Set   = BitSet<Bits | (1ULL << Pos)>;
    using Clear = BitSet<0>;
};

int main() {
    // 编译期排序后的查找表
    constexpr std::array<int,5> raw{3,1,4,1,5};
    using Table = SortedTable<sort_array(raw)>;
    
    std::cout << Table::contains(4) << "\n";  // 1
    std::cout << Table::contains(2) << "\n";  // 0
    
    // 字符串模板参数
    StringConst<"hello">::print();  // hello
    
    // 编译期位操作
    using BS = BitSet<0>::Set<3>::Set<7>::Set<15>;
    std::cout << BS::test(7) << "\n";   // 1
    std::cout << BS::count() << "\n";   // 3
    
    static_assert(BS::test(3));
    static_assert(!BS::test(5));
}
```

**解析**：
- C++20 大幅扩展 NTTP：支持浮点数、有用户定义比较的字面量类（满足 structural type 要求）。
- `consteval` 函数（C++20）强制在编译期求值，`sort_array` 在模板实参推导时执行排序。
- `std::array` 作为 NTTP 需满足：所有成员都是 structural（public、非static、非mutable）。
- `FixedStr` 作为 NTTP 实现"以字符串为模板参数"：不同字符串产生不同类型。
- **关键规则**：structural type 要求所有非静态数据成员都是 public 且满足 structural 约束（递归定义）。

---

### 题目 14：表达式模板（Expression Templates）

```cpp
#include <iostream>
#include <vector>
#include <cassert>

// 表达式模板：延迟求值避免临时对象
template<typename E>
class VecExpr {
public:
    double operator[](std::size_t i) const {
        return static_cast<const E&>(*this)[i];
    }
    std::size_t size() const {
        return static_cast<const E&>(*this).size();
    }
};

// 向量加法表达式
template<typename L, typename R>
class VecAdd : public VecExpr<VecAdd<L,R>> {
    const L& l_;
    const R& r_;
public:
    VecAdd(const L& l, const R& r) : l_(l), r_(r) {
        assert(l.size() == r.size());
    }
    double operator[](std::size_t i) const { return l_[i] + r_[i]; }
    std::size_t size() const { return l_.size(); }
};

// 标量乘法表达式
template<typename E>
class VecScale : public VecExpr<VecScale<E>> {
    double scalar_;
    const E& expr_;
public:
    VecScale(double s, const E& e) : scalar_(s), expr_(e) {}
    double operator[](std::size_t i) const { return scalar_ * expr_[i]; }
    std::size_t size() const { return expr_.size(); }
};

// 具体向量类
class Vec : public VecExpr<Vec> {
    std::vector<double> data_;
public:
    Vec(std::size_t n, double v = 0) : data_(n, v) {}
    
    template<typename E>
    Vec(const VecExpr<E>& expr) : data_(expr.size()) {
        for (std::size_t i = 0; i < data_.size(); ++i)
            data_[i] = expr[i];  // 单次循环，无临时向量
    }
    
    double& operator[](std::size_t i) { return data_[i]; }
    double  operator[](std::size_t i) const { return data_[i]; }
    std::size_t size() const { return data_.size(); }
};

// 运算符重载
template<typename L, typename R>
VecAdd<L,R> operator+(const VecExpr<L>& l, const VecExpr<R>& r) {
    return {static_cast<const L&>(l), static_cast<const R&>(r)};
}

template<typename E>
VecScale<E> operator*(double s, const VecExpr<E>& e) {
    return {s, static_cast<const E&>(e)};
}

int main() {
    Vec a(5, 1.0), b(5, 2.0), c(5, 3.0);
    
    // 无临时向量：d[i] = 2*(a[i]+b[i]) + c[i]，单次循环
    Vec d = 2.0 * (a + b) + c;
    
    for (std::size_t i = 0; i < 5; ++i)
        std::cout << d[i] << " ";  // 9 9 9 9 9
    std::cout << "\n";
    
    // 传统写法需要3次临时向量分配：
    // Vec tmp1 = a + b;       // 分配 + 循环1
    // Vec tmp2 = 2.0 * tmp1;  // 分配 + 循环2
    // Vec d2   = tmp2 + c;    // 分配 + 循环3
}
```

**解析**：
- 表达式模板将运算的**描述**与**执行**分离，`VecAdd` 只是记录操作数，不实际计算。
- 最终赋值给 `Vec` 时才触发单次循环，避免了 N 次临时分配（N=运算次数）。
- Eigen、Blaze 等线性代数库大量使用此技术，性能接近手写循环。
- **生命周期陷阱**：`VecAdd` 存储引用，若操作数被销毁（如右值临时对象），则产生悬空引用。
- **解决方案**：对右值存储副本，对左值存储引用（用 `std::conditional_t` 或引用折叠实现）。

---

### 题目 15：Policy-Based 设计

```cpp
#include <iostream>
#include <mutex>
#include <atomic>

// Policy：线程安全策略
struct SingleThreaded {
    struct Lock { Lock() {} };  // 空锁，无开销
    using Counter = int;
    static void increment(Counter& c) { ++c; }
    static void decrement(Counter& c) { --c; }
    static int  load(const Counter& c) { return c; }
};

struct MultiThreaded {
    struct Lock {
        std::mutex& m_;
        Lock(std::mutex& m) : m_(m) { m_.lock(); }
        ~Lock() { m_.unlock(); }
    };
    using Counter = std::atomic<int>;
    static void increment(Counter& c) { c.fetch_add(1, std::memory_order_relaxed); }
    static void decrement(Counter& c) { c.fetch_sub(1, std::memory_order_acq_rel); }
    static int  load(const Counter& c) { return c.load(std::memory_order_acquire); }
};

// Policy：内存分配策略
struct HeapAlloc {
    template<typename T, typename... Args>
    static T* create(Args&&... args) { return new T(std::forward<Args>(args)...); }
    template<typename T>
    static void destroy(T* p) { delete p; }
};

struct PoolAlloc {
    template<typename T, typename... Args>
    static T* create(Args&&... args) {
        // 简化版：实际从内存池分配
        return new T(std::forward<Args>(args)...);
    }
    template<typename T>
    static void destroy(T* p) { delete p; }
};

// 策略组合的智能指针
template<
    typename T,
    typename ThreadPolicy = SingleThreaded,
    typename AllocPolicy  = HeapAlloc
>
class SmartPtr {
    T* ptr_;
    typename ThreadPolicy::Counter* refcount_;
    
public:
    explicit SmartPtr(T* p = nullptr) : ptr_(p) {
        if (p) {
            refcount_ = AllocPolicy::template create<typename ThreadPolicy::Counter>(1);
        } else {
            refcount_ = nullptr;
        }
    }
    
    SmartPtr(const SmartPtr& o) : ptr_(o.ptr_), refcount_(o.refcount_) {
        if (refcount_) ThreadPolicy::increment(*refcount_);
    }
    
    ~SmartPtr() {
        if (refcount_) {
            ThreadPolicy::decrement(*refcount_);
            if (ThreadPolicy::load(*refcount_) == 0) {
                AllocPolicy::destroy(ptr_);
                AllocPolicy::destroy(refcount_);
            }
        }
    }
    
    T* operator->() const { return ptr_; }
    T& operator*()  const { return *ptr_; }
    int use_count() const { return refcount_ ? ThreadPolicy::load(*refcount_) : 0; }
};

struct Widget { int x; Widget(int v) : x(v) {} };

int main() {
    SmartPtr<Widget, SingleThreaded> p1(new Widget(42));
    auto p2 = p1;
    std::cout << p1.use_count() << "\n";   // 2
    std::cout << p1->x << "\n";            // 42
    
    SmartPtr<Widget, MultiThreaded> p3(new Widget(99));
    auto p4 = p3;
    std::cout << p3.use_count() << "\n";   // 2
}
```

**解析**：
- Policy-Based Design（Andrei Alexandrescu 提出）通过模板参数注入行为，实现零开销抽象。
- `SingleThreaded::Lock` 是空结构体，编译器直接优化掉，无运行时开销。
- `MultiThreaded` 用 `std::atomic` 保证引用计数线程安全，`mutex` 保护复合操作。
- **模板别名简化使用**：`using SafePtr<T> = SmartPtr<T, MultiThreaded, HeapAlloc>`。
- 与虚函数多态相比：Policy 在编译期绑定，无虚调用开销，但不能运行期切换策略。

---

## 第二章：内存管理与智能指针（题目 16-40）

---

### 题目 16：对象生命周期与析构顺序

```cpp
#include <iostream>

struct A {
    int id;
    A(int i) : id(i) { std::cout << "A(" << id << ") ctor\n"; }
    ~A()              { std::cout << "A(" << id << ") dtor\n"; }
};

struct B {
    A a1{1}, a2{2};
    A a3;
    B() : a3(3) { std::cout << "B ctor\n"; }
    ~B()        { std::cout << "B dtor\n"; }
};

struct C : B {
    A a4{4};
    C() { std::cout << "C ctor\n"; }
    ~C() { std::cout << "C dtor\n"; }
};

void f() {
    A arr[3] = {A(10), A(11), A(12)};  // 数组成员的构造顺序？
    std::cout << "--- in f ---\n";
}  // 析构顺序？

int main() {
    { C c; }
    std::cout << "---\n";
    f();
}
```

**解析**：输出顺序：
1. `B` 的成员按声明顺序构造：`A(1)`, `A(2)`, `A(3)`，然后 `B ctor`。
2. `C` 的成员 `A(4)`，然后 `C ctor`。
3. 析构以**逆序**：`C dtor`, `A(4)`, `B dtor`, `A(3)`, `A(2)`, `A(1)`。
4. `f()` 中数组元素按下标顺序构造：`A(10)`, `A(11)`, `A(12)`；`"--- in f ---"`；析构逆序：`A(12)`, `A(11)`, `A(10)`。
- **规则**：成员按声明顺序构造（与初始化列表顺序无关！），基类先于派生类，析构完全逆序。
- **经典bug**：初始化列表顺序与声明顺序不一致导致使用未初始化成员。

---

### 题目 17：unique_ptr 与自定义删除器

```cpp
#include <iostream>
#include <memory>
#include <cstdio>
#include <functional>

// 自定义删除器（函数指针 vs lambda vs 仿函数）
struct FileDeleter {
    void operator()(FILE* f) const {
        if (f) { std::cout << "closing file\n"; std::fclose(f); }
    }
};

// 注意：函数指针删除器使 unique_ptr 体积翻倍
using FilePtr1 = std::unique_ptr<FILE, void(*)(FILE*)>;   // sizeof = 2 * sizeof(ptr)
using FilePtr2 = std::unique_ptr<FILE, FileDeleter>;       // sizeof = 1 * sizeof(ptr)（空类优化）
using FilePtr3 = std::unique_ptr<FILE, std::function<void(FILE*)>>; // sizeof 更大！

// release() vs reset() 的区别
void demonstrate_ownership() {
    auto p1 = std::make_unique<int>(42);
    
    int* raw = p1.release();   // p1 放弃所有权，raw 需手动 delete
    std::cout << *raw << "\n"; // 42
    delete raw;                // 必须手动释放！
    
    auto p2 = std::make_unique<int>(99);
    p2.reset(new int(100));    // 先 delete 旧对象，再持有新对象
    std::cout << *p2 << "\n"; // 100
    
    p2.reset();                // delete 当前对象，变为 nullptr
    std::cout << (p2 == nullptr) << "\n";  // 1
}

// unique_ptr 数组特化
void array_unique_ptr() {
    auto arr = std::make_unique<int[]>(5);
    for (int i = 0; i < 5; ++i) arr[i] = i * i;
    // arr[0]=0, arr[1]=1, arr[2]=4, arr[3]=9, arr[4]=16
    // 析构时调用 delete[]（非 delete）
    
    // 注意：unique_ptr<T[]> 不提供 operator* 和 operator->
    // 只提供 operator[]
}

int main() {
    {
        FilePtr2 fp(std::fopen("test.txt", "w"), FileDeleter{});
        // 若 fopen 失败返回 nullptr，FileDeleter 需处理 nullptr
    }  // 自动关闭
    
    demonstrate_ownership();
    array_unique_ptr();
    
    std::cout << "sizeof FilePtr1: " << sizeof(FilePtr1) << "\n";
    std::cout << "sizeof FilePtr2: " << sizeof(FilePtr2) << "\n";
}
```

**解析**：
- 空基类优化（EBO）：`FileDeleter` 是无状态仿函数（空类），`unique_ptr` 将其压缩，不占额外空间。
- 函数指针删除器必须存储指针值，使 `unique_ptr` 体积变为两个指针大小。
- `std::function` 有类型擦除开销，是最重量级的选项。
- `release()` 只转让所有权不释放内存；`reset()` 释放当前资源并可选地接管新资源。
- `unique_ptr<T[]>` 特化用 `delete[]`，禁止 `operator*`，但支持 `operator[]`。

---

### 题目 18：shared_ptr 的陷阱

```cpp
#include <iostream>
#include <memory>

struct Node {
    int val;
    std::shared_ptr<Node> next;  // 循环引用！应用 weak_ptr
    ~Node() { std::cout << "~Node(" << val << ")\n"; }
    Node(int v) : val(v) {}
};

// 陷阱1：从原始指针创建多个 shared_ptr
void trap1() {
    int* raw = new int(42);
    std::shared_ptr<int> p1(raw);
    std::shared_ptr<int> p2(raw);  // UB！两个独立控制块，double free
    // 正确：auto p2 = p1;  // 共享控制块
}

// 陷阱2：this 指针问题
struct BadWidget {
    std::shared_ptr<BadWidget> get_self() {
        return std::shared_ptr<BadWidget>(this);  // UB！
    }
};

struct GoodWidget : std::enable_shared_from_this<GoodWidget> {
    std::shared_ptr<GoodWidget> get_self() {
        return shared_from_this();  // 正确！
    }
    static std::shared_ptr<GoodWidget> create() {
        return std::make_shared<GoodWidget>();
    }
};

// 陷阱3：make_shared vs shared_ptr(new T)
void make_shared_advantage() {
    // make_shared：一次内存分配（控制块+对象合并）
    auto p1 = std::make_shared<int>(42);
    
    // shared_ptr(new T)：两次内存分配
    std::shared_ptr<int> p2(new int(42));
    
    // 缺点：make_shared 使得对象内存在所有 weak_ptr 销毁前不释放
    std::weak_ptr<int> w = p1;
    p1.reset();  // 对象析构，但内存未释放（因为 weak 还在引用控制块）
    // 直到 w 也销毁，内存才释放
}

int main() {
    // 演示循环引用内存泄漏
    {
        auto n1 = std::make_shared<Node>(1);
        auto n2 = std::make_shared<Node>(2);
        n1->next = n2;
        n2->next = n1;  // 循环引用
    }  // n1, n2 离开作用域，但 use_count 仍为 1，不会调用析构！
    std::cout << "After scope - no destructors called!\n";
    
    // 正确：使用 weak_ptr 打破循环
    struct NodeGood {
        int val;
        std::weak_ptr<NodeGood> next;
        ~NodeGood() { std::cout << "~NodeGood(" << val << ")\n"; }
        NodeGood(int v) : val(v) {}
    };
    {
        auto n1 = std::make_shared<NodeGood>(1);
        auto n2 = std::make_shared<NodeGood>(2);
        n1->next = n2;
        n2->next = n1;
    }  // 正常析构
}
```

**解析**：
- **陷阱1**：同一裸指针创建两个 `shared_ptr` 产生两个独立控制块，各自认为自己是唯一所有者，析构时 double free，UB。
- **陷阱2**：在成员函数中用 `this` 创建 `shared_ptr`，若外部已有 `shared_ptr` 管理此对象，则产生两个控制块。`enable_shared_from_this` 通过内嵌 `weak_ptr` 解决此问题，但**前提是对象必须已被 `shared_ptr` 管理**（否则 `shared_from_this()` 抛出异常）。
- **陷阱3**：`make_shared` 的内存更紧凑但对象内存释放延迟；若有长期存活的 `weak_ptr`，可能导致大对象内存无法及时回收。

---

### 题目 19：内存对齐与 aligned_storage

```cpp
#include <iostream>
#include <type_traits>
#include <new>
#include <optional>

// 手动对齐存储（实现 optional 的底层原理）
template<typename T>
class ManualOptional {
    alignas(T) unsigned char storage_[sizeof(T)];
    bool has_value_ = false;

public:
    template<typename... Args>
    void emplace(Args&&... args) {
        if (has_value_) reset();
        new (storage_) T(std::forward<Args>(args)...);
        has_value_ = true;
    }

    void reset() {
        if (has_value_) {
            reinterpret_cast<T*>(storage_)->~T();
            has_value_ = false;
        }
    }

    T& value() {
        if (!has_value_) throw std::bad_optional_access{};
        return *std::launder(reinterpret_cast<T*>(storage_));
        //       ↑ C++17: 告诉编译器此指针"合法"，避免优化器误判
    }

    bool has_value() const { return has_value_; }
    ~ManualOptional() { reset(); }
};

// 对齐要求验证
struct alignas(16) SIMD_Vec { float data[4]; };

int main() {
    ManualOptional<std::string> opt;
    opt.emplace("hello, world");
    std::cout << opt.value() << "\n";

    opt.emplace("new value");  // 先析构旧值，再构造新值
    std::cout << opt.value() << "\n";

    opt.reset();
    // opt.value();  // throws std::bad_optional_access

    // 对齐检查
    alignas(SIMD_Vec) char buf[sizeof(SIMD_Vec)];
    std::cout << "SIMD_Vec size="    << sizeof(SIMD_Vec)    << "\n";  // 16
    std::cout << "SIMD_Vec align="   << alignof(SIMD_Vec)   << "\n";  // 16
    std::cout << "buf aligned: " << (reinterpret_cast<uintptr_t>(buf) % 16 == 0) << "\n";

    // std::launder 的必要性
    // 若无 launder，编译器可能因为"T对象从未在此地址创建"的假设而优化错误
    SIMD_Vec* p = new (buf) SIMD_Vec{{1,2,3,4}};
    SIMD_Vec* p2 = std::launder(reinterpret_cast<SIMD_Vec*>(buf));
    std::cout << p2->data[0] << "\n";  // 1
}
```

**解析**：
- `alignas(T)` 确保 `storage_` 满足 `T` 的对齐要求，`sizeof(T)` 确保空间足够。
- `placement new` 在已分配内存上构造对象，不分配内存；析构时需显式调用析构函数。
- `std::launder`（C++17）：告诉编译器通过该指针访问是合法的，防止 strict aliasing 相关的错误优化。没有 `launder`，在某些情况下编译器可能认为对象未在该地址创建，进行非法优化。
- `alignof(T)` 返回类型的对齐要求，`sizeof(T)` 包含 padding 字节。
- `std::optional` 的标准实现与 `ManualOptional` 类似，但还处理 trivially destructible 类型的优化（无需调用析构函数）。

---

### 题目 20：内存泄漏检测模式

```cpp
#include <iostream>
#include <unordered_map>
#include <string>
#include <cassert>
#include <new>

// 简单内存泄漏追踪器
class MemTracker {
    struct AllocInfo {
        std::size_t size;
        const char* file;
        int line;
    };
    std::unordered_map<void*, AllocInfo> allocs_;
    std::size_t total_allocated_ = 0;
    std::size_t peak_allocated_  = 0;
    std::size_t current_         = 0;

public:
    static MemTracker& instance() {
        static MemTracker t;
        return t;
    }

    void record_alloc(void* p, std::size_t sz, const char* f, int l) {
        allocs_[p] = {sz, f, l};
        total_allocated_ += sz;
        current_ += sz;
        peak_allocated_ = std::max(peak_allocated_, current_);
    }

    void record_free(void* p) {
        auto it = allocs_.find(p);
        if (it == allocs_.end()) {
            std::cerr << "Double free or invalid pointer: " << p << "\n";
            return;
        }
        current_ -= it->second.size;
        allocs_.erase(it);
    }

    void report() const {
        if (allocs_.empty()) {
            std::cout << "No leaks! Total=" << total_allocated_
                      << " Peak=" << peak_allocated_ << "\n";
        } else {
            std::cout << allocs_.size() << " LEAK(S):\n";
            for (auto& [p, info] : allocs_)
                std::cout << "  " << info.size << " bytes at "
                          << info.file << ":" << info.line << "\n";
        }
    }
};

// 重载 new/delete（仅用于演示，生产环境更复杂）
#ifdef TRACK_MEMORY
void* operator new(std::size_t sz, const char* f, int l) {
    void* p = std::malloc(sz);
    if (!p) throw std::bad_alloc{};
    MemTracker::instance().record_alloc(p, sz, f, l);
    return p;
}
void operator delete(void* p) noexcept {
    MemTracker::instance().record_free(p);
    std::free(p);
}
#define new new(__FILE__, __LINE__)
#endif

int main() {
    int* p1 = new int(42);
    int* p2 = new int(99);
    delete p1;
    // p2 未释放：内存泄漏

    MemTracker::instance().report();
    delete p2;  // 清理
}
```

**解析**：
- 重载 `operator new` 接受额外的 `file`/`line` 参数，配合宏 `#define new new(__FILE__, __LINE__)` 实现无侵入追踪。
- 此技术在 Valgrind 不可用的平台（如某些嵌入式系统）上非常有用。
- **陷阱**：`#define new` 会破坏 placement new（`new (addr) T()`），需要特殊处理。
- **生产级方案**：AddressSanitizer（ASan）、Valgrind、tcmalloc 的 heap checker 等工具更可靠。
- `unordered_map` 本身也会分配内存，追踪器需要用系统 `malloc` 而非追踪版 `new`，避免递归。

---

### 题目 21：移动语义的五法则

```cpp
#include <iostream>
#include <algorithm>
#include <stdexcept>

class Buffer {
    std::size_t size_;
    char*       data_;

public:
    // 构造
    explicit Buffer(std::size_t n)
        : size_(n), data_(n ? new char[n]() : nullptr) {
        std::cout << "Buffer(" << n << ")\n";
    }

    // 析构
    ~Buffer() {
        std::cout << "~Buffer(" << size_ << ")\n";
        delete[] data_;
    }

    // 拷贝构造（深拷贝）
    Buffer(const Buffer& o)
        : size_(o.size_), data_(o.size_ ? new char[o.size_] : nullptr) {
        std::copy_n(o.data_, size_, data_);
        std::cout << "Buffer copy\n";
    }

    // 移动构造（O(1)，窃取资源）
    Buffer(Buffer&& o) noexcept
        : size_(o.size_), data_(o.data_) {
        o.size_ = 0;
        o.data_ = nullptr;
        std::cout << "Buffer move\n";
    }

    // 拷贝赋值（copy-and-swap，强异常安全）
    Buffer& operator=(Buffer o) noexcept {  // 按值传参：拷贝或移动
        swap(*this, o);
        return *this;
        // o 的析构函数释放旧资源
    }

    // 注意：上面的 operator= 同时处理拷贝和移动赋值！
    // 调用 b1 = b2    → 参数 o 由拷贝构造
    // 调用 b1 = move(b2) → 参数 o 由移动构造

    friend void swap(Buffer& a, Buffer& b) noexcept {
        std::swap(a.size_, b.size_);
        std::swap(a.data_, b.data_);
    }

    std::size_t size() const { return size_; }
    char& operator[](std::size_t i) { return data_[i]; }
};

int main() {
    Buffer b1(10);
    Buffer b2(b1);          // 拷贝构造
    Buffer b3(std::move(b1)); // 移动构造，b1 变为空
    std::cout << "b1.size=" << b1.size() << "\n";  // 0

    Buffer b4(5);
    b4 = b2;               // 拷贝赋值
    b4 = std::move(b3);    // 移动赋值

    // 问：以下代码是否存在问题？
    // b3 = std::move(b3);  // 自移动赋值
    // 对于上面的 operator=(Buffer o)：
    // move(b3) 构造 o（b3 变空），swap 后 o 持有空，b3 持有原b3的资源
    // 实际上对自移动安全（因为按值传参已经移走资源）
}
```

**解析**：
- **五法则**：若定义了析构函数，通常需要同时定义拷贝构造、移动构造、拷贝赋值、移动赋值。
- `operator=(Buffer o)` 利用**值语义统一**：拷贝赋值时 `o` 被拷贝构造（可能抛异常），移动赋值时 `o` 被移动构造（不抛异常）；`swap` 之后无论如何 `o` 的析构释放旧资源。
- **自移动安全**：`b = std::move(b)` 时，`o` 被移动构造（窃取 `b` 的资源），`swap(b, o)` 后 `b` 持有原资源，`o` 持有空，析构 `o` 不出问题。
- `noexcept` 至关重要：`std::vector` 重新分配时只有 `noexcept` 的移动构造才被使用（否则退化为拷贝）。

---

### 题目 22：new/delete 的 operator 重载与 placement new

```cpp
#include <iostream>
#include <cstdlib>
#include <new>

// 重载类级别的 new/delete
class Tracked {
    static std::size_t s_alloc_count;
    static std::size_t s_bytes;

public:
    void* operator new(std::size_t sz) {
        ++s_alloc_count;
        s_bytes += sz;
        std::cout << "Tracked::new(" << sz << ")\n";
        return ::operator new(sz);  // 委托给全局 new
    }

    void operator delete(void* p, std::size_t sz) noexcept {
        --s_alloc_count;
        s_bytes -= sz;
        std::cout << "Tracked::delete(" << sz << ")\n";
        ::operator delete(p);
    }

    // 数组版本
    void* operator new[](std::size_t sz) {
        std::cout << "Tracked::new[](" << sz << ")\n";
        return ::operator new[](sz);
    }
    void operator delete[](void* p) noexcept {
        std::cout << "Tracked::delete[]\n";
        ::operator delete[](p);
    }

    static void stats() {
        std::cout << "Active: " << s_alloc_count << ", Bytes: " << s_bytes << "\n";
    }
    virtual ~Tracked() {}
};
std::size_t Tracked::s_alloc_count = 0;
std::size_t Tracked::s_bytes = 0;

// nothrow new
struct HugeObj { char data[1024*1024*1024]; };  // 1GB

int main() {
    Tracked* p = new Tracked();
    Tracked::stats();   // Active: 1, Bytes: ?
    delete p;
    Tracked::stats();   // Active: 0, Bytes: 0

    Tracked* arr = new Tracked[3];
    delete[] arr;

    // nothrow new：失败返回 nullptr 而非抛出异常
    HugeObj* big = new (std::nothrow) HugeObj;
    if (!big) std::cout << "Allocation failed (nothrow)\n";

    // placement new：在指定地址构造对象
    alignas(Tracked) char buf[sizeof(Tracked)];
    Tracked* pt = new (buf) Tracked();  // 不分配内存
    pt->~Tracked();                      // 必须显式析构！

    // 问：delete pt 会怎样？
    // delete pt;  // UB！buf 是栈内存，不能 delete
}
```

**解析**：
- 类级别的 `operator new/delete` 优先于全局版本，仅对该类（及派生类）生效。
- `operator delete(void*, size_t)` 的 `size_t` 参数由编译器传入，对派生类传入实际大小（需虚析构函数配合）。
- `new (std::nothrow)` 失败返回 `nullptr` 而非抛出 `std::bad_alloc`，用于内存受限环境。
- `placement new` 不分配内存，只在指定地址调用构造函数；对应的析构必须显式调用，**绝不能用 `delete` 删除 placement new 的对象**（除非内存本身也是堆内存）。
- `new T[N]` 分配的内存比 `N * sizeof(T)` 多（存储数组大小），所以必须用 `delete[]` 配对。

---

### 题目 23：弱指针与观察者模式

```cpp
#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>

class EventSource;

class Observer {
public:
    virtual void on_event(const std::string& event) = 0;
    virtual ~Observer() = default;
};

class EventSource {
    std::vector<std::weak_ptr<Observer>> observers_;

public:
    void subscribe(std::weak_ptr<Observer> obs) {
        observers_.push_back(std::move(obs));
    }

    void emit(const std::string& event) {
        // 通知所有存活的观察者，自动清理已销毁的
        observers_.erase(
            std::remove_if(observers_.begin(), observers_.end(),
                [&](const std::weak_ptr<Observer>& wp) {
                    if (auto sp = wp.lock()) {
                        sp->on_event(event);
                        return false;  // 保留
                    }
                    std::cout << "  [removed dead observer]\n";
                    return true;       // 移除
                }),
            observers_.end()
        );
    }
};

class Logger : public Observer {
    std::string name_;
public:
    Logger(std::string n) : name_(std::move(n)) {}
    void on_event(const std::string& e) override {
        std::cout << name_ << " received: " << e << "\n";
    }
    ~Logger() { std::cout << name_ << " destroyed\n"; }
};

int main() {
    EventSource src;
    
    auto l1 = std::make_shared<Logger>("L1");
    auto l2 = std::make_shared<Logger>("L2");
    
    src.subscribe(l1);
    src.subscribe(l2);
    
    src.emit("event1");  // L1 received, L2 received
    
    l2.reset();  // L2 销毁
    
    src.emit("event2");  // L1 received, [removed dead observer]
    src.emit("event3");  // L1 received
    
    // weak_ptr 的原子性检查
    std::weak_ptr<Logger> w = l1;
    std::cout << "expired: " << w.expired() << "\n";  // 0
    l1.reset();
    std::cout << "expired: " << w.expired() << "\n";  // 1
    
    // lock() 返回的 shared_ptr 保证期间不被销毁
    // if (auto sp = w.lock()) { ... }  // 安全的临界区
}
```

**解析**：
- `weak_ptr` 不延长对象生命周期，`lock()` 原子地检查对象是否存活并获取 `shared_ptr`（引用计数+1）。
- 观察者模式中用 `weak_ptr` 存储观察者，避免 `EventSource` 意外延长观察者生命周期（即使注册了也不影响销毁）。
- `expired()` 与 `lock()` 之间存在竞态（多线程下 `expired()==false` 后对象可能被销毁），应使用 `lock()` 的结果判断，而非 `expired()`。
- erase-remove 惯用法：`remove_if` 将"要删除"的元素移到末尾，`erase` 真正删除，一次操作。

---

### 题目 24：内存模型与缓存行

```cpp
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>

// False Sharing 演示
struct alignas(64) PaddedCounter {  // 独占一条缓存行（64字节）
    std::atomic<long long> value{0};
    char padding[64 - sizeof(std::atomic<long long>)];
};

struct UnpaddedCounter {
    std::atomic<long long> value{0};
    // 没有 padding，多个 counter 可能在同一缓存行
};

void benchmark(const char* name, auto& counters, int N) {
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::thread> threads;
    for (int i = 0; i < (int)counters.size(); ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < N; ++j)
                counters[i].value.fetch_add(1, std::memory_order_relaxed);
        });
    }
    for (auto& t : threads) t.join();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << name << ": " << ms << "ms\n";
}

int main() {
    const int N = 50'000'000;
    
    std::array<PaddedCounter, 4>   padded{};
    std::array<UnpaddedCounter, 4> unpadded{};
    
    benchmark("Padded  (no false sharing)", padded,   N);
    benchmark("Unpadded (false sharing)",   unpadded, N);
    
    // 典型结果：Padded 比 Unpadded 快 2-5 倍
    // 原因：Unpadded 时多个核心竞争同一缓存行，导致缓存一致性协议开销
    
    std::cout << "Cache line size (typical): 64 bytes\n";
    std::cout << "sizeof(PaddedCounter):   " << sizeof(PaddedCounter)   << "\n";  // 64
    std::cout << "sizeof(UnpaddedCounter): " << sizeof(UnpaddedCounter) << "\n";  // 8
}
```

**解析**：
- **False Sharing**：多个线程修改不同变量，但它们位于同一缓存行（通常64字节），导致缓存一致性协议频繁失效整个缓存行。
- `alignas(64)` 确保每个计数器独占一条缓存行，消除 false sharing。
- `padding` 数组填充剩余空间，防止编译器将多个对象紧密排列。
- `memory_order_relaxed` 是最弱的内存序，适合无需同步的计数（只关心原子性，不关心顺序）。
- **实际应用**：高性能并发数据结构（如 Java ConcurrentHashMap 的 `@Contended` 注解）。

---

### 题目 25：allocator_traits 与自定义分配器

```cpp
#include <iostream>
#include <memory>
#include <vector>
#include <list>

// 最小化分配器接口（C++11 之后只需提供 value_type + allocate + deallocate）
template<typename T>
struct LoggingAllocator {
    using value_type = T;

    LoggingAllocator() = default;
    template<typename U>
    LoggingAllocator(const LoggingAllocator<U>&) noexcept {}

    T* allocate(std::size_t n) {
        std::cout << "alloc " << n << " * " << sizeof(T) << " bytes\n";
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* p, std::size_t n) noexcept {
        std::cout << "dealloc " << n << " * " << sizeof(T) << " bytes\n";
        ::operator delete(p);
    }

    // allocator_traits 提供所有其他函数的默认实现：
    // construct(), destroy(), max_size(), select_on_container_copy_construction()
};

// 两个 LoggingAllocator<T> 实例总是相等（无状态）
template<typename T, typename U>
bool operator==(const LoggingAllocator<T>&, const LoggingAllocator<U>&) { return true; }

template<typename T, typename U>
bool operator!=(const LoggingAllocator<T>&, const LoggingAllocator<U>&) { return false; }

int main() {
    std::vector<int, LoggingAllocator<int>> v;
    v.reserve(4);        // alloc 4 * 4 bytes
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    v.push_back(4);
    v.push_back(5);      // 触发扩容：dealloc 4, alloc 8
    
    std::cout << "---\n";
    
    // list 每个节点单独分配
    std::list<int, LoggingAllocator<int>> l;
    l.push_back(1);      // alloc 1 node
    l.push_back(2);      // alloc 1 node
    l.pop_back();        // dealloc 1 node

    // propagate_on_container_copy_assignment 等 traits
    using Traits = std::allocator_traits<LoggingAllocator<int>>;
    static_assert(!Traits::propagate_on_container_copy_assignment::value);
    // 默认 false：容器拷贝赋值时不拷贝分配器
}
```

**解析**：
- C++11 起 `allocator_traits` 为分配器提供默认实现，自定义分配器只需最小接口（`value_type`, `allocate`, `deallocate`）。
- `allocator_traits::construct` 默认用 placement new 调用构造函数；`destroy` 默认调用析构函数。
- `propagate_on_container_copy_assignment`（默认 false）：控制容器赋值时是否传播分配器。若为 true 且两端分配器不等，会先用旧分配器释放，再用新分配器分配。
- **有状态分配器**（如内存池）：当 `operator==` 返回 false 时，不同实例管理不同内存池，容器赋值前需要先释放所有元素（因为新分配器无法释放旧分配器分配的内存）。
- `std::list` 使用 `rebind`（通过 `allocator_traits`）将 `Alloc<T>` 转换为 `Alloc<ListNode<T>>`。

---

## 第三章：移动语义与值类别（题目 26-45）

---

### 题目 26：值类别深度剖析

```cpp
#include <iostream>
#include <type_traits>

// 判断表达式值类别的工具
template<typename T>
void category(T&&) {
    if constexpr (std::is_lvalue_reference_v<T>)
        std::cout << "lvalue\n";
    else
        std::cout << "rvalue (xvalue or prvalue)\n";
}

int g = 42;
int& get_ref()  { return g; }
int  get_val()  { return g; }
int&& get_rref(){ return std::move(g); }

struct Widget {
    int x;
    int& member_ref() & { return x; }   // lvalue 成员函数
    int  member_val() && { return x; }  // rvalue 成员函数
};

int main() {
    int x = 1;
    category(x);           // lvalue
    category(42);          // rvalue (prvalue)
    category(std::move(x));// rvalue (xvalue)
    category(get_ref());   // lvalue
    category(get_val());   // rvalue (prvalue)
    category(get_rref());  // lvalue! （具名右值引用是左值）

    // 具名右值引用是左值！
    int&& rr = 42;
    category(rr);          // lvalue （rr 有名字）
    category(std::move(rr)); // rvalue

    // 成员访问
    Widget w{10};
    category(w.x);         // lvalue
    category(Widget{}.x);  // xvalue（通过右值访问成员）

    // 条件运算符的值类别
    int a = 1, b = 2;
    category(true ? a : b);   // lvalue（两侧均为左值引用）
    category(true ? 1 : 2);   // prvalue
    category(true ? a : 42);  // prvalue（类型不对称，decay）

    // ref-qualified member functions
    w.member_ref();            // OK：w 是 lvalue
    Widget{}.member_val();     // OK：Widget{} 是 rvalue
    // Widget{}.member_ref();  // 编译错误：& 限定不接受 rvalue
    // w.member_val();         // 编译错误：&& 限定不接受 lvalue
}
```

**解析**：
- C++ 中的值类别：**lvalue**（有名字、有地址）、**prvalue**（纯右值，如字面量、函数返回值）、**xvalue**（将亡值，如 `std::move(x)` 或通过右值访问成员）。
- `get_rref()` 返回 `int&&`，但在调用处是 `lvalue`——**具名的右值引用是左值**，这是最常见的混淆点。
- `std::move` 实际上只是 `static_cast<T&&>`，将左值转为 xvalue（右值引用），触发移动语义。
- ref-qualified member functions（`&`/`&&` 限定）允许基于对象的值类别重载成员函数。
- **实际意义**：`category` 函数的模板参数 `T` 在左值传入时被推导为 `T&`（引用折叠），右值传入时为 `T`。

---

### 题目 27：完美转发的边界情况

```cpp
#include <iostream>
#include <utility>
#include <vector>

void process(int& v)       { std::cout << "lvalue: " << v << "\n"; }
void process(const int& v) { std::cout << "const lvalue: " << v << "\n"; }
void process(int&& v)      { std::cout << "rvalue: " << v << "\n"; }

// 完美转发
template<typename T>
void wrapper(T&& arg) {
    process(std::forward<T>(arg));
}

// 多参数完美转发
template<typename... Args>
void multi_wrapper(Args&&... args) {
    (process(std::forward<Args>(args)), ...);
}

// 陷阱：initializer_list 无法推导
template<typename T>
void bad_wrapper(T&& arg) {
    // process(std::forward<T>(arg));
}

// 陷阱：位域不能绑定到右值引用
struct BitfieldStruct {
    int x : 8;
    int y : 8;
};

template<typename T>
void maybe_move(T&& val) {
    auto copy = std::forward<T>(val);  // 若 T 是 int&&，copy 是移动构造
    (void)copy;
}

int main() {
    int x = 42;
    const int cx = 99;

    wrapper(x);              // lvalue
    wrapper(cx);             // const lvalue
    wrapper(42);             // rvalue
    wrapper(std::move(x));   // rvalue

    multi_wrapper(1, x, cx); // rvalue, lvalue, const lvalue

    // initializer_list 推导失败
    // wrapper({1,2,3});  // 编译错误：无法推导 T

    // 位域不能取地址，无法绑定非const引用
    BitfieldStruct bs{10, 20};
    // wrapper(bs.x);  // 编译错误：位域不能绑定到 T&&
    int bx = bs.x;
    wrapper(bx);  // OK：先拷贝到局部变量

    // 转发到 vector emplace
    std::vector<std::pair<int,std::string>> v;
    v.emplace_back(1, "hello");  // 完美转发到 pair 构造
    v.emplace_back(std::piecewise_construct,
                   std::forward_as_tuple(2),
                   std::forward_as_tuple(3, 'a'));
}
```

**解析**：
- `std::forward<T>(arg)` 的关键：当 `T` 被推导为 `T&`（左值传入）时，`forward` 返回左值引用；当 `T` 为非引用（右值传入）时，返回右值引用。本质是有条件的 cast。
- `initializer_list` 是特殊的语言构造，无法作为模板参数推导，需要显式写 `std::initializer_list<int>`。
- 位域（bit-field）没有地址，无法绑定到任何引用（包括 `const&`，因为绑定 `const&` 需要创建临时副本，但标准不允许位域作为模板类型推导的来源）。
- `emplace_back` 对比 `push_back`：前者将参数完美转发到构造函数，避免额外的移动/拷贝。

---

### 题目 28：返回值优化（RVO/NRVO）

```cpp
#include <iostream>

struct Heavy {
    int data[1000];
    Heavy()             { std::cout << "ctor\n"; }
    Heavy(const Heavy&) { std::cout << "copy\n"; }
    Heavy(Heavy&&)      { std::cout << "move\n"; }
    ~Heavy()            { std::cout << "dtor\n"; }
};

// NRVO：Named Return Value Optimization
Heavy make_nrvo() {
    Heavy h;
    h.data[0] = 42;
    return h;  // 编译器可以在调用者的存储空间直接构造 h
}

// 不确定能 NRVO 的情况
Heavy make_conditional(bool flag) {
    Heavy a, b;
    if (flag) return a;  // 可能 NRVO
    return b;            // 多个返回路径，NRVO 可能失败
}

// RVO：Return Value Optimization（匿名临时对象，C++17 强制）
Heavy make_rvo() {
    return Heavy{};  // C++17: 强制 copy elision，无 move
}

// 阻止 NRVO 的情况
Heavy make_no_nrvo() {
    Heavy h;
    return std::move(h);  // 显式 move 阻止了 NRVO！反而可能更慢
    // 最佳实践：直接 return h; 让编译器决定
}

Heavy global_h;
Heavy make_global() {
    return global_h;  // 返回全局变量：不能 NRVO，必须拷贝
}

int main() {
    std::cout << "=== NRVO ===\n";
    Heavy h1 = make_nrvo();  // 通常只有 ctor

    std::cout << "=== RVO (C++17 mandatory) ===\n";
    Heavy h2 = make_rvo();   // C++17: 只有 ctor

    std::cout << "=== conditional ===\n";
    Heavy h3 = make_conditional(true);  // 可能 ctor + move

    std::cout << "=== std::move blocks NRVO ===\n";
    Heavy h4 = make_no_nrvo();  // ctor + move（劣于直接 return h）
}
```

**解析**：
- **RVO**（Return Value Optimization）：返回匿名临时对象时，C++17 强制省略拷贝（mandatory copy elision），直接在目标位置构造。
- **NRVO**（Named RVO）：返回具名局部变量时，编译器**可以**（非强制）省略拷贝，直接在目标位置构造。
- `return std::move(h)` **阻止 NRVO**：因为 `std::move(h)` 是 xvalue，不再是"直接返回具名局部变量"，编译器改用移动构造，反而更慢。
- 返回全局变量、引用、条件多路径等情况均可能阻止 NRVO。
- **最佳实践**：直接 `return local_var;`，让编译器优化；不要添加多余的 `std::move`。

---

### 题目 29：std::move_if_noexcept

```cpp
#include <iostream>
#include <vector>
#include <stdexcept>

struct SafeMove {
    SafeMove() { std::cout << "SafeMove ctor\n"; }
    SafeMove(const SafeMove&) { std::cout << "SafeMove copy\n"; }
    SafeMove(SafeMove&&) noexcept { std::cout << "SafeMove move\n"; }
};

struct UnsafeMove {
    UnsafeMove() { std::cout << "UnsafeMove ctor\n"; }
    UnsafeMove(const UnsafeMove&) { std::cout << "UnsafeMove copy\n"; }
    UnsafeMove(UnsafeMove&&) { std::cout << "UnsafeMove move (may throw)\n"; }
    // 注意：没有 noexcept
};

int main() {
    SafeMove   s;
    UnsafeMove u;

    // move_if_noexcept：若移动构造 noexcept 则移动，否则拷贝
    auto s2 = std::move_if_noexcept(s);  // 移动（noexcept）
    auto u2 = std::move_if_noexcept(u);  // 拷贝！（移动可能抛异常）

    // 这正是 std::vector 重新分配时的行为：
    // vector 需要保证强异常安全（要么完全成功，要么不变）
    // 若移动构造可能抛异常，重新分配时会用拷贝而非移动
    
    std::cout << "\n--- vector reallocation ---\n";
    
    std::vector<SafeMove> vs;
    vs.reserve(1);
    vs.emplace_back();
    vs.emplace_back();  // 触发重新分配 → 移动旧元素
    
    std::cout << "\n";
    std::vector<UnsafeMove> vu;
    vu.reserve(1);
    vu.emplace_back();
    vu.emplace_back();  // 触发重新分配 → 拷贝旧元素（因移动不 noexcept）
}
```

**解析**：
- `std::move_if_noexcept(x)` 等价于：若 `T` 的移动构造是 `noexcept` 的，返回 `std::move(x)`（右值引用），否则返回 `x`（左值引用，触发拷贝）。
- `std::vector::push_back` / `reserve` 在重新分配时使用此机制：移动操作必须 `noexcept` 才能利用移动优化，否则为保证强异常安全而退化为拷贝。
- **实践建议**：所有移动构造函数都应标记 `noexcept`（若确实不抛异常），这对 vector 性能至关重要。
- `std::is_nothrow_move_constructible_v<T>` 可在编译期检查此特性。

---

### 题目 30：引用折叠与万能引用

```cpp
#include <iostream>
#include <type_traits>

// 引用折叠规则：
// T& &   → T&
// T& &&  → T&
// T&& &  → T&
// T&& && → T&&

template<typename T>
void show_type(T&&) {
    using Decayed = std::decay_t<T>;
    std::cout << "T = ";
    if      (std::is_lvalue_reference_v<T>) std::cout << "lvalue ref\n";
    else if (std::is_rvalue_reference_v<T>) std::cout << "rvalue ref\n";
    else                                    std::cout << "non-ref\n";
}

// 手动实现 forward
template<typename T>
T&& my_forward(std::remove_reference_t<T>& arg) noexcept {
    return static_cast<T&&>(arg);
}

template<typename T>
T&& my_forward(std::remove_reference_t<T>&& arg) noexcept {
    static_assert(!std::is_lvalue_reference_v<T>,
                  "Cannot forward rvalue as lvalue");
    return static_cast<T&&>(arg);
}

// 陷阱：auto&& 也是万能引用
auto deduce_and_forward(auto&& x) -> decltype(auto) {
    return std::forward<decltype(x)>(x);
}

int main() {
    int x = 42;
    show_type(x);           // T = lvalue ref (T → int&)
    show_type(42);          // T = non-ref    (T → int)
    show_type(std::move(x));// T = non-ref    (T → int)

    // auto&& 推导
    auto&& r1 = x;   // r1 是 int&（万能引用绑定左值）
    auto&& r2 = 42;  // r2 是 int&&（万能引用绑定右值）
    
    static_assert(std::is_lvalue_reference_v<decltype(r1)>);  // int&
    static_assert(std::is_rvalue_reference_v<decltype(r2)>);  // int&&

    // decltype(auto) 保留值类别
    int&  lr = x;
    int&& rr = std::move(x);
    
    decltype(auto) d1 = lr;  // int& （保留左值引用）
    decltype(auto) d2 = rr;  // int&& 但 rr 是左值... 实际为 int&
    // 等同于 int& d2 = rr;
    
    // 万能引用 vs 右值引用重载的优先级
    // template<typename T> void f(T&&);  // 万能引用（更贪婪）
    // void f(int&&);                     // 右值引用重载
    // 传入 int 右值时，哪个被选中？
    // → 万能引用模板更精确匹配，被选中（T=int）
}
```

**解析**：
- **引用折叠**是 `std::forward` 能工作的基础：`T&&` 当 `T=U&` 时变为 `U& &&` 即 `U&`（保持左值），当 `T=U` 时为 `U&&`（保持右值）。
- `my_forward` 两个重载：第一个接受左值引用（当 T 为左值引用时用），第二个接受右值引用（当 T 为非引用时用）；`static_assert` 防止将右值 forward 为左值（会产生悬空引用）。
- `decltype(auto)` vs `auto`：`auto` 总是推导为值类型（剥去引用），`decltype(auto)` 保留引用和值类别。
- 万能引用（T&&在模板中）比具体的 `int&&` 重载**更贪婪**，需要注意重载冲突。

---

## 第四章：并发与原子操作（题目 31-55）

---

### 题目 31：std::mutex 的死锁场景

```cpp
#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>

std::mutex m1, m2;

// 死锁场景
void thread1_bad() {
    std::lock_guard<std::mutex> lg1(m1);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::lock_guard<std::mutex> lg2(m2);  // 等待 m2
    std::cout << "Thread1 done\n";
}

void thread2_bad() {
    std::lock_guard<std::mutex> lg2(m2);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::lock_guard<std::mutex> lg1(m1);  // 等待 m1 → 死锁！
    std::cout << "Thread2 done\n";
}

// 解决方案1：std::lock 同时加锁
void thread1_good() {
    std::unique_lock<std::mutex> lg1(m1, std::defer_lock);
    std::unique_lock<std::mutex> lg2(m2, std::defer_lock);
    std::lock(lg1, lg2);  // 原子地同时锁定两个，自动处理死锁
    std::cout << "Thread1 done\n";
}

void thread2_good() {
    std::unique_lock<std::mutex> lg1(m1, std::defer_lock);
    std::unique_lock<std::mutex> lg2(m2, std::defer_lock);
    std::lock(lg1, lg2);
    std::cout << "Thread2 done\n";
}

// 解决方案2：C++17 scoped_lock
void thread1_cpp17() {
    std::scoped_lock lock(m1, m2);  // 一行解决多锁死锁
    std::cout << "Thread1 cpp17\n";
}

// 解决方案3：全局锁顺序（按地址排序）
void lock_ordered(std::mutex& a, std::mutex& b) {
    if (&a < &b) { a.lock(); b.lock(); }
    else         { b.lock(); a.lock(); }
}

int main() {
    // 演示死锁（注释掉以运行）
    // std::thread t1(thread1_bad), t2(thread2_bad);
    // t1.join(); t2.join();  // 永远阻塞

    std::thread t3(thread1_good), t4(thread2_good);
    t3.join(); t4.join();

    std::thread t5(thread1_cpp17), t6([](){
        std::scoped_lock lock(m1, m2);
        std::cout << "Thread2 cpp17\n";
    });
    t5.join(); t6.join();
}
```

**解析**：
- 死锁四条件：互斥、持有并等待、不可抢占、循环等待。破坏任一条件即可避免死锁。
- `std::lock(lg1, lg2)` 使用无死锁算法（如 try-lock + backoff）原子地锁定多个互斥量。
- `std::scoped_lock`（C++17）是 `lock_guard` 的多锁版本，构造时调用 `std::lock`，析构时逆序解锁。
- 全局锁顺序（按地址）是最简单的无死锁策略，但要求所有代码遵守同一顺序。
- **RAII 锁管理**：始终用 `lock_guard`/`scoped_lock`/`unique_lock`，不要手动 `lock/unlock`。

---

### 题目 32：条件变量与虚假唤醒

```cpp
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>

// 线程安全队列
template<typename T>
class BlockingQueue {
    std::queue<T> q_;
    mutable std::mutex mtx_;
    std::condition_variable cv_not_empty_;
    std::condition_variable cv_not_full_;
    std::size_t max_size_;

public:
    explicit BlockingQueue(std::size_t max = 16) : max_size_(max) {}

    void push(T val) {
        std::unique_lock<std::mutex> lock(mtx_);
        // 必须用 while 循环检查条件，防止虚假唤醒
        cv_not_full_.wait(lock, [this]{ return q_.size() < max_size_; });
        q_.push(std::move(val));
        cv_not_empty_.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_not_empty_.wait(lock, [this]{ return !q_.empty(); });
        // lambda 版 wait 等价于：
        // while (q_.empty()) cv_not_empty_.wait(lock);
        T val = std::move(q_.front());
        q_.pop();
        cv_not_full_.notify_one();
        return val;
    }

    bool try_pop(T& val, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mtx_);
        if (!cv_not_empty_.wait_for(lock, timeout, [this]{ return !q_.empty(); }))
            return false;  // 超时
        val = std::move(q_.front());
        q_.pop();
        cv_not_full_.notify_one();
        return true;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return q_.size();
    }
};

int main() {
    BlockingQueue<int> bq(4);

    auto producer = std::thread([&](){
        for (int i = 0; i < 10; ++i) {
            bq.push(i);
            std::cout << "pushed: " << i << "\n";
        }
    });

    auto consumer = std::thread([&](){
        for (int i = 0; i < 10; ++i) {
            int v = bq.pop();
            std::cout << "popped: " << v << "\n";
        }
    });

    producer.join();
    consumer.join();
}
```

**解析**：
- **虚假唤醒（Spurious Wakeup）**：`wait` 可能在没有 `notify` 的情况下返回（操作系统层面），因此必须在循环（或带谓词的 `wait`）中重新检查条件。
- `cv.wait(lock, pred)` 等价于 `while (!pred()) cv.wait(lock)`，标准库保证安全处理虚假唤醒。
- `wait` 内部自动释放锁（进入等待态）并在被唤醒后重新获取锁，保证原子性。
- `notify_one` vs `notify_all`：前者唤醒一个等待线程（性能更好），后者唤醒所有（用于广播条件改变）。
- `mutable` 关键字允许在 `const` 成员函数中修改互斥量（互斥量本身不属于逻辑状态）。

---

### 题目 33：原子操作与内存序详解

```cpp
#include <atomic>
#include <thread>
#include <iostream>
#include <cassert>

// 演示各种内存序的行为
std::atomic<int> x{0}, y{0};
std::atomic<bool> ready{false};

// 场景1：release-acquire 同步
void writer() {
    x.store(42, std::memory_order_relaxed);  // 在 release 前
    y.store(99, std::memory_order_relaxed);
    ready.store(true, std::memory_order_release);  // 发布屏障
}

void reader() {
    while (!ready.load(std::memory_order_acquire));  // 获取屏障
    // 保证：看到 ready=true 的线程，也能看到 x=42, y=99
    assert(x.load(std::memory_order_relaxed) == 42);
    assert(y.load(std::memory_order_relaxed) == 99);
}

// 场景2：relaxed 不保证顺序
std::atomic<int> a{0}, b{0};
void t1() { a.store(1, std::memory_order_relaxed); b.store(1, std::memory_order_relaxed); }
void t2() {
    while (b.load(std::memory_order_relaxed) != 1);
    // 不保证能看到 a=1！因为 relaxed 不建立同步关系
    // a.load() 可能仍为 0
}

// 场景3：seq_cst（顺序一致性）是最强的内存序
std::atomic<bool> flag1{false}, flag2{false};
std::atomic<int>  victim{0};

void peterson_t1() {  // Peterson 算法（简化）
    flag1.store(true, std::memory_order_seq_cst);
    victim.store(0, std::memory_order_seq_cst);
    while (flag2.load(std::memory_order_seq_cst) &&
           victim.load(std::memory_order_seq_cst) == 0);
    // 临界区
    flag1.store(false, std::memory_order_seq_cst);
}

// 性能对比（伪代码）
// relaxed:   ~1 ns（无屏障）
// release/acquire: ~3 ns（单向屏障）
// seq_cst:   ~10 ns（全屏障，如 MFENCE）

int main() {
    std::thread tw(writer), tr(reader);
    tw.join(); tr.join();

    // compare_exchange 实现自旋锁
    std::atomic<bool> lock_flag{false};
    auto spin_lock = [&](){
        bool expected = false;
        while (!lock_flag.compare_exchange_weak(
                expected, true,
                std::memory_order_acquire,
                std::memory_order_relaxed)) {
            expected = false;  // CAS 失败后重置 expected
            // 可加 _mm_pause() 减少 CPU 功耗
        }
    };
    auto spin_unlock = [&](){
        lock_flag.store(false, std::memory_order_release);
    };
    
    spin_lock();
    std::cout << "In critical section\n";
    spin_unlock();
}
```

**解析**：
- **memory_order_relaxed**：只保证操作本身原子性，不建立 happens-before 关系，不限制重排序。适用于计数器、标志位（配合其他同步）。
- **memory_order_release/acquire**：形成"发布-获取"对，release 之前的所有写入对 acquire 之后的所有读取可见。是最常用的轻量同步原语。
- **memory_order_seq_cst**：全局顺序一致，所有 seq_cst 操作有全局唯一线性顺序。最安全但最慢（需要 MFENCE 或等价指令）。
- `compare_exchange_weak` 允许虚假失败（spurious failure）但在某些平台更高效（配合循环使用）；`compare_exchange_strong` 不允许虚假失败。
- **自旋锁的 memory_order**：加锁用 `acquire`（获取后续操作可见的值），解锁用 `release`（使临界区操作对下一个加锁者可见）。

---

### 题目 34：无锁数据结构 — Michael-Scott 队列

```cpp
#include <atomic>
#include <memory>
#include <optional>
#include <iostream>

// Michael & Scott 无锁队列（论文级实现）
template<typename T>
class MSQueue {
    struct Node {
        T data;
        std::atomic<Node*> next{nullptr};
        explicit Node(T d) : data(std::move(d)) {}
        Node() : data{} {}  // dummy 节点
    };

    std::atomic<Node*> head_;  // 指向 dummy 节点
    std::atomic<Node*> tail_;  // 指向最后一个节点

public:
    MSQueue() {
        Node* dummy = new Node();
        head_.store(dummy, std::memory_order_relaxed);
        tail_.store(dummy, std::memory_order_relaxed);
    }

    ~MSQueue() {
        Node* cur = head_.load(std::memory_order_relaxed);
        while (cur) {
            Node* next = cur->next.load(std::memory_order_relaxed);
            delete cur;
            cur = next;
        }
    }

    void enqueue(T val) {
        Node* newNode = new Node(std::move(val));
        Node* prevTail;
        while (true) {
            prevTail = tail_.load(std::memory_order_acquire);
            Node* next = prevTail->next.load(std::memory_order_acquire);
            
            if (prevTail != tail_.load(std::memory_order_relaxed))
                continue;  // tail 被其他线程修改，重试
            
            if (next == nullptr) {
                // tail 确实指向最后一个节点，尝试追加
                if (prevTail->next.compare_exchange_weak(
                        next, newNode,
                        std::memory_order_release,
                        std::memory_order_relaxed)) {
                    break;
                }
            } else {
                // tail 落后了，帮助推进（lock-free 的关键：帮助其他线程）
                tail_.compare_exchange_weak(prevTail, next,
                    std::memory_order_release, std::memory_order_relaxed);
            }
        }
        // 尝试推进 tail（可能失败，但没关系）
        tail_.compare_exchange_weak(prevTail, newNode,
            std::memory_order_release, std::memory_order_relaxed);
    }

    std::optional<T> dequeue() {
        while (true) {
            Node* h = head_.load(std::memory_order_acquire);
            Node* t = tail_.load(std::memory_order_acquire);
            Node* next = h->next.load(std::memory_order_acquire);
            
            if (h != head_.load(std::memory_order_relaxed)) continue;
            
            if (h == t) {
                if (next == nullptr) return std::nullopt;  // 队列空
                tail_.compare_exchange_weak(t, next,  // 帮助推进 tail
                    std::memory_order_release, std::memory_order_relaxed);
            } else {
                T val = next->data;
                if (head_.compare_exchange_weak(h, next,
                        std::memory_order_release, std::memory_order_relaxed)) {
                    delete h;  // 释放旧 dummy
                    return val;
                }
            }
        }
    }
};

int main() {
    MSQueue<int> q;
    q.enqueue(1); q.enqueue(2); q.enqueue(3);
    std::cout << *q.dequeue() << "\n";  // 1
    std::cout << *q.dequeue() << "\n";  // 2
    q.enqueue(4);
    std::cout << *q.dequeue() << "\n";  // 3
    std::cout << *q.dequeue() << "\n";  // 4
    std::cout << q.dequeue().has_value() << "\n";  // 0（空）
}
```

**解析**：
- MS Queue 的核心思想：tail 可能暂时滞后（指向非最后节点），每个线程在操作前先帮助推进 tail，保证进度（lock-free，非 wait-free）。
- **Lock-free vs Wait-free**：lock-free 保证系统整体进度（至少一个线程推进），wait-free 保证每个线程都能在有限步内完成。MS Queue 是 lock-free 的。
- 内存回收问题：`delete h` 在多线程下仍可能产生 ABA（回收后重分配到同一地址）。需用 hazard pointer 或 epoch-based reclamation 解决。
- 双指针（head/tail）+ 哨兵节点（dummy）是无锁队列的经典设计，使入队和出队可以并发执行（操作不同端）。

---

### 题目 35：thread_local 与线程存储

```cpp
#include <iostream>
#include <thread>
#include <vector>

// thread_local 变量每个线程独立一份
thread_local int tl_counter = 0;
thread_local std::vector<int> tl_buffer;

// 线程局部单例
class ThreadLocalCache {
    thread_local static ThreadLocalCache* instance_;
    int hits_ = 0, misses_ = 0;

    ThreadLocalCache() = default;
public:
    static ThreadLocalCache& get() {
        if (!instance_) instance_ = new ThreadLocalCache();
        return *instance_;
    }
    void record_hit()  { ++hits_; }
    void record_miss() { ++misses_; }
    void report() const {
        std::cout << "Thread " << std::this_thread::get_id()
                  << ": hits=" << hits_ << " misses=" << misses_ << "\n";
    }
};
thread_local ThreadLocalCache* ThreadLocalCache::instance_ = nullptr;

// 注意：thread_local 对象在线程结束时析构
struct TLSLifetime {
    int id;
    TLSLifetime(int i) : id(i) { std::cout << "TLS ctor " << id << "\n"; }
    ~TLSLifetime()              { std::cout << "TLS dtor " << id << "\n"; }
};

thread_local TLSLifetime tls_obj{0};  // 每线程一份，各自独立

void worker(int thread_id) {
    tls_obj.id = thread_id;  // 修改本线程的副本
    tl_counter = thread_id * 100;
    
    for (int i = 0; i < thread_id; ++i)
        tl_buffer.push_back(i);
    
    ThreadLocalCache::get().record_hit();
    ThreadLocalCache::get().record_miss();
    ThreadLocalCache::get().report();
    
    std::cout << "Thread " << thread_id 
              << ": counter=" << tl_counter 
              << " buffer.size=" << tl_buffer.size() << "\n";
}

int main() {
    std::vector<std::thread> threads;
    for (int i = 1; i <= 3; ++i)
        threads.emplace_back(worker, i);
    for (auto& t : threads) t.join();
    
    // 主线程的 tl_counter 未被修改
    std::cout << "Main: counter=" << tl_counter << "\n";  // 0
}
```

**解析**：
- `thread_local` 是 C++11 引入的存储类说明符，每个线程拥有该变量的独立副本，初始化在线程首次访问时（对非 POD 类型）或线程启动时。
- 析构顺序：`thread_local` 对象在线程函数返回后、线程析构前销毁，顺序与构造逆序。
- **thread_local 与静态成员**：`thread_local static` 成员变量在每个线程中有独立实例，但类的静态成员通常只有一份。
- **性能**：访问 `thread_local` 变量比全局变量稍慢（需通过 TLS 段访问），但比加锁共享变量快得多。
- **陷阱**：在线程池中重用线程时，`thread_local` 变量的状态会被保留（不会重置），需要手动清理。

---

### 题目 36：std::future 与 std::promise

```cpp
#include <iostream>
#include <future>
#include <thread>
#include <stdexcept>
#include <chrono>

// async 的三种启动策略
void demo_async() {
    // std::launch::async：立即在新线程执行
    auto f1 = std::async(std::launch::async, [](){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return 42;
    });

    // std::launch::deferred：延迟执行，在 get() 时才运行（当前线程）
    auto f2 = std::async(std::launch::deferred, [](){
        return 99;
    });

    std::cout << f1.get() << "\n";  // 等待异步线程
    std::cout << f2.get() << "\n";  // 在此处执行 lambda

    // 默认策略（async | deferred）：实现决定
    auto f3 = std::async([]{ return 1; });
}

// promise + future：手动传递值/异常
void demo_promise() {
    std::promise<int> p;
    std::future<int>  f = p.get_future();

    std::thread t([&p](){
        try {
            // 模拟工作
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            int result = 100;
            // if (fail) throw std::runtime_error("failed");
            p.set_value(result);
        } catch (...) {
            p.set_exception(std::current_exception());
        }
    });

    try {
        std::cout << "Result: " << f.get() << "\n";  // 100
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << "\n";
    }
    t.join();
}

// packaged_task：包装可调用对象
void demo_packaged_task() {
    std::packaged_task<int(int, int)> task([](int a, int b){
        return a + b;
    });
    
    std::future<int> f = task.get_future();
    
    std::thread t(std::move(task), 3, 4);  // 在新线程执行
    t.join();
    
    std::cout << "3 + 4 = " << f.get() << "\n";  // 7
}

// shared_future：多个线程等待同一结果
void demo_shared_future() {
    std::promise<std::string> p;
    std::shared_future<std::string> sf = p.get_future().share();

    auto t1 = std::thread([sf](){ std::cout << "T1: " << sf.get() << "\n"; });
    auto t2 = std::thread([sf](){ std::cout << "T2: " << sf.get() << "\n"; });
    
    p.set_value("broadcast message");
    t1.join(); t2.join();
}

int main() {
    demo_async();
    demo_promise();
    demo_packaged_task();
    demo_shared_future();
}
```

**解析**：
- `std::async` 的默认策略是 `async|deferred`，**不保证**在新线程执行；应显式指定 `std::launch::async` 确保异步。
- `future::get()` 只能调用一次（移动语义，获取后 future 失效）；`shared_future::get()` 可多次调用。
- `promise` 和 `future` 通过共享状态通信，`set_value`/`set_exception` 与 `get()` 之间有 release-acquire 同步。
- `packaged_task` 可以被传递到不同线程执行，与 `std::thread` 组合更灵活。
- **陷阱**：`std::async` 返回的 `future` 若不被 `get()` 或 `wait()`，析构时会阻塞（若是 `async` 策略）——因为 `future` 析构等待关联线程。

---

### 题目 37：读写锁与 shared_mutex

```cpp
#include <iostream>
#include <shared_mutex>
#include <thread>
#include <unordered_map>
#include <vector>
#include <chrono>

// 线程安全的配置存储（读多写少）
class Config {
    mutable std::shared_mutex rw_;
    std::unordered_map<std::string, std::string> data_;

public:
    // 读：共享锁（允许多个线程同时读）
    std::string get(const std::string& key) const {
        std::shared_lock<std::shared_mutex> lock(rw_);
        auto it = data_.find(key);
        return it != data_.end() ? it->second : "";
    }

    // 写：独占锁（排斥所有读和其他写）
    void set(const std::string& key, std::string val) {
        std::unique_lock<std::shared_mutex> lock(rw_);
        data_[key] = std::move(val);
    }

    // 升级锁的模式（C++14 无直接支持，需要释放读锁再获取写锁）
    bool update_if_empty(const std::string& key, const std::string& val) {
        {
            std::shared_lock<std::shared_mutex> lock(rw_);
            if (!data_[key].empty()) return false;
        }  // 释放读锁
        {
            std::unique_lock<std::shared_mutex> lock(rw_);
            // 必须再次检查（TOCTOU 问题）
            if (!data_[key].empty()) return false;
            data_[key] = val;
            return true;
        }
    }
};

int main() {
    Config cfg;
    cfg.set("host", "localhost");
    cfg.set("port", "8080");

    std::vector<std::thread> readers, writers;

    // 多个读线程并发
    for (int i = 0; i < 5; ++i) {
        readers.emplace_back([&cfg, i](){
            for (int j = 0; j < 100; ++j) {
                auto v = cfg.get("host");
                (void)v;
            }
        });
    }

    // 少量写线程
    for (int i = 0; i < 2; ++i) {
        writers.emplace_back([&cfg, i](){
            for (int j = 0; j < 10; ++j) {
                cfg.set("port", std::to_string(8080 + i * 10 + j));
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

    for (auto& t : readers) t.join();
    for (auto& t : writers) t.join();

    std::cout << "Final port: " << cfg.get("port") << "\n";
}
```

**解析**：
- `std::shared_mutex`（C++17）支持共享（读）模式和独占（写）模式，适用于读多写少场景。
- `std::shared_lock` 获取共享锁（允许并发读），`std::unique_lock` 获取独占锁（排他写）。
- **升级锁（Upgrade Lock）**：C++ 标准库不直接支持从共享锁升级到独占锁（会死锁），必须释放共享锁后重新获取独占锁，并在独占区重新检查条件（TOCTOU）。
- `mutable` 关键字：`rw_` 在 `const` 成员函数（`get`）中被锁定，需要 `mutable`，因为加锁是实现细节而非逻辑状态。
- **性能**：读写锁在争用高时不一定比普通 mutex 快（共享锁本身有原子操作开销），需要实际测量。

---

### 题目 38：std::latch 和 std::barrier（C++20）

```cpp
#include <iostream>
#include <latch>
#include <barrier>
#include <thread>
#include <vector>
#include <numeric>

// latch：一次性计数器，倒计到0后所有等待线程放行
void demo_latch() {
    constexpr int N = 5;
    std::latch start_signal(1);  // 1: 主线程控制开始
    std::latch done(N);          // N: 等待 N 个线程完成

    std::vector<std::thread> workers;
    for (int i = 0; i < N; ++i) {
        workers.emplace_back([&, i](){
            start_signal.wait();  // 等待主线程信号
            std::cout << "Worker " << i << " started\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(i * 10));
            done.count_down();  // 完成一个
        });
    }

    std::cout << "Main: releasing workers...\n";
    start_signal.count_down();  // 同时释放所有工作线程
    done.wait();                // 等待所有完成
    std::cout << "All done!\n";

    for (auto& t : workers) t.join();
}

// barrier：可重用的同步点，支持完成回调
void demo_barrier() {
    constexpr int N = 4;
    int phase = 0;

    // 回调在所有线程到达后、释放前执行（只有一个线程执行）
    auto on_completion = [&]() noexcept {
        ++phase;
        std::cout << "=== Phase " << phase << " complete ===\n";
    };

    std::barrier sync(N, on_completion);
    std::vector<int> data(N, 0);

    std::vector<std::thread> workers;
    for (int i = 0; i < N; ++i) {
        workers.emplace_back([&, i](){
            for (int round = 0; round < 3; ++round) {
                // Phase 1: 生产数据
                data[i] = (round + 1) * (i + 1);
                sync.arrive_and_wait();  // 等待所有线程完成本阶段

                // Phase 2: 消费数据（此时所有数据已就绪）
                int sum = std::accumulate(data.begin(), data.end(), 0);
                if (i == 0) std::cout << "Round " << round << " sum=" << sum << "\n";
                sync.arrive_and_wait();
            }
        });
    }
    for (auto& t : workers) t.join();
}

int main() {
    std::cout << "=== Latch ===\n";
    demo_latch();
    std::cout << "\n=== Barrier ===\n";
    demo_barrier();
}
```

**解析**：
- `std::latch`（C++20）：一次性递减计数器，`count_down()` 减 1，`wait()` 阻塞直到计数到 0，不可重置。
- `std::barrier`（C++20）：可重用的同步屏障，所有线程调用 `arrive_and_wait()` 后统一放行，可重复使用。
- **回调函数**：barrier 的完成回调在所有线程到达后、放行前被单个线程执行（无需额外同步），`noexcept` 是必须的。
- 对比 `condition_variable`：latch/barrier 是更高级的抽象，专为特定同步模式设计，代码更简洁且不易出错。
- **实际应用**：并行算法的分阶段执行、任务图的依赖管理、测试框架的并发测试。

---

### 题目 39：无锁环形缓冲区

```cpp
#include <atomic>
#include <array>
#include <optional>
#include <iostream>
#include <thread>

// 单生产者单消费者（SPSC）无锁环形缓冲区
template<typename T, std::size_t N>
class SPSCRingBuffer {
    static_assert((N & (N-1)) == 0, "N must be power of 2");
    static constexpr std::size_t MASK = N - 1;

    std::array<T, N> buf_{};
    alignas(64) std::atomic<std::size_t> head_{0};  // 生产者写
    alignas(64) std::atomic<std::size_t> tail_{0};  // 消费者读

public:
    // 生产者调用（只有一个生产者）
    bool push(const T& val) {
        std::size_t h = head_.load(std::memory_order_relaxed);
        std::size_t next_h = (h + 1) & MASK;
        if (next_h == tail_.load(std::memory_order_acquire))
            return false;  // 满

        buf_[h] = val;
        head_.store(next_h, std::memory_order_release);
        return true;
    }

    // 消费者调用（只有一个消费者）
    std::optional<T> pop() {
        std::size_t t = tail_.load(std::memory_order_relaxed);
        if (t == head_.load(std::memory_order_acquire))
            return std::nullopt;  // 空

        T val = buf_[t];
        tail_.store((t + 1) & MASK, std::memory_order_release);
        return val;
    }

    std::size_t size() const {
        auto h = head_.load(std::memory_order_acquire);
        auto t = tail_.load(std::memory_order_acquire);
        return (h - t + N) & MASK;
    }

    bool empty() const { return head_.load() == tail_.load(); }
    bool full()  const { return ((head_.load() + 1) & MASK) == tail_.load(); }
};

int main() {
    SPSCRingBuffer<int, 8> rb;

    auto producer = std::thread([&](){
        for (int i = 0; i < 20; ++i) {
            while (!rb.push(i));  // 等待空间
            std::cout << "pushed: " << i << "\n";
        }
    });

    auto consumer = std::thread([&](){
        int received = 0;
        while (received < 20) {
            if (auto v = rb.pop()) {
                std::cout << "popped: " << *v << "\n";
                ++received;
            }
        }
    });

    producer.join();
    consumer.join();
}
```

**解析**：
- SPSC 环形缓冲区是最简单的无锁数据结构：单生产者只写 `head`，单消费者只写 `tail`，各自只需读对方的值。
- **N 必须是 2 的幂**：使用位运算 `& MASK` 替代取模，避免昂贵的除法指令。
- `head_` 和 `tail_` 各自 `alignas(64)` 放在不同缓存行，避免 false sharing（生产者和消费者在不同 CPU 核上运行时分别写各自的原子量）。
- 内存序分析：`push` 中 `release` 确保 `buf_[h]=val` 对消费者可见；`pop` 中 `acquire` 确保读到最新的 `head`。
- 此实现不支持多生产者/多消费者（MPMC），MPMC 需要更复杂的 CAS 循环。

---

### 题目 40：互斥量的实现原理

```cpp
#include <atomic>
#include <thread>
#include <iostream>
#include <chrono>

// 自旋锁（spinlock）实现
class SpinLock {
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
public:
    void lock() {
        while (flag_.test_and_set(std::memory_order_acquire)) {
            // 自旋等待，减少 CPU 流水线冲突
            #if defined(__x86_64__) || defined(_M_X64)
            __builtin_ia32_pause();  // x86 PAUSE 指令
            #endif
        }
    }
    void unlock() {
        flag_.clear(std::memory_order_release);
    }
};

// 带退让的自旋锁（exponential backoff）
class BackoffSpinLock {
    std::atomic<bool> locked_{false};
public:
    void lock() {
        int backoff = 1;
        while (true) {
            bool expected = false;
            if (locked_.compare_exchange_weak(expected, true,
                    std::memory_order_acquire, std::memory_order_relaxed))
                return;
            // 退让：避免总线争用
            for (int i = 0; i < backoff; ++i)
                std::this_thread::yield();
            backoff = std::min(backoff * 2, 1024);
        }
    }
    void unlock() { locked_.store(false, std::memory_order_release); }
};

// 带 RAII 的锁守卫
template<typename Lock>
class LockGuard {
    Lock& lock_;
public:
    explicit LockGuard(Lock& l) : lock_(l) { lock_.lock(); }
    ~LockGuard() { lock_.unlock(); }
    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;
};

SpinLock sl;
int shared_counter = 0;

void increment(int times) {
    for (int i = 0; i < times; ++i) {
        LockGuard<SpinLock> guard(sl);
        ++shared_counter;
    }
}

int main() {
    std::thread t1(increment, 100000);
    std::thread t2(increment, 100000);
    t1.join(); t2.join();
    std::cout << shared_counter << "\n";  // 200000（若正确同步）

    // 问：自旋锁 vs std::mutex 的适用场景
    // 自旋锁：持锁时间极短（几纳秒到微秒），线程数少于 CPU 核数
    // std::mutex：持锁时间较长，可以让出 CPU（通过 futex 系统调用睡眠）
}
```

**解析**：
- `std::atomic_flag` 是 C++ 中唯一**保证无锁**的原子类型，`test_and_set` 原子地设置并返回旧值。
- `memory_order_acquire`（lock）和 `memory_order_release`（unlock）构成同步对，保证临界区内的操作正确可见。
- **PAUSE 指令**：x86 PAUSE 告知 CPU 当前在自旋等待，减少流水线冲撞，降低功耗，并在超线程中礼让另一逻辑核心。
- **指数退让**：每次 CAS 失败后等待时间翻倍（上限 1024），减少总线争用（bus contention）。
- **自旋锁缺点**：若持锁线程被操作系统调度出去，其他线程将空转浪费 CPU 时间。`std::mutex` 通过 futex 在内核层睡眠，避免此问题。

---

## 第五章：虚函数与多态（题目 41-60）

---

### 题目 41：虚函数表（vtable）内存布局

```cpp
#include <iostream>
#include <cstdint>

struct Base {
    virtual void f() { std::cout << "Base::f\n"; }
    virtual void g() { std::cout << "Base::g\n"; }
    virtual ~Base()  { std::cout << "~Base\n"; }
    int x = 1;
};

struct Derived : Base {
    void f() override { std::cout << "Derived::f\n"; }
    virtual void h()  { std::cout << "Derived::h\n"; }
    int y = 2;
};

// 手动调用虚函数（用于理解 vtable 结构，不要在生产代码中使用）
void call_vfunc_manually(Base* obj, int index) {
    // vptr 是对象的第一个字段（通常）
    void*** vptr = reinterpret_cast<void***>(obj);
    void** vtable = *vptr;

    using FuncPtr = void(*)(Base*);
    FuncPtr func = reinterpret_cast<FuncPtr>(vtable[index]);
    func(obj);
}

int main() {
    Base b;
    Derived d;

    // 正常虚函数调用
    Base* pb = &d;
    pb->f();  // Derived::f（动态绑定）
    pb->g();  // Base::g
    
    // 查看 vtable（平台相关，仅供理解）
    std::cout << "\nsizeof(Base)    = " << sizeof(Base)    << "\n";
    std::cout << "sizeof(Derived) = " << sizeof(Derived) << "\n";
    
    // 通常布局（GCC/Clang，64位）：
    // Base: [vptr(8)] [x(4)] [pad(4)] = 16 bytes
    // Derived: [vptr(8)] [x(4)] [pad(4)] [y(4)] [pad(4)] = 24 bytes
    
    // vtable（虚函数表）通常存储：
    // [0]: destructor (thunk)
    // [1]: f()
    // [2]: g()
    // (Derived 还有 h())
    
    // 问：以下代码有什么问题？
    Derived* pd = static_cast<Derived*>(pb);  // OK（downcast 已知安全）
    pd->h();
    
    Base* pb2 = new Derived();
    delete pb2;  // 调用 Derived::~Derived()，然后 Base::~Base()（因为虚析构）
    
    // 若 Base 没有虚析构：
    // delete pb2;  // UB！只调用 Base::~Base()，Derived 的资源泄漏
}
```

**解析**：
- 每个有虚函数的类对象包含一个**vptr**（虚函数表指针），指向该类的虚函数表（vtable）。
- vtable 是每个类一份（不是每个对象），存储虚函数地址，按声明顺序排列。
- `sizeof(Base)` = vptr(8) + x(4) + padding(4) = 16；`sizeof(Derived)` = 16 + y(4) + padding(4) = 24。
- 虚析构函数**必须声明**：若通过基类指针 `delete` 派生类对象，无虚析构则只调用基类析构，派生类资源泄漏（UB）。
- **覆盖（override）vs 隐藏（hiding）**：`override` 关键字（C++11）让编译器检查签名匹配，防止意外隐藏虚函数。

---

### 题目 42：纯虚函数与抽象类

```cpp
#include <iostream>
#include <memory>
#include <vector>

// 抽象接口
class Serializable {
public:
    virtual std::string serialize() const = 0;
    virtual void deserialize(const std::string& s) = 0;
    virtual ~Serializable() = default;
};

// 纯虚函数可以有实现（用于提供默认实现）
class Logger {
public:
    virtual void log(const std::string& msg) = 0;
    virtual ~Logger() = default;

protected:
    // 纯虚函数的实现（子类可以调用）
    void log_with_timestamp(const std::string& prefix, const std::string& msg) {
        std::cout << "[" << prefix << "] " << msg << "\n";
    }
};

// 纯虚析构（使类抽象，但允许子类正常析构）
class AbstractBase {
public:
    virtual ~AbstractBase() = 0;  // 纯虚析构
    int value = 42;
};
AbstractBase::~AbstractBase() {}  // 必须提供实现！

class Concrete : public AbstractBase {
public:
    ~Concrete() override { std::cout << "~Concrete\n"; }
};

// 接口组合
class Document : public Serializable {
    std::string content_;
public:
    Document(std::string c) : content_(std::move(c)) {}
    
    std::string serialize() const override {
        return "{\"content\":\"" + content_ + "\"}";
    }
    
    void deserialize(const std::string& s) override {
        // 简化解析
        auto start = s.find("\"content\":\"") + 11;
        auto end   = s.rfind("\"");
        content_ = s.substr(start, end - start);
    }
    
    const std::string& content() const { return content_; }
};

int main() {
    // Serializable* s = new Serializable();  // 编译错误：抽象类
    // AbstractBase* a = new AbstractBase();  // 编译错误：纯虚析构使其抽象
    
    std::unique_ptr<AbstractBase> ab = std::make_unique<Concrete>();
    // ~Concrete → ~AbstractBase（虚析构链）
    
    Document doc("Hello, World!");
    std::cout << doc.serialize() << "\n";
    
    doc.deserialize("{\"content\":\"New content\"}");
    std::cout << doc.content() << "\n";
    
    // 多态容器
    std::vector<std::unique_ptr<Serializable>> docs;
    docs.push_back(std::make_unique<Document>("Doc1"));
    docs.push_back(std::make_unique<Document>("Doc2"));
    
    for (const auto& d : docs)
        std::cout << d->serialize() << "\n";
}
```

**解析**：
- 纯虚函数（`= 0`）使类成为**抽象类**，不能直接实例化，但可以有实现（为子类提供通用功能）。
- **纯虚析构**：若只需要使类抽象（不希望实例化）但所有虚函数都有实现，可声明纯虚析构。**但必须在类外提供析构函数的定义**，因为派生类析构链会隐式调用基类析构。
- `override` 关键字（C++11）让编译器验证函数确实覆盖了基类虚函数，防止签名错误导致的意外隐藏。
- `final` 关键字（C++11）阻止进一步继承类或覆盖虚函数，编译器可借此优化去虚调用（devirtualization）。

---

### 题目 43：多重继承与菱形继承

```cpp
#include <iostream>

struct Animal {
    std::string name;
    Animal(std::string n) : name(std::move(n)) {}
    virtual void speak() const { std::cout << name << ": ...\n"; }
    virtual ~Animal() = default;
};

struct Dog : virtual Animal {  // 虚继承
    Dog(std::string n) : Animal(std::move(n)) {}
    void speak() const override { std::cout << name << ": Woof!\n"; }
};

struct Cat : virtual Animal {  // 虚继承
    Cat(std::string n) : Animal(std::move(n)) {}
    void speak() const override { std::cout << name << ": Meow!\n"; }
};

// 菱形继承（若无虚继承，会有两份 Animal）
struct Chimera : Dog, Cat {
    // 虚继承确保只有一份 Animal
    Chimera(std::string n) : Animal(std::move(n)), Dog(n), Cat(n) {
        // 虚基类必须由最派生类直接初始化
    }
    void speak() const override {
        std::cout << name << ": Woof+Meow!\n";
    }
};

// 接口多重继承（无状态）
struct Flyable {
    virtual void fly()  const = 0;
    virtual ~Flyable()  = default;
};
struct Swimmable {
    virtual void swim() const = 0;
    virtual ~Swimmable() = default;
};

struct Duck : virtual Animal, Flyable, Swimmable {
    Duck(std::string n) : Animal(std::move(n)) {}
    void fly()   const override { std::cout << name << " flies\n"; }
    void swim()  const override { std::cout << name << " swims\n"; }
    void speak() const override { std::cout << name << ": Quack!\n"; }
};

int main() {
    Chimera c("Chimera");
    c.speak();          // Woof+Meow!
    // c.name 只有一份（虚继承）
    std::cout << c.name << "\n";

    // 指针转换
    Animal* a = &c;
    a->speak();         // 动态绑定 → Woof+Meow!
    
    Dog* d = &c;
    d->speak();         // Woof+Meow!（覆盖了 Dog::speak）
    
    // 指针偏移（虚继承时，不同基类指针值不同）
    std::cout << (void*)&c << "\n";
    std::cout << (void*)(Animal*)&c << "\n";   // 可能不同
    std::cout << (void*)(Dog*)&c   << "\n";   // 可能不同

    Duck duck("Donald");
    duck.fly(); duck.swim(); duck.speak();
    
    // dynamic_cast 用于安全的多态转换
    Animal* ap = &duck;
    Flyable* fp = dynamic_cast<Flyable*>(ap);
    if (fp) fp->fly();
}
```

**解析**：
- **菱形继承**：不用虚继承时，`Chimera` 会有两份 `Animal`（一份来自 `Dog`，一份来自 `Cat`），访问 `name` 产生歧义。
- **虚继承**：`Dog : virtual Animal` 保证只有一份 `Animal` 子对象，但**最派生类**（`Chimera`）必须直接调用虚基类的构造函数。
- 虚继承的代价：每个虚继承添加一个虚基类指针（vbptr），增加对象大小；访问虚基类成员需要间接寻址。
- **接口继承（无状态多重继承）**：只继承纯虚函数的接口（如 Java/C# 的 interface），无菱形问题，是最常用的多重继承模式。
- `dynamic_cast` 可在多重继承层次中安全地侧转（cross-cast）：`Animal*` → `Flyable*`。

---

### 题目 44：协变返回类型

```cpp
#include <iostream>
#include <memory>

struct Base {
    virtual Base* clone() const {
        return new Base(*this);
    }
    virtual void identify() const {
        std::cout << "Base\n";
    }
    virtual ~Base() = default;
};

struct Derived : Base {
    // 协变返回类型：返回 Derived* 而非 Base*（合法！）
    Derived* clone() const override {
        return new Derived(*this);
    }
    void identify() const override {
        std::cout << "Derived\n";
    }
    int extra = 42;
};

// CRTP 实现类型安全的 clone
template<typename Derived_>
struct Cloneable : Base {
    Base* clone() const override {
        return new Derived_(*static_cast<const Derived_*>(this));
    }
};

struct Widget : Cloneable<Widget> {
    int value;
    Widget(int v) : value(v) {}
    void identify() const override { std::cout << "Widget(" << value << ")\n"; }
};

// 协变返回类型的智能指针版本（需要包装）
struct BaseFactory {
    virtual std::unique_ptr<Base> create() const {
        return std::make_unique<Base>();
    }
    virtual ~BaseFactory() = default;
};

struct DerivedFactory : BaseFactory {
    // 不能直接协变 unique_ptr<Derived>，因为不是原始指针或引用
    std::unique_ptr<Base> create() const override {
        return std::make_unique<Derived>();
    }
    // 额外提供精确类型版本
    std::unique_ptr<Derived> create_derived() const {
        return std::make_unique<Derived>();
    }
};

int main() {
    Derived d;
    Derived* d2 = d.clone();  // 协变：直接得到 Derived*
    d2->identify();
    d2->extra = 99;  // 无需 static_cast
    delete d2;
    
    Base* b = &d;
    Base* b2 = b->clone();    // 通过 Base* 调用，返回 Base*（但实际是 Derived*）
    b2->identify();            // Derived
    delete b2;
    
    // CRTP clone
    Widget w(10);
    auto* w2 = static_cast<Widget*>(w.clone());
    w2->identify();  // Widget(10)
    delete w2;
}
```

**解析**：
- **协变返回类型**（Covariant Return Type）：覆盖虚函数时，返回类型可以是基类返回类型的派生类（原始指针或引用）。这是 C++ 对 Liskov 替换原则的语法支持。
- **限制**：协变只适用于原始指针和引用，`std::unique_ptr<Derived>` 不能协变（因为 `unique_ptr<Derived>` 不是 `unique_ptr<Base>` 的子类，即使 `Derived*` 是 `Base*` 的子类）。
- **智能指针协变的解决方案**：返回 `unique_ptr<Base>` 但内部存 `Derived`，或提供两个函数（基类接口 + 派生类精确版本）。
- CRTP `Cloneable` 自动为每个派生类提供正确的 `clone` 实现，避免重复代码。

---

### 题目 45：虚函数的性能影响

```cpp
#include <iostream>
#include <chrono>
#include <vector>
#include <memory>
#include <numeric>

// 场景：对大量小对象调用虚函数的性能对比

struct ICompute {
    virtual double compute(double x) const = 0;
    virtual ~ICompute() = default;
};

struct Square   : ICompute { double compute(double x) const override { return x * x; } };
struct Cube     : ICompute { double compute(double x) const override { return x * x * x; } };
struct Sqrt     : ICompute { double compute(double x) const override { return std::sqrt(x); } };

// 非虚版本（用于对比）
template<typename F>
double compute_static(F&& f, double x) { return f(x); }

const int N = 10'000'000;

void bench_virtual() {
    std::vector<std::unique_ptr<ICompute>> ops;
    for (int i = 0; i < N; ++i) {
        switch (i % 3) {
            case 0: ops.push_back(std::make_unique<Square>()); break;
            case 1: ops.push_back(std::make_unique<Cube>());   break;
            case 2: ops.push_back(std::make_unique<Sqrt>());   break;
        }
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    double sum = 0;
    for (int i = 0; i < N; ++i)
        sum += ops[i]->compute(i);  // 虚函数调用（间接跳转 + 可能缓存未命中）
    auto end = std::chrono::high_resolution_clock::now();
    
    std::cout << "Virtual: " 
              << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count()
              << "ms, sum=" << sum << "\n";
}

void bench_devirtualized() {
    // 分离存储（提升缓存局部性）
    std::vector<Square> squares;
    std::vector<Cube>   cubes;
    std::vector<int>    indices(N);
    
    // 同类型批量处理（编译器可内联、矢量化）
    auto start = std::chrono::high_resolution_clock::now();
    double sum = 0;
    for (int i = 0; i < N; ++i) {
        switch (i % 3) {
            case 0: sum += Square{}.compute(i); break;
            case 1: sum += Cube{}.compute(i);   break;
            case 2: sum += Sqrt{}.compute(i);   break;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Devirtualized: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count()
              << "ms, sum=" << sum << "\n";
}

int main() {
    bench_virtual();
    bench_devirtualized();
    // 典型结果：devirtualized 快 2-5 倍
    // 原因：
    // 1. 虚调用需要额外的内存访问（vptr → vtable → 函数地址）
    // 2. 间接跳转阻止编译器内联
    // 3. 分散的对象内存导致 cache miss
}
```

**解析**：
- 虚函数调用的开销：1) 通过 vptr 读 vtable（可能 cache miss），2) 读函数地址（可能 cache miss），3) 间接跳转（分支预测困难），4) 阻止内联优化。
- **数据局部性**：将同类型对象连续存储（Data-Oriented Design），提升缓存命中率，是游戏引擎优化的核心技术（ECS 架构）。
- **去虚化（Devirtualization）**：编译器可在知道确切类型时去掉虚调用（如 `final` 类型），或通过 PGO 推断热路径类型。
- **std::variant + std::visit** 是现代 C++ 替代多态的方案，可实现完全去虚化：`std::visit([](auto& op){ return op.compute(x); }, variant_obj)`。
- 当性能关键路径有大量多态调用时，考虑：批量处理同类型、模板静态多态、variant、函数指针表等替代方案。

---

## 第六章：类型系统与转换（题目 46-60）

---

### 题目 46：四种 cast 的区别与陷阱

```cpp
#include <iostream>
#include <typeinfo>

struct Base { virtual ~Base() {} int b = 1; };
struct Derived : Base { int d = 2; };
struct Other : Base { int o = 3; };

void demonstrate_casts() {
    Derived dd;
    Base* bp = &dd;

    // static_cast: 编译期检查，无运行时开销
    Derived* dp = static_cast<Derived*>(bp);  // downcast（程序员保证安全）
    std::cout << dp->d << "\n";  // 2

    // dynamic_cast: 运行时 RTTI 检查（需要虚函数）
    Other* op = dynamic_cast<Other*>(bp);  // 失败 → nullptr
    if (!op) std::cout << "dynamic_cast failed\n";

    Derived* dp2 = dynamic_cast<Derived*>(bp);  // 成功
    std::cout << dp2->d << "\n";  // 2

    // reinterpret_cast: 位模式重解释，几乎不检查任何东西
    uintptr_t addr = reinterpret_cast<uintptr_t>(bp);
    Base* bp2 = reinterpret_cast<Base*>(addr);  // 合法（原类型）
    
    // 危险：将 Base* 重解释为 Other*（UB，严格别名违反）
    // Other* op2 = reinterpret_cast<Other*>(bp);  // 技术上 UB

    // const_cast: 添加或移除 const
    const int cx = 42;
    int& rx = const_cast<int&>(cx);
    rx = 99;  // UB！cx 是 const 对象，修改它是未定义行为
    // 合法用法：移除指向非 const 对象的 const 指针的 const
    int x = 10;
    const int* cp = &x;
    int* p = const_cast<int*>(cp);  // 合法：x 本身非 const
    *p = 20;  // OK
    std::cout << x << "\n";  // 20

    // C 风格 cast（危险！按顺序尝试多种 cast）
    // (Type)expr 等价于尝试：const_cast → static_cast → reinterpret_cast
    int i = 42;
    double d = (double)i;  // 相当于 static_cast<double>(i)
    
    // 但这是危险的：
    const int* cp2 = &x;
    int* p2 = (int*)cp2;  // 相当于 const_cast，隐藏了危险操作
}

int main() {
    demonstrate_casts();
    
    // dynamic_cast 引用版本：失败时抛出 std::bad_cast
    Derived d;
    Base& br = d;
    try {
        Other& or_ = dynamic_cast<Other&>(br);  // 抛出
    } catch (const std::bad_cast& e) {
        std::cout << "bad_cast: " << e.what() << "\n";
    }
}
```

**解析**：
- `static_cast`：编译期检查继承关系，无运行时开销，但 downcast 不安全（程序员需保证类型正确）。
- `dynamic_cast`：运行时 RTTI 检查，指针版失败返回 `nullptr`，引用版失败抛 `bad_cast`。需要虚函数（多态类型）。
- `reinterpret_cast`：位模式重解释，绕过类型系统。将不同类型指针互转通常违反严格别名规则（UB），仅在特定场景（如序列化、硬件寄存器）合法。
- `const_cast`：唯一能去掉 `const`/`volatile` 的 cast。若原对象本身是 `const`，通过 `const_cast` 修改是 UB。
- **规则**：能用 `static_cast` 就不用 `reinterpret_cast`；能避免 `const_cast` 就避免；C 风格 cast 混淆语义，应禁止。

---

### 题目 47：std::variant 与类型安全的联合体

```cpp
#include <iostream>
#include <variant>
#include <string>
#include <vector>

// 类型安全的 union 替代
using Value = std::variant<int, double, std::string, std::vector<int>>;

// Visitor 模式
struct Printer {
    void operator()(int i)                    { std::cout << "int: "    << i << "\n"; }
    void operator()(double d)                 { std::cout << "double: " << d << "\n"; }
    void operator()(const std::string& s)     { std::cout << "string: " << s << "\n"; }
    void operator()(const std::vector<int>& v){ 
        std::cout << "vector[" << v.size() << "]: ";
        for (int x : v) std::cout << x << " ";
        std::cout << "\n";
    }
};

// 重载 lambda（C++17 pattern）
template<typename... Fs>
struct overload : Fs... { using Fs::operator()...; };
template<typename... Fs> overload(Fs...) -> overload<Fs...>;

// 递归 variant（如 JSON）
struct Null {};
struct JsonValue;
using JsonArray  = std::vector<JsonValue>;
using JsonObject = std::vector<std::pair<std::string, JsonValue>>;

struct JsonValue {
    std::variant<Null, bool, int, double, std::string, JsonArray, JsonObject> value;
    
    JsonValue() : value(Null{}) {}
    template<typename T>
    JsonValue(T&& v) : value(std::forward<T>(v)) {}
};

int main() {
    Value v = 42;
    std::visit(Printer{}, v);    // int: 42

    v = 3.14;
    std::visit(Printer{}, v);    // double: 3.14

    v = std::string("hello");
    std::visit(Printer{}, v);    // string: hello

    v = std::vector<int>{1,2,3};
    std::visit(Printer{}, v);    // vector[3]: 1 2 3

    // overload idiom：内联 visitor
    std::visit(overload{
        [](int i)         { std::cout << "got int " << i << "\n"; },
        [](double d)      { std::cout << "got double " << d << "\n"; },
        [](const auto& x) { std::cout << "got other\n"; }  // catch-all
    }, v);

    // get 和 holds_alternative
    if (std::holds_alternative<std::vector<int>>(v)) {
        auto& vec = std::get<std::vector<int>>(v);
        vec.push_back(4);
    }
    
    // std::get 失败时抛 std::bad_variant_access
    try {
        std::get<int>(v);  // v 是 vector，抛异常
    } catch (const std::bad_variant_access&) {
        std::cout << "bad variant access\n";
    }

    // valueless_by_exception 状态
    // 若赋值过程中新类型构造抛异常，variant 进入 valueless 状态
    std::cout << "index: " << v.index() << "\n";  // 3（vector<int> 是第4个类型）
}
```

**解析**：
- `std::variant` 是**类型安全的 union**，只能持有一种类型，编译器跟踪当前类型，访问错误类型时抛异常或 UB（`get_if` 返回 nullptr）。
- `std::visit` 实现访问者模式，要求提供所有类型的处理，**编译器强制穷举**（未处理类型是编译错误）。
- `overload` 模式（聚合继承多个 lambda）是 C++17 的惯用法，语法简洁。
- **vs union**：union 不跟踪当前类型、不调用析构函数（非平凡类型 UB）、访问不安全。
- `valueless_by_exception`：variant 的异常安全边界情况，赋值失败时变为无效状态（`index()` 返回 `variant_npos`），通常只在移动构造抛异常时出现。

---

### 题目 48：std::any 的类型擦除机制

```cpp
#include <iostream>
#include <any>
#include <typeinfo>
#include <vector>

// 手动实现简化版 any（理解机制）
class MyAny {
    struct Base {
        virtual const std::type_info& type() const = 0;
        virtual Base* clone() const = 0;
        virtual ~Base() = default;
    };

    template<typename T>
    struct Holder : Base {
        T value;
        Holder(T v) : value(std::move(v)) {}
        const std::type_info& type()  const override { return typeid(T); }
        Base* clone() const override { return new Holder<T>(value); }
    };

    // SOO: 小对象优化（对象 <= 8 字节时直接存储）
    Base* ptr_ = nullptr;

public:
    MyAny() = default;
    
    template<typename T>
    MyAny(T val) : ptr_(new Holder<T>(std::move(val))) {}
    
    MyAny(const MyAny& o) : ptr_(o.ptr_ ? o.ptr_->clone() : nullptr) {}
    MyAny(MyAny&& o) noexcept : ptr_(o.ptr_) { o.ptr_ = nullptr; }
    ~MyAny() { delete ptr_; }
    
    bool has_value() const { return ptr_ != nullptr; }
    
    template<typename T>
    T& get() {
        if (!ptr_ || ptr_->type() != typeid(T))
            throw std::bad_any_cast{};
        return static_cast<Holder<T>*>(ptr_)->value;
    }
    
    void reset() { delete ptr_; ptr_ = nullptr; }
};

int main() {
    // std::any 使用
    std::any a = 42;
    std::cout << std::any_cast<int>(a) << "\n";  // 42

    a = std::string("hello");
    std::cout << std::any_cast<std::string>(a) << "\n";  // hello

    a = std::vector<int>{1,2,3};
    auto& v = std::any_cast<std::vector<int>&>(a);
    v.push_back(4);
    std::cout << std::any_cast<std::vector<int>>(a).size() << "\n";  // 4

    // 类型不匹配时抛出 bad_any_cast
    try {
        std::any_cast<int>(a);  // 当前是 vector<int>
    } catch (const std::bad_any_cast& e) {
        std::cout << "bad_any_cast: " << e.what() << "\n";
    }

    // type() 获取当前类型信息
    std::cout << a.type().name() << "\n";  // 实现定义（通常是 mangled name）

    // 指针版 any_cast（失败返回 nullptr，不抛异常）
    auto* p = std::any_cast<std::vector<int>>(&a);
    if (p) std::cout << "got vector of size " << p->size() << "\n";

    // MyAny 演示
    MyAny ma = 3.14;
    std::cout << ma.get<double>() << "\n";  // 3.14
}
```

**解析**：
- `std::any` 通过**类型擦除**存储任意类型，内部有虚函数表（或等价的函数指针）记录类型信息和操作（拷贝、析构等）。
- **小对象优化（SOO）**：`std::any` 通常有内置缓冲区（约 24-32 字节），小对象直接存在其中，大对象在堆上分配。
- `std::any_cast<T&>(a)` 返回引用，避免拷贝；`std::any_cast<T>(&a)` 返回指针，失败返回 `nullptr`（更高效的失败路径）。
- **vs void\***：`void*` 完全放弃类型安全；`any` 在运行时保持类型信息，类型不匹配时抛异常。
- **vs variant**：`variant` 是封闭类型集合（编译期确定）；`any` 是开放的（可存任意类型），但 `any` 无法对所有类型执行 `visit`。

---

### 题目 49：decltype 与 auto 类型推导陷阱

```cpp
#include <iostream>
#include <type_traits>
#include <vector>

// decltype 规则
int x = 5;
int& rx = x;
const int cx = 10;

// auto 推导规则（类似模板参数推导）
// 1. 去掉 reference
// 2. 去掉 const（对于拷贝）

void auto_traps() {
    auto a = x;       // int（去掉引用，拷贝）
    auto b = rx;      // int（去掉引用，拷贝）
    auto c = cx;      // int（去掉 const，拷贝）
    auto& d = rx;     // int&（保留引用）
    auto& e = cx;     // const int&（保留 const，因为绑定引用）
    const auto& f = x;// const int&
    auto&& g = x;     // int&（万能引用 + 左值 → 左值引用）
    auto&& h = 42;    // int&&（万能引用 + 右值 → 右值引用）

    static_assert(std::is_same_v<decltype(a), int>);
    static_assert(std::is_same_v<decltype(d), int&>);
    static_assert(std::is_same_v<decltype(g), int&>);
    static_assert(std::is_same_v<decltype(h), int&&>);

    // auto 与数组/函数的衰变
    int arr[3] = {1,2,3};
    auto ap = arr;    // int*（数组衰变为指针）
    auto& ar = arr;   // int(&)[3]（保留数组类型）

    static_assert(std::is_same_v<decltype(ap), int*>);
    static_assert(std::is_same_v<decltype(ar), int(&)[3]>);
}

// decltype 规则（更精确）
// decltype(变量名) → 变量声明的类型
// decltype(表达式) → 若表达式是 lvalue：T&；若是 xvalue：T&&；若是 prvalue：T

void decltype_traps() {
    int y = 5;
    decltype(y)    a = y;  // int（变量名）
    decltype((y))  b = y;  // int&！（括号使其成为表达式，y 是 lvalue）
    
    static_assert(std::is_same_v<decltype(y),   int>);
    static_assert(std::is_same_v<decltype((y)),  int&>);
    
    // decltype(auto) 的用途：保留函数返回值的精确类型
    auto get = [&]() -> decltype(auto) { return y; };   // 返回 int（拷贝）
    auto get_ref = [&]() -> decltype(auto) { return (y); }; // 返回 int&（引用！）
    
    static_assert(std::is_same_v<decltype(get()),     int>);
    static_assert(std::is_same_v<decltype(get_ref()), int&>);
}

int main() {
    auto_traps();
    decltype_traps();
    
    // 常见陷阱：auto 不保留引用
    std::vector<int> v{1,2,3};
    auto elem = v[0];    // int（拷贝！修改 elem 不影响 v）
    elem = 99;
    std::cout << v[0] << "\n";  // 1（未被修改）
    
    auto& ref = v[0];   // int&（引用，修改会影响 v）
    ref = 99;
    std::cout << v[0] << "\n";  // 99
}
```

**解析**：
- `auto` 推导规则与模板参数推导相同：去掉引用和顶层 `const`（对于拷贝语义）；保留底层 `const`（对于引用语义）。
- `decltype(expr)` vs `decltype((expr))`：括号将标识符转为表达式，lvalue 表达式推导为 `T&`。这是 C++ 中最著名的陷阱之一。
- `decltype(auto)` 完全保留类型（包括引用），比 `auto` 精确，用于"透明"转发函数的返回值。
- **实际陷阱**：`auto elem = v[0]` 是拷贝，修改 `elem` 不影响容器。在基于范围的 for 循环中应用 `auto&` 避免不必要拷贝。

---

### 题目 50：类型特征与 is_detected

```cpp
#include <iostream>
#include <type_traits>
#include <string>
#include <vector>

// C++17 检测习语（Detection Idiom）
template<typename, template<typename> class, typename = void>
struct is_detected : std::false_type {};

template<typename T, template<typename> class Op>
struct is_detected<T, Op, std::void_t<Op<T>>> : std::true_type {};

template<typename T, template<typename> class Op>
constexpr bool is_detected_v = is_detected<T, Op>::value;

// 定义检测操作
template<typename T> using has_begin_t      = decltype(std::declval<T>().begin());
template<typename T> using has_size_t       = decltype(std::declval<T>().size());
template<typename T> using has_toString_t   = decltype(std::declval<T>().toString());
template<typename T> using has_serialize_t  = decltype(std::declval<T>().serialize());
template<typename T> using is_addable_t     = decltype(std::declval<T>() + std::declval<T>());
template<typename T> using has_ostream_t    = decltype(std::declval<std::ostream&>() << std::declval<T>());

// 根据检测结果选择行为
template<typename T>
std::string to_string(const T& val) {
    if constexpr (is_detected_v<T, has_toString_t>) {
        return val.toString();
    } else if constexpr (is_detected_v<T, has_serialize_t>) {
        return val.serialize();
    } else if constexpr (is_detected_v<T, has_ostream_t>) {
        std::ostringstream oss;
        oss << val;
        return oss.str();
    } else {
        return "[unprintable]";
    }
}

struct WithToString {
    std::string toString() const { return "WithToString"; }
};

struct WithSerialize {
    std::string serialize() const { return "Serialized"; }
};

struct Unprintable { int x; };

int main() {
    // 检测
    static_assert(is_detected_v<std::vector<int>, has_begin_t>);
    static_assert(is_detected_v<std::vector<int>, has_size_t>);
    static_assert(!is_detected_v<int,             has_begin_t>);
    static_assert(is_detected_v<int,              is_addable_t>);
    static_assert(is_detected_v<std::string,      has_ostream_t>);
    static_assert(!is_detected_v<Unprintable,     has_ostream_t>);
    
    std::cout << to_string(WithToString{})   << "\n";  // WithToString
    std::cout << to_string(WithSerialize{})  << "\n";  // Serialized
    std::cout << to_string(42)               << "\n";  // 42
    std::cout << to_string(Unprintable{99})  << "\n";  // [unprintable]
    
    // C++20 requires 表达式更简洁
    auto has_size = []<typename T>(T) {
        return requires(T t) { t.size(); };
    };
    std::cout << has_size(std::vector<int>{}) << "\n";  // 1
    std::cout << has_size(42)                 << "\n";  // 0
}
```

**解析**：
- **检测习语（Detection Idiom）**：用 `void_t` + 偏特化检测类型是否具有某个特征（成员函数、运算符等）。
- `std::void_t<Op<T>>` 在 `Op<T>` 合法时为 `void`，非法时 SFINAE 失败，选择 `false_type` 特化。
- C++20 的 `requires` 表达式替代检测习语，语法更直观，但 C++17 兼容性更广。
- `if constexpr` 与检测习语结合实现**编译期策略选择**：根据类型能力自动选择最佳实现。
- **实际应用**：序列化库（根据类型选择序列化策略）、日志库（根据是否可输出决定格式）、测试框架。

