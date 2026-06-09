# C++ 硬核笔试题 第 51-200 道（含代码 + 解析）

> 涵盖：STL 内部机制、并发与无锁、协程、UB 陷阱、类型擦除、内存模型、CRTP 进阶、constexpr/consteval、lambda 深度、异常安全、设计模式实战、算法综合压轴

---

## 第七章：STL 内部机制（题目 51-75）

---

### 题目 51：vector 扩容的精确控制与迭代器失效

```cpp
#include <iostream>
#include <vector>
#include <cassert>
#include <string>

struct Canary {
    int id;
    static int alive;
    Canary(int i) : id(i) { ++alive; }
    Canary(const Canary& o) : id(o.id) { ++alive; }
    Canary(Canary&& o) noexcept : id(o.id) { ++alive; o.id = -1; }
    ~Canary() { --alive; }
};
int Canary::alive = 0;

int main() {
    std::vector<Canary> v;
    v.reserve(4);
    v.emplace_back(1);
    v.emplace_back(2);
    v.emplace_back(3);

    // 保存原始指针（realloc 后失效）
    Canary* raw = v.data();
    int* id0    = &v[0].id;

    std::cout << "capacity before: " << v.capacity() << "\n";  // 4
    std::cout << "alive: "           << Canary::alive << "\n";  // 3

    // 触发扩容
    v.emplace_back(4);
    v.emplace_back(5);  // capacity=4 不够，reallocation

    std::cout << "capacity after: " << v.capacity() << "\n";  // >=5
    std::cout << "alive: "          << Canary::alive << "\n";  // 5

    // 指针失效检测
    bool ptr_moved = (v.data() != raw);
    std::cout << "data pointer changed: " << ptr_moved << "\n";  // 1（大概率）
    // *id0 = 99;  // UB！原内存已被 free

    // erase-remove idiom
    std::vector<int> nums{1,2,3,2,4,2,5};
    nums.erase(std::remove(nums.begin(), nums.end(), 2), nums.end());
    for (int x : nums) std::cout << x << " ";  // 1 3 4 5
    std::cout << "\n";

    // erase 返回的迭代器继续遍历（安全写法）
    std::vector<int> data{1,2,3,4,5,6};
    for (auto it = data.begin(); it != data.end(); ) {
        if (*it % 2 == 0)
            it = data.erase(it);   // 返回下一个有效迭代器
        else
            ++it;
    }
    for (int x : data) std::cout << x << " ";  // 1 3 5
    std::cout << "\n";
}
```

**解析**：
- `reserve(N)` 只改变 `capacity`，`size` 不变；`resize(N)` 改变 `size` 并值初始化新元素。
- reallocation 后，**所有**指针、引用、迭代器均失效——包括 `data()` 返回的裸指针。
- `vector` 扩容策略通常是 2x（GCC/Clang）或 1.5x（MSVC），保证 `push_back` 均摊 O(1)。
- `erase-remove` 是删除匹配元素的惯用法：`remove` 只是将不删除的元素前移，需配合 `erase` 真正缩短容器。
- `erase` 返回指向被删元素下一位置的新迭代器；循环体内删除时必须使用此返回值，而非 `++it`。

---

### 题目 52：std::deque 分块存储与 O(1) 两端操作

```cpp
#include <iostream>
#include <deque>
#include <vector>
#include <chrono>

void bench(const char* label, auto fn) {
    auto t0 = std::chrono::high_resolution_clock::now();
    fn();
    auto t1 = std::chrono::high_resolution_clock::now();
    std::cout << label << " "
              << std::chrono::duration_cast<std::chrono::microseconds>(t1-t0).count()
              << "us\n";
}

int main() {
    constexpr int N = 100'000;

    // vector 头插 O(n) vs deque 头插 O(1)
    bench("vector push_front N times", [&]{
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i)
            v.insert(v.begin(), i);   // O(n) 每次
    });

    bench("deque  push_front N times", [&]{
        std::deque<int> d;
        for (int i = 0; i < N; ++i)
            d.push_front(i);          // O(1) 均摊
    });

    // 迭代器类型差异
    std::deque<int> d{1,2,3,4,5};
    auto it = d.begin() + 2;          // random access OK
    std::cout << *it << "\n";         // 3

    // deque 迭代器非连续：相邻元素地址不保证连续
    std::cout << "addr[0]=" << &d[0]
              << " addr[1]=" << &d[1] << "\n";
    // 二者差不一定是 sizeof(int)

    // 问：以下是否 UB？
    int* p0 = &d[0];
    int* p1 = &d[1];
    // std::ptrdiff_t diff = p1 - p0;  // 可能 UB（非同一数组）

    // 中间 insert/erase 导致所有迭代器失效
    d.insert(d.begin() + 2, 99);
    // it 已失效！
    it = d.begin() + 2;
    std::cout << *it << "\n";  // 99
}
```

**解析**：
- `deque` 内部是**分块连续**（map + 固定大小 block），头尾插入无需移动数据，O(1) 均摊。
- `deque` 的 iterator 是 random-access，支持 `+n`，但底层跨 block 时有额外跳转开销（比 vector 慢约 2-3x）。
- `&d[i]` 与 `&d[i+1]` 不保证差值等于 `sizeof(T)`，对不同 block 的相邻元素做指针算术是 UB。
- 中间 `insert`/`erase` 使**所有**迭代器、指针、引用失效；两端操作只使**对端迭代器**失效（标准 C++03 以后）。
- `std::stack`/`std::queue` 默认用 `deque` 做底层容器。

---

### 题目 53：list 的 splice 与 O(1) 转移

```cpp
#include <iostream>
#include <list>
#include <chrono>
#include <algorithm>

int main() {
    std::list<int> a{1, 2, 3, 4, 5};
    std::list<int> b{10, 20, 30};

    // splice 整个 b 到 a 的第二个位置之前：O(1)，无拷贝
    auto it = std::next(a.begin());
    a.splice(it, b);
    // a: 1,10,20,30,2,3,4,5   b: 空
    for (int x : a) std::cout << x << " ";
    std::cout << "\n";
    std::cout << "b empty: " << b.empty() << "\n";  // 1

    // 单节点 splice：O(1)
    auto it3 = std::find(a.begin(), a.end(), 3);
    b.splice(b.begin(), a, it3);   // 把 3 从 a 移到 b
    for (int x : a) std::cout << x << " ";  // 少了 3
    std::cout << "\n";

    // 排序：list::sort 是归并排序 O(n log n)，稳定，无需随机访问
    a.sort();
    for (int x : a) std::cout << x << " ";
    std::cout << "\n";

    // std::sort(a.begin(), a.end());  // 编译错误：list 不是 RandomAccess

    // merge：合并两个有序 list（两个 list 之后均可能为空）
    std::list<int> c{2, 5, 8};
    std::list<int> d{1, 3, 7, 9};
    c.merge(d);   // d 变空，c 变有序合并结果
    for (int x : c) std::cout << x << " ";
    std::cout << "\n";
}
```

**解析**：
- `list::splice` 通过修改节点指针在 O(1) 内转移元素，**不拷贝、不移动**对象——这是 list 相对 vector 最大优势。
- splice 后，被转移元素的迭代器仍然有效（指向同一节点，只是所属容器变了）。
- `list::sort` 使用**稳定归并排序**，时间 O(n log n)，不需要 RandomAccess 迭代器，且不失效任何迭代器。
- `list::merge` 要求两个 list 已排序；合并后来源 list 变为空（元素被转移）。
- **适用场景**：频繁中间插删、需要 O(1) splice 的数据结构（LRU cache、任务调度）。

---

### 题目 54：unordered_map 哈希表实现与最坏情况

```cpp
#include <iostream>
#include <unordered_map>
#include <map>
#include <vector>
#include <string>
#include <chrono>

// 恶意哈希函数：所有 key 返回同一哈希
struct BadHash {
    std::size_t operator()(int) const { return 42; }
};

void worst_case() {
    // 所有元素都在同一 bucket：退化为链表 O(n)
    std::unordered_map<int, int, BadHash> bad;
    for (int i = 0; i < 10000; ++i)
        bad[i] = i;

    auto t0 = std::chrono::high_resolution_clock::now();
    auto it = bad.find(9999);      // 退化为 O(n) 链式搜索
    auto t1 = std::chrono::high_resolution_clock::now();
    std::cout << "bad hash find: "
              << std::chrono::duration_cast<std::chrono::microseconds>(t1-t0).count()
              << "us\n";
}

// 为自定义类型提供正确的哈希
struct Point { int x, y; };

struct PointHash {
    std::size_t operator()(const Point& p) const {
        // 组合哈希：黄金比例散列
        std::size_t h = std::hash<int>{}(p.x);
        h ^= std::hash<int>{}(p.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

struct PointEq {
    bool operator()(const Point& a, const Point& b) const {
        return a.x == b.x && a.y == b.y;
    }
};

int main() {
    worst_case();

    std::unordered_map<Point, std::string, PointHash, PointEq> grid;
    grid[{0,0}] = "origin";
    grid[{1,2}] = "somewhere";
    std::cout << grid[{0,0}] << "\n";  // origin

    // 查询 bucket 分布
    std::unordered_map<int,int> m;
    for (int i = 0; i < 100; ++i) m[i] = i;
    std::size_t max_chain = 0;
    for (std::size_t b = 0; b < m.bucket_count(); ++b)
        max_chain = std::max(max_chain, m.bucket_size(b));
    std::cout << "max chain: " << max_chain << "\n";  // 理想情况 1-2
    std::cout << "load factor: " << m.load_factor()  << "\n";
}
```

**解析**：
- `unordered_map` 平均 O(1)，但最坏 O(n)——当哈希全冲突时退化为链表遍历。
- 自定义类型需同时提供 `Hash` 和 `Equal`，且满足：`Equal(a,b)==true ⟹ Hash(a)==Hash(b)`。
- 组合多个字段的哈希时，建议用黄金比例偏移 `0x9e3779b9` 避免简单 XOR 的对称缺陷。
- `load_factor = size/bucket_count`，超过 `max_load_factor`（默认 1.0）时自动 rehash，**使所有迭代器失效**。
- 竞赛环境下哈希攻击（hash DoS）：攻击者构造大量同哈希 key，用 `std::map`（红黑树 O(log n)）或自定义随机种子哈希防御。

---

### 题目 55：priority_queue 与自定义比较器的陷阱

```cpp
#include <iostream>
#include <queue>
#include <vector>
#include <functional>
#include <algorithm>

// 演示 5 种构建 min-heap 的方式
int main() {
    // 方式1：greater<int> 标准小顶堆
    std::priority_queue<int, std::vector<int>, std::greater<int>> minq1;
    for (int x : {3,1,4,1,5,9,2,6}) minq1.push(x);
    std::cout << minq1.top() << "\n";  // 1

    // 方式2：lambda 比较器（C++20 之前需要 decltype）
    auto cmp = [](int a, int b){ return a > b; };  // 注意：方向与 sort 相反！
    std::priority_queue<int, std::vector<int>, decltype(cmp)> minq2(cmp);
    for (int x : {3,1,4,1,5}) minq2.push(x);
    std::cout << minq2.top() << "\n";  // 1

    // 方式3：从 vector 建堆 O(n)
    std::vector<int> v{3,1,4,1,5,9,2,6};
    std::make_heap(v.begin(), v.end());           // 大顶堆
    std::cout << v.front() << "\n";               // 9（堆顶在 front）

    // 方式4：自定义结构体
    struct Task { int priority; std::string name; };
    auto task_cmp = [](const Task& a, const Task& b){
        return a.priority < b.priority;  // 大 priority 优先
    };
    std::priority_queue<Task, std::vector<Task>, decltype(task_cmp)> pq(task_cmp);
    pq.push({1, "low"});
    pq.push({10, "high"});
    pq.push({5, "mid"});
    std::cout << pq.top().name << "\n";  // high

    // 陷阱：priority_queue 不支持 decrease-key
    // 解决方案：lazy deletion（标记作废）
    struct Entry { int cost; int node; bool operator>(const Entry& o) const { return cost > o.cost; } };
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> dijkstra_pq;
    std::vector<bool> processed(10, false);
    dijkstra_pq.push({0, 0});
    while (!dijkstra_pq.empty()) {
        auto [c, u] = dijkstra_pq.top(); dijkstra_pq.pop();
        if (processed[u]) continue;  // lazy deletion
        processed[u] = true;
        // process u...
        break;
    }
}
```

**解析**：
- `priority_queue` 的比较器语义与 `sort` **相反**：`cmp(a,b)==true` 表示 a 的**优先级低**于 b（b 排在前面）。
- `std::make_heap` 原地在 vector 上建大顶堆，O(n)（比 N 次 push 的 O(n log n) 更快）。
- `priority_queue` 不支持 decrease-key（无法修改已入队元素的优先级）——Dijkstra 中用 lazy deletion 绕过：重复 push 更新后的代价，弹出时跳过已处理节点。
- `top()` 只读，`pop()` 移除堆顶，二者必须分开调用（不像 `std::queue::front` + `pop`）。
- 底层用 `push_heap`/`pop_heap`/`make_heap`，满足 `complete binary tree` 存储于连续 vector 中。

---

### 题目 56：map 的下界查找与 interval 问题

```cpp
#include <iostream>
#include <map>
#include <string>
#include <optional>

// 区间调度：用 map<int,int> 存储 [start, end] 区间，
// 查询点 x 被哪些区间覆盖
class IntervalMap {
    // key=start, value=end
    std::map<int,int> intervals_;
public:
    void insert(int s, int e) {
        intervals_[s] = std::max(intervals_[s], e);  // 合并同起点
    }

    bool contains(int x) const {
        // upper_bound(x)：第一个 start > x，回退一步就是 start <= x 的最大者
        auto it = intervals_.upper_bound(x);
        if (it == intervals_.begin()) return false;
        --it;
        // it->first <= x，检查 x <= end
        return x <= it->second;
    }

    void print() const {
        for (auto& [s, e] : intervals_)
            std::cout << "[" << s << "," << e << "] ";
        std::cout << "\n";
    }
};

// 字典序最近词（lower_bound 应用）
int main() {
    IntervalMap im;
    im.insert(1, 5);
    im.insert(10, 20);
    im.insert(3, 8);   // 与 [1,5] 重叠
    im.print();

    std::cout << im.contains(4)  << "\n";  // 1（在[1,5]或[3,8]中）
    std::cout << im.contains(7)  << "\n";  // 1（在[3,8]中）
    std::cout << im.contains(9)  << "\n";  // 0
    std::cout << im.contains(15) << "\n";  // 1（在[10,20]中）

    // equal_range 批量查询
    std::map<std::string,int> dict{{"apple",1},{"apply",2},{"apt",3},{"ban",4}};
    std::string prefix = "app";
    auto lo = dict.lower_bound(prefix);
    auto hi = dict.lower_bound(prefix + '\xff');  // 字典序上界
    std::cout << "words with prefix 'app':\n";
    for (auto it = lo; it != hi; ++it)
        std::cout << "  " << it->first << "\n";  // apple, apply
}
```

**解析**：
- `lower_bound(k)` 返回第一个 `key >= k` 的迭代器；`upper_bound(k)` 返回第一个 `key > k` 的迭代器。
- 区间查询的核心：`upper_bound(x)` 找到第一个 start > x，回退一步即 start ≤ x 的最大区间——用一次 `upper_bound` + `--it` 解决。
- `prefix + '\xff'` 是字典序前缀搜索技巧：`'\xff'` 是 ASCII 最大字符，使上界恰好超过所有同前缀字符串。
- `map` 的 `operator[]` 对不存在的 key 会**插入**默认值，用 `find` 或 `count` 查询更安全。
- `std::map` 操作均 O(log n)；`lower_bound` 是成员函数版（比 `std::lower_bound` 泛型版快），因为利用了红黑树结构。

---

### 题目 57：std::string 的 SSO 原理与性能陷阱

```cpp
#include <iostream>
#include <string>
#include <cstring>

// 探测 SSO 阈值
void probe_sso() {
    for (int len = 0; len <= 30; ++len) {
        std::string s(len, 'x');
        // 在 SSO 范围内，string 对象本身存储字符；否则在堆上
        // 启发式：capacity() == sizeof(string)-1 时为 SSO
        bool is_sso = (s.capacity() < sizeof(std::string));
        std::cout << "len=" << len
                  << " cap=" << s.capacity()
                  << " sso=" << is_sso << "\n";
    }
}

// SSO 对拷贝性能的影响
void copy_perf() {
    std::string short_s = "hi";          // SSO：栈拷贝
    std::string long_s(100, 'x');        // 堆分配

    // 短字符串拷贝：无堆分配，几乎零开销
    auto s1 = short_s;
    // 长字符串拷贝：堆 malloc + memcpy
    auto s2 = long_s;

    std::cout << short_s.size() << " " << long_s.size() << "\n";
}

// data() 失效场景
int main() {
    probe_sso();

    std::string s = "hello";
    const char* ptr = s.data();

    // 以下操作可能导致 ptr 失效：
    s += " world";              // 若超出 SSO，重新分配堆内存
    // std::cout << ptr;        // UB！ptr 可能悬垂

    // 安全：重新获取
    ptr = s.data();
    std::cout << ptr << "\n";  // hello world

    // string_view 不拥有内存，注意生命周期
    std::string_view sv = s;
    s.clear();
    // std::cout << sv;          // UB！s 被清空，sv 悬垂
}
```

**解析**：
- SSO（Small String Optimization）：字符串短于某阈值（GCC libstdc++ 为 15 字节，MSVC 为 15 字节，libc++ 为 22 字节）时直接存在 `string` 对象内部，避免堆分配。
- 对短字符串，`std::string` 的拷贝接近 `memcpy` 一个栈对象的开销；长字符串则需 `malloc` + 复制。
- `operator+=` 若触发扩容（超出 SSO 或 capacity），`data()` 返回的指针**全部失效**。
- `string_view` 是非 owning 视图，原字符串析构/重分配后，`string_view` 立即悬垂，是最常见的生命周期 bug 之一。
- C++17 起 `string::data()` 返回 `char*`（non-const），允许原地修改字符，但不能改变 `size()`。

---

### 题目 58：std::sort 的实现细节与 introsort

```cpp
#include <iostream>
#include <algorithm>
#include <vector>
#include <functional>
#include <random>

// 手写 3 路快排（荷兰国旗划分）
template<typename It>
void three_way_qsort(It first, It last) {
    if (last - first <= 1) return;
    auto pivot = *first;
    auto lt = first, gt = last, i = first + 1;
    while (i != gt) {
        if (*i < pivot)       std::iter_swap(i++, lt++);
        else if (*i > pivot)  std::iter_swap(i, --gt);
        else                  ++i;
    }
    three_way_qsort(first, lt);
    three_way_qsort(gt, last);
}

// introsort 骨架（std::sort 的典型实现）
template<typename It>
void introsort(It first, It last, int depth_limit) {
    auto size = last - first;
    if (size <= 16) {
        // 小段用插入排序
        for (auto i = first + 1; i != last; ++i)
            for (auto j = i; j != first && *j < *(j-1); --j)
                std::iter_swap(j, j-1);
        return;
    }
    if (depth_limit == 0) {
        // 递归过深，退化到堆排序 O(n log n) 最坏
        std::make_heap(first, last);
        std::sort_heap(first, last);
        return;
    }
    // 三数取中选 pivot
    auto mid = first + size/2;
    if (*mid < *first) std::iter_swap(mid, first);
    if (*last-1 < *first) std::iter_swap(last-1, first);
    if (*mid < *(last-1)) std::iter_swap(mid, last-1);
    auto pivot = *(last-1);

    auto p = std::partition(first, last-1, [&](auto x){ return x < pivot; });
    std::iter_swap(p, last-1);

    introsort(first, p, depth_limit - 1);
    introsort(p + 1, last, depth_limit - 1);
}

int main() {
    std::vector<int> v(1000);
    std::mt19937 rng(42);
    std::iota(v.begin(), v.end(), 0);
    std::shuffle(v.begin(), v.end(), rng);

    three_way_qsort(v.begin(), v.end());
    std::cout << "sorted: " << std::is_sorted(v.begin(), v.end()) << "\n";  // 1

    // 对重复元素，3 路快排远优于 2 路
    std::vector<int> dup(10000, 5);
    three_way_qsort(dup.begin(), dup.end());  // O(n)，因为全部相同直接完成

    // std::sort 不稳定；stable_sort 用归并排序 O(n log n)
    struct Item { int key, order; };
    std::vector<Item> items{{3,0},{1,1},{3,2},{1,3}};
    std::stable_sort(items.begin(), items.end(),
                     [](auto& a, auto& b){ return a.key < b.key; });
    for (auto& x : items) std::cout << x.key << "(" << x.order << ") ";
    std::cout << "\n";  // 1(1) 1(3) 3(0) 3(2)
}
```

**解析**：
- `std::sort` 通常实现为 **introsort**：先快排，递归深度超过 `2*log2(n)` 时切换到堆排序，保证最坏 O(n log n)；小段（≤16 元素）切换为插入排序（常数小）。
- **3 路快排**对重复元素极其高效：相同元素只在 `[lt,gt)` 区间内，不参与递归，全相同数组 O(n)。
- `std::stable_sort` 稳定（相等元素保持原顺序），用归并排序，O(n log n) 但常数大于 `sort`。
- 三数取中选 pivot（first/mid/last）避免有序输入的退化 O(n²)。
- 比较器必须满足**严格弱序**（不自反、传递、非对称），否则 UB（程序可能崩溃或无限循环）。

---

### 题目 59：std::lower_bound 的泛化与手写

```cpp
#include <iostream>
#include <algorithm>
#include <vector>

// 泛化版 lower_bound：在任意满足单调性的范围上二分
// 找到最小 x in [lo, hi) 使得 pred(x) == true
// 要求：pred 先 false 后 true（单调）
template<typename T, typename Pred>
T lower_bound_pred(T lo, T hi, Pred pred) {
    while (lo < hi) {
        T mid = lo + (hi - lo) / 2;
        if (pred(mid)) hi = mid;
        else           lo = mid + 1;
    }
    return lo;  // lo == hi，即第一个 pred(x)==true 的位置
}

// 实数二分（浮点）
double sqrt_binary(double n, double eps = 1e-9) {
    double lo = 0, hi = n;
    while (hi - lo > eps) {
        double mid = (lo + hi) / 2;
        if (mid * mid >= n) hi = mid;
        else                lo = mid;
    }
    return lo;
}

// 在旋转有序数组中查找
int search_rotated(const std::vector<int>& v, int target) {
    int lo = 0, hi = (int)v.size() - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (v[mid] == target) return mid;
        if (v[lo] <= v[mid]) {           // 左半有序
            if (v[lo] <= target && target < v[mid]) hi = mid - 1;
            else                                     lo = mid + 1;
        } else {                          // 右半有序
            if (v[mid] < target && target <= v[hi]) lo = mid + 1;
            else                                     hi = mid - 1;
        }
    }
    return -1;
}

int main() {
    // 找最小 x 满足 x^2 >= 50（整数开方上取整）
    int sq = lower_bound_pred(0, 100, [](int x){ return x*x >= 50; });
    std::cout << sq << "\n";   // 8（8²=64>=50, 7²=49<50）

    std::cout << sqrt_binary(2.0) << "\n";   // ~1.41421356

    std::vector<int> rot{4,5,6,7,0,1,2};
    std::cout << search_rotated(rot, 0) << "\n";  // 4
    std::cout << search_rotated(rot, 3) << "\n";  // -1

    // 标准库版本：在 vector 上用自定义投影
    std::vector<std::pair<int,std::string>> pairs{{1,"a"},{3,"b"},{5,"c"},{7,"d"}};
    auto it = std::lower_bound(pairs.begin(), pairs.end(), 4,
        [](const auto& p, int v){ return p.first < v; });
    std::cout << it->first << "\n";  // 5（第一个 key >= 4）
}
```

**解析**：
- 二分查找的本质是在**单调谓词**上找边界，不仅限于有序数组，可用于实数域、答案空间（二分答案）。
- **整数二分**的边界写法：`lo + (hi - lo) / 2` 避免 `(lo+hi)/2` 在大数时溢出。
- 旋转有序数组：每次判断哪一半有序（靠比较 `v[lo]` 与 `v[mid]`），在有序半上二分。
- `std::lower_bound` 第4参数是比较器 `comp(element, value)`（注意参数顺序：元素在前，目标值在后）。
- 二分答案技巧：若"满足条件"与"不满足条件"在答案空间上单调，直接二分答案，内层验证。

---

### 题目 60：std::transform 与 std::accumulate 的组合力量

```cpp
#include <iostream>
#include <vector>
#include <string>
#include <numeric>
#include <algorithm>
#include <functional>
#include <sstream>

int main() {
    std::vector<int> v{1, 2, 3, 4, 5};

    // 1. 前缀和（包含式）
    std::vector<int> prefix(v.size());
    std::partial_sum(v.begin(), v.end(), prefix.begin());
    for (int x : prefix) std::cout << x << " ";  // 1 3 6 10 15
    std::cout << "\n";

    // 2. 点积（transform + accumulate）
    std::vector<int> w{2, 3, 4, 5, 6};
    int dot = std::transform_reduce(v.begin(), v.end(), w.begin(), 0);
    std::cout << "dot=" << dot << "\n";  // 2+6+12+20+30=70

    // 3. 字符串 join（accumulate 字符串版）
    std::vector<std::string> words{"hello", "world", "foo"};
    std::string joined = std::accumulate(
        std::next(words.begin()), words.end(), words[0],
        [](std::string a, const std::string& b){ return std::move(a) + "," + b; });
    std::cout << joined << "\n";  // hello,world,foo

    // 4. 差分数组（相邻差）
    std::vector<int> diff(v.size());
    std::adjacent_difference(v.begin(), v.end(), diff.begin());
    for (int x : diff) std::cout << x << " ";  // 1 1 1 1 1
    std::cout << "\n";

    // 5. 扫描（inclusive_scan / exclusive_scan，C++17）
    std::vector<int> inc(v.size()), exc(v.size());
    std::inclusive_scan(v.begin(), v.end(), inc.begin());
    std::exclusive_scan(v.begin(), v.end(), exc.begin(), 0);
    for (int x : inc) std::cout << x << " ";  // 1 3 6 10 15
    std::cout << "\n";
    for (int x : exc) std::cout << x << " ";  // 0 1 3 6 10
    std::cout << "\n";

    // 6. 多条件 transform（两个输入范围）
    std::vector<int> result(v.size());
    std::transform(v.begin(), v.end(), w.begin(), result.begin(),
                   [](int a, int b){ return a * b - a; });
    for (int x : result) std::cout << x << " ";  // 1 4 9 16 25
    std::cout << "\n";
}
```

**解析**：
- `std::partial_sum` 计算包含式前缀和；`std::exclusive_scan` 是排除式（当前元素不计入，从初始值开始）。
- `std::transform_reduce` = map + reduce，在 C++17 中还支持并行执行策略（`std::execution::par`）。
- `accumulate` 的 `std::next(words.begin())` 技巧：避免第一个元素被多加一次分隔符。
- `adjacent_difference` 计算相邻元素之差（差分数组），逆操作是 `partial_sum`。
- 二元 `transform`（接受两个输入范围）的第4参数是输出 begin，第5参数才是操作符——注意参数顺序。

---

### 题目 61：std::move_iterator 与隐式移动

```cpp
#include <iostream>
#include <vector>
#include <string>
#include <iterator>
#include <algorithm>

int main() {
    std::vector<std::string> src{"hello", "world", "foo", "bar"};

    // 用 move_iterator 批量移动（而非拷贝）到新容器
    std::vector<std::string> dst(
        std::make_move_iterator(src.begin()),
        std::make_move_iterator(src.end())
    );

    std::cout << "dst[0]=" << dst[0] << "\n";  // hello
    std::cout << "src[0]=" << src[0] << "\n";  // ""（已被移走）

    // 与 transform 配合：move + transform
    std::vector<std::string> words{"alpha", "beta", "gamma"};
    std::vector<std::string> upper(words.size());
    std::transform(
        std::make_move_iterator(words.begin()),
        std::make_move_iterator(words.end()),
        upper.begin(),
        [](std::string s) {           // 参数按值：move 进来
            for (char& c : s) c = std::toupper(c);
            return s;                 // NRVO 移出
        }
    );
    for (auto& s : upper) std::cout << s << " ";
    std::cout << "\n";  // ALPHA BETA GAMMA

    // move_iterator 解包：*mit 返回右值引用
    std::vector<std::vector<int>> src2{{1,2},{3,4},{5,6}};
    std::vector<std::vector<int>> dst2;
    dst2.reserve(src2.size());
    auto mit = std::make_move_iterator(src2.begin());
    auto mend = std::make_move_iterator(src2.end());
    while (mit != mend)
        dst2.push_back(*mit++);  // *mit 是右值引用，触发移动
    std::cout << "src2[0] empty: " << src2[0].empty() << "\n";  // 1
}
```

**解析**：
- `std::make_move_iterator` 包装迭代器，使解引用返回右值引用（`T&&`），触发移动而非拷贝。
- 配合范围构造函数使用时，批量移动的时间复杂度与拷贝相同（O(n)），但每个元素避免深拷贝——对 `string`/`vector` 等类型性能差异显著。
- `transform` + `move_iterator`：lambda 按值接收参数，接收的是右值引用——编译器会移动构造参数，函数体内可随意修改。
- 移动后的源元素处于**有效但未指定状态**（valid but unspecified），通常为空/零，但不能依赖具体值。
- C++17 起 `std::copy`/`std::transform` 等算法有并行重载，配合 `move_iterator` 时注意线程安全。

---

### 题目 62：std::bind 与占位符的替换

```cpp
#include <iostream>
#include <functional>
#include <algorithm>
#include <vector>

void print3(int a, int b, int c) {
    std::cout << a << " " << b << " " << c << "\n";
}

int main() {
    using namespace std::placeholders;

    // 固定参数
    auto f1 = std::bind(print3, 1, _1, _2);
    f1(10, 20);   // 1 10 20

    // 重排参数
    auto f2 = std::bind(print3, _3, _1, _2);
    f2(10, 20, 30);  // 30 10 20（_3=30, _1=10, _2=20）

    // bind 成员函数
    struct Calc {
        int base;
        int add(int x) const { return base + x; }
    };
    Calc c{100};
    auto add100 = std::bind(&Calc::add, &c, _1);
    std::cout << add100(42) << "\n";  // 142

    // 现代替代：lambda 更清晰
    auto add100_lambda = [&c](int x){ return c.add(x); };
    std::cout << add100_lambda(42) << "\n";  // 142

    // bind 在算法中的应用（C++14 后 lambda 更通用）
    std::vector<int> v{1,2,3,4,5,6,7,8,9,10};
    auto gt5 = std::bind(std::greater<int>(), _1, 5);
    auto cnt = std::count_if(v.begin(), v.end(), gt5);
    std::cout << cnt << "\n";  // 5（6,7,8,9,10）

    // bind 的性能问题：类型擦除开销
    // 更好方案：
    auto cnt2 = std::count_if(v.begin(), v.end(), [](int x){ return x > 5; });
    std::cout << cnt2 << "\n";  // 5
}
```

**解析**：
- `std::bind` 返回一个函数对象，占位符 `_1`/`_2` 表示调用时传入的第 1/2 个参数，可任意重排。
- `_3` 在函数中排在第3位，但在调用 `f2(10,20,30)` 时对应第3个实参 `30`——占位符下标是**调用**时的参数顺序，不是绑定函数的参数顺序。
- 绑定成员函数时必须传递对象（指针或引用），且对象在 bind 的对象列表中排在其他参数之前。
- **现代实践**：`std::bind` 已基本被 lambda 取代——lambda 更直观、无额外类型擦除、编译器可更好内联。
- `bind` 在 C++20 的 `std::bind_front` 中有更简洁的前绑定版本：`std::bind_front(&Calc::add, &c)` 等价于 `add100`。

---

### 题目 63：迭代器适配器与 iota/generate

```cpp
#include <iostream>
#include <iterator>
#include <vector>
#include <numeric>
#include <algorithm>
#include <sstream>

int main() {
    // iota：填充连续整数
    std::vector<int> v(10);
    std::iota(v.begin(), v.end(), 1);      // 1,2,...,10
    for (int x : v) std::cout << x << " ";
    std::cout << "\n";

    // generate：用生成器填充
    std::vector<int> fibs(10);
    int a = 0, b = 1;
    std::generate(fibs.begin(), fibs.end(), [&]() mutable {
        int tmp = a; a = b; b = tmp + b; return tmp;
    });
    for (int x : fibs) std::cout << x << " ";
    std::cout << "\n";  // 0 1 1 2 3 5 8 13 21 34

    // back_inserter：向容器末尾插入
    std::vector<int> src{1,2,3,4,5};
    std::vector<int> dst;
    std::copy_if(src.begin(), src.end(), std::back_inserter(dst),
                 [](int x){ return x % 2; });
    for (int x : dst) std::cout << x << " ";  // 1 3 5
    std::cout << "\n";

    // ostream_iterator：直接输出到流
    std::copy(v.begin(), v.end(),
              std::ostream_iterator<int>(std::cout, ","));
    std::cout << "\n";  // 1,2,3,4,5,6,7,8,9,10,

    // istream_iterator：从流读取
    std::istringstream ss("10 20 30 40 50");
    std::vector<int> from_stream{
        std::istream_iterator<int>(ss),
        std::istream_iterator<int>()
    };
    for (int x : from_stream) std::cout << x << " ";
    std::cout << "\n";  // 10 20 30 40 50
}
```

**解析**：
- `std::iota(first, last, val)` 用递增序列填充，等价于 `for (auto it=first; it!=last; ++it, ++val) *it=val`。
- `generate` 的生成器 lambda 必须是无参调用对象；`generate_n` 指定生成个数（配合 `back_inserter`）。
- `back_inserter` 返回一个迭代器，`*it = val` 等价于 `container.push_back(val)`，无需预分配 `dst`。
- `ostream_iterator` 将每次赋值转为 `os << value << delimiter` 输出——注意分隔符在最后一个元素后也会输出。
- `istream_iterator<T>()` 是默认构造的哨兵（end of stream），用于构造 range `[begin, end)`。

---

### 题目 64：std::optional 的 monadic 链式操作

```cpp
#include <iostream>
#include <optional>
#include <string>
#include <charconv>

// 链式解析：字符串 -> int -> 区间验证 -> 字符串
std::optional<int> parse_int(std::string_view sv) {
    int val;
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), val);
    if (ec != std::errc{} || ptr != sv.data() + sv.size())
        return std::nullopt;
    return val;
}

std::optional<int> in_range(int x, int lo, int hi) {
    if (x < lo || x > hi) return std::nullopt;
    return x;
}

std::string to_grade(int score) {
    if (score >= 90) return "A";
    if (score >= 75) return "B";
    if (score >= 60) return "C";
    return "D";
}

int main() {
    // C++23 monadic operations
    auto result = parse_int("85")
        .and_then([](int x){ return in_range(x, 0, 100); })  // C++23
        .transform([](int x){ return to_grade(x); });         // C++23

    if (result) std::cout << *result << "\n";  // B

    auto bad = parse_int("abc")
        .and_then([](int x){ return in_range(x, 0, 100); })
        .transform([](int x){ return to_grade(x); })
        .value_or("invalid");
    std::cout << bad << "\n";  // invalid

    // C++17 替代写法
    auto manual = [](std::string_view sv) -> std::string {
        auto a = parse_int(sv);
        if (!a) return "invalid";
        auto b = in_range(*a, 0, 100);
        if (!b) return "out of range";
        return to_grade(*b);
    };
    std::cout << manual("85") << "\n";   // B
    std::cout << manual("999") << "\n";  // out of range

    // or_else（C++23）：提供备选
    auto with_default = parse_int("bad").or_else([]() -> std::optional<int> {
        return 0;  // 解析失败时返回 0
    });
    std::cout << *with_default << "\n";  // 0
}
```

**解析**：
- `optional::and_then(f)`（C++23）：有值时调用 `f(*opt)` 并返回其结果（必须是 optional），无值时传播 nullopt——实现 optional 的 flatMap/bind。
- `optional::transform(f)`（C++23）：有值时调用 `f(*opt)` 并包装为 optional，无值时传播——实现 optional 的 map。
- `optional::or_else(f)`（C++23）：有值时直接返回，无值时调用 `f()` 提供备选——实现 optional 的 recover。
- C++23 monadic 接口使可选值的链式处理类似 Haskell 的 Maybe monad，消除了大量 `if (!opt) return nullopt;` 样板代码。
- `std::from_chars` 是 C++17 高性能无分配解析，返回 `{ptr, errc}`；不受 locale 影响，适合协议解析。

---

### 题目 65：std::variant 的递归类型与 JSON AST

```cpp
#include <iostream>
#include <variant>
#include <vector>
#include <map>
#include <string>
#include <memory>

// 递归 variant：JSON 值类型
// 不能直接写 variant<..., vector<JsonValue>, ...>（不完整类型），
// 需要用 unique_ptr 或 recursive_wrapper 间接
struct JsonValue;
using JsonArray  = std::vector<std::unique_ptr<JsonValue>>;
using JsonObject = std::map<std::string, std::unique_ptr<JsonValue>>;

struct JsonValue {
    std::variant<
        std::nullptr_t,
        bool,
        double,
        std::string,
        JsonArray,
        JsonObject
    > data;

    static JsonValue null()           { return {nullptr}; }
    static JsonValue boolean(bool b)  { return {b}; }
    static JsonValue number(double d) { return {d}; }
    static JsonValue string(std::string s){ return {std::move(s)}; }
};

std::string stringify(const JsonValue& jv);

std::string stringify(const JsonValue& jv) {
    return std::visit([](auto&& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) return "null";
        if constexpr (std::is_same_v<T, bool>)           return v ? "true" : "false";
        if constexpr (std::is_same_v<T, double>)         return std::to_string(v);
        if constexpr (std::is_same_v<T, std::string>)    return "\"" + v + "\"";
        if constexpr (std::is_same_v<T, JsonArray>) {
            std::string s = "[";
            for (auto& elem : v) s += stringify(*elem) + ",";
            if (!v.empty()) s.pop_back();
            return s + "]";
        }
        if constexpr (std::is_same_v<T, JsonObject>) {
            std::string s = "{";
            for (auto& [k,val] : v) s += "\"" + k + "\":" + stringify(*val) + ",";
            if (!v.empty()) s.pop_back();
            return s + "}";
        }
    }, jv.data);
}

int main() {
    JsonValue root;
    root.data = JsonObject{};
    auto& obj = std::get<JsonObject>(root.data);
    obj["name"] = std::make_unique<JsonValue>(JsonValue::string("Alice"));
    obj["age"]  = std::make_unique<JsonValue>(JsonValue::number(30));
    obj["active"] = std::make_unique<JsonValue>(JsonValue::boolean(true));

    JsonArray arr;
    arr.push_back(std::make_unique<JsonValue>(JsonValue::number(1)));
    arr.push_back(std::make_unique<JsonValue>(JsonValue::number(2)));
    obj["scores"] = std::make_unique<JsonValue>(JsonValue{std::move(arr)});

    std::cout << stringify(root) << "\n";
}
```

**解析**：
- `std::variant` 不支持不完整类型成员，递归 variant 需用 `unique_ptr` 或 `recursive_variant`（Boost）间接包装。
- `std::visit` 的 `if constexpr` 分支：编译器为每种 alternative 实例化不同代码，无运行时开销。
- `variant` 的内存布局：大小等于最大 alternative 加上 discriminant（类型标签），通常对齐到最大对齐要求。
- `std::get<T>(v)` 在类型不匹配时抛 `std::bad_variant_access`；`std::get_if<T>(&v)` 返回指针（失败返回 nullptr）。
- `valueless_by_exception`：variant 在转换中途抛出异常时进入此特殊状态，`index()` 返回 `variant_npos`。

---

### 题目 66：std::any 与 type_index

```cpp
#include <iostream>
#include <any>
#include <typeindex>
#include <unordered_map>
#include <functional>
#include <string>

// 用 any 实现异构容器 + 类型安全分发
class AnyDispatcher {
    using Handler = std::function<void(const std::any&)>;
    std::unordered_map<std::type_index, Handler> handlers_;
public:
    template<typename T>
    void on(std::function<void(const T&)> h) {
        handlers_[typeid(T)] = [h](const std::any& a) {
            h(std::any_cast<const T&>(a));
        };
    }

    void dispatch(const std::any& val) const {
        auto it = handlers_.find(val.type());
        if (it != handlers_.end()) it->second(val);
        else std::cout << "no handler for " << val.type().name() << "\n";
    }
};

int main() {
    AnyDispatcher disp;
    disp.on<int>([](int v){ std::cout << "int: " << v << "\n"; });
    disp.on<std::string>([](const std::string& s){ std::cout << "str: " << s << "\n"; });
    disp.on<double>([](double d){ std::cout << "double: " << d << "\n"; });

    std::vector<std::any> events{42, std::string("hello"), 3.14, true};
    for (auto& e : events) disp.dispatch(e);
    // int: 42 / str: hello / double: 3.14 / no handler for bool

    // any_cast 的两种形式
    std::any a = std::string("world");
    try {
        std::string& ref = std::any_cast<std::string&>(a);  // 引用，无拷贝
        ref += "!";
        std::cout << std::any_cast<std::string>(a) << "\n";  // world!
        int bad = std::any_cast<int>(a);     // 抛 bad_any_cast
    } catch (const std::bad_any_cast& e) {
        std::cout << "bad_any_cast: " << e.what() << "\n";
    }

    // any_cast 指针版：失败返回 nullptr
    if (auto* p = std::any_cast<std::string>(&a))
        std::cout << "got: " << *p << "\n";
}
```

**解析**：
- `std::any` 用类型擦除存储任意 CopyConstructible 类型，通过 RTTI 的 `type_info` 跟踪当前类型。
- `std::type_index` 对 `type_info` 做了哈希/比较包装，适合用作 `unordered_map` 的 key。
- `any_cast<T&>(a)` 返回引用（高效），失败抛异常；`any_cast<T*>(&a)` 返回指针（失败为 nullptr，不抛）。
- `any` 内部通常实现 SOO（Small Object Optimization）：小对象（如 int、double）存在 `any` 对象内，大对象在堆上。
- 与 `variant` 对比：`any` 是**开放**类型集合（运行时添加），`variant` 是**封闭**类型集合（编译期固定）；`any` 无法 `visit` 所有可能类型。

---

### 题目 67：std::function 的对象切片与开销

```cpp
#include <iostream>
#include <functional>
#include <chrono>
#include <vector>

int add(int a, int b) { return a + b; }

struct FuncObj {
    int multiplier;
    int operator()(int a, int b) const { return (a + b) * multiplier; }
};

void benchmark() {
    constexpr int N = 10'000'000;
    int result = 0;

    // 直接函数指针：快
    auto t0 = std::chrono::high_resolution_clock::now();
    int (*fp)(int,int) = add;
    for (int i = 0; i < N; ++i) result += fp(i, 1);
    auto t1 = std::chrono::high_resolution_clock::now();

    // std::function：类型擦除间接调用，有开销
    std::function<int(int,int)> f = add;
    auto t2 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < N; ++i) result += f(i, 1);
    auto t3 = std::chrono::high_resolution_clock::now();

    using ms = std::chrono::milliseconds;
    std::cout << "func_ptr: "    << std::chrono::duration_cast<ms>(t1-t0).count() << "ms\n";
    std::cout << "std::function: " << std::chrono::duration_cast<ms>(t3-t2).count() << "ms\n";
    (void)result;
}

int main() {
    // 1. 函数指针
    std::function<int(int,int)> f1 = add;
    std::cout << f1(3,4) << "\n";   // 7

    // 2. lambda
    std::function<int(int,int)> f2 = [](int a, int b){ return a * b; };
    std::cout << f2(3,4) << "\n";   // 12

    // 3. 有状态的函数对象
    std::function<int(int,int)> f3 = FuncObj{2};
    std::cout << f3(3,4) << "\n";   // 14

    // 4. 成员函数（需绑定对象）
    struct Obj { int val; int get(int x) const { return val + x; } };
    Obj obj{10};
    std::function<int(int)> f4 = [&obj](int x){ return obj.get(x); };
    std::cout << f4(5) << "\n";     // 15

    // 空 function 调用抛异常
    std::function<void()> empty;
    try { empty(); }
    catch (const std::bad_function_call& e) {
        std::cout << "bad_function_call\n";
    }

    benchmark();
}
```

**解析**：
- `std::function` 通过虚函数表（或等效的函数指针表）实现类型擦除，每次调用有间接跳转——比直接函数调用慢约 2-5x。
- `function` 内有 SOO 缓冲区（通常 16-32 字节），小 callable（无状态 lambda、函数指针）存在缓冲区内；大 callable 在堆上分配。
- **替代方案**：模板参数传递 callable（`template<typename F> void foo(F f)`）完全无开销，编译器可内联；`std::move_only_function`（C++23）支持仅移动的 callable。
- 成员函数需要绑定对象，推荐用 lambda 捕获（而非 `std::bind`），更清晰。
- `function` 是 CopyConstructible 的，包含的 callable 也必须可拷贝；若 callable 是 `unique_ptr` 的 lambda，则需 `move_only_function`。

---

### 题目 68：ranges::views 惰性管道

```cpp
#include <iostream>
#include <ranges>
#include <vector>
#include <string>
#include <algorithm>

int main() {
    std::vector<int> v{1,2,3,4,5,6,7,8,9,10};

    // 管道：filter + transform + take —— 惰性，零中间容器
    auto pipeline = v
        | std::views::filter([](int x){ return x % 2 == 0; })   // 偶数
        | std::views::transform([](int x){ return x * x; })     // 平方
        | std::views::take(3);                                    // 前3个

    for (int x : pipeline) std::cout << x << " ";   // 4 16 36
    std::cout << "\n";

    // iota_view：无限整数序列
    auto nat = std::views::iota(1)
        | std::views::filter([](int x){ return x % 3 == 0; })
        | std::views::take(5);
    for (int x : nat) std::cout << x << " ";    // 3 6 9 12 15
    std::cout << "\n";

    // zip（C++23）
    std::vector<std::string> names{"Alice","Bob","Charlie"};
    std::vector<int> scores{90, 85, 92};
    for (auto [name, score] : std::views::zip(names, scores))
        std::cout << name << ":" << score << " ";
    std::cout << "\n";

    // split + join
    std::string csv = "hello,world,foo,bar";
    for (auto word : csv | std::views::split(',')) {
        std::cout << std::string_view(word) << " ";
    }
    std::cout << "\n";

    // reverse + drop
    auto last3 = v | std::views::reverse | std::views::take(3);
    for (int x : last3) std::cout << x << " ";  // 10 9 8
    std::cout << "\n";
}
```

**解析**：
- C++20 `ranges::views` 采用**惰性求值**：管道表达式不立即计算，迭代时才逐元素按管道处理，无中间容器分配。
- `views::iota(1)` 是无限序列，只有配合 `take` 或其他终止条件才安全迭代。
- `views::zip`（C++23）将多个 range 并行迭代，短的那个决定长度。
- `views::split` 按分隔符切割，每个子 range 是原始 string 的子视图（zero-copy）。
- 注意：views 存储迭代器/引用到原容器，若原容器被修改或销毁，view 悬垂——不要把 view 存储超过原容器生命周期。

---

### 题目 69：std::string_view 的零拷贝与生命周期陷阱

```cpp
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

// 安全：string_view 参数，接受 string/字面量/char* 无拷贝
std::size_t count_vowels(std::string_view sv) {
    std::size_t cnt = 0;
    for (char c : sv)
        if (std::string_view("aeiouAEIOU").find(c) != std::string_view::npos)
            ++cnt;
    return cnt;
}

// 危险：返回 string_view 时的生命周期问题
std::string_view dangerous() {
    std::string s = "temporary";
    return s;   // 返回指向已销毁局部变量的 view！UB
}

std::string_view safe(std::string& s) {
    return s;   // OK：s 的生命周期由调用者管理
}

int main() {
    // 接受任意字符串类型
    std::cout << count_vowels("Hello World") << "\n";  // 3
    std::string s = "Beautiful";
    std::cout << count_vowels(s) << "\n";              // 5

    // string_view 的 substr 是零拷贝
    std::string_view sv = "Hello, World!";
    auto sub = sv.substr(7, 5);  // "World"，无分配
    std::cout << sub << "\n";

    // 陷阱1：从临时 string 构造
    std::string_view bad_sv = std::string("temp");  // 临时 string 已析构！
    // std::cout << bad_sv;  // UB

    // 陷阱2：string 重新分配后 view 失效
    std::string growing = "short";
    std::string_view view = growing;
    growing += std::string(100, 'x');  // 可能 reallocation
    // std::cout << view;   // UB！view 指向旧内存

    // 正确：需要存储时转为 string
    std::string owned{sub};    // 按需拷贝
    std::cout << owned << "\n";

    // starts_with / ends_with（C++20）
    std::string_view url = "https://example.com";
    std::cout << url.starts_with("https") << "\n";  // 1
    std::cout << url.ends_with(".com")    << "\n";  // 1
}
```

**解析**：
- `string_view` 是只读的非 owning 视图，内部只有 `{const char* data, size_t size}`，构造和拷贝 O(1)。
- **最常见 bug**：从临时 `string` 隐式转 `string_view`——临时对象在全表达式结束后析构，view 立即悬垂。
- `string` 的 `push_back`/`append`/`+=` 若触发 reallocation，之前从该 `string` 创建的所有 `string_view` 全部悬垂。
- `string_view::substr` 是 O(1) 视图切片，而 `string::substr` 是 O(n) 拷贝——传参时用 `string_view` 可避免无谓拷贝。
- C++20 新增 `starts_with`/`ends_with`；C++23 新增 `contains`。

---

### 题目 70：std::span 与连续内存的零拷贝接口

```cpp
#include <iostream>
#include <span>
#include <vector>
#include <array>
#include <algorithm>
#include <numeric>

// 接受任意连续存储的通用接口
void process(std::span<const int> data) {
    int sum = std::reduce(data.begin(), data.end(), 0);
    auto [mn, mx] = std::minmax_element(data.begin(), data.end());
    std::cout << "n=" << data.size()
              << " sum=" << sum
              << " min=" << *mn << " max=" << *mx << "\n";
}

void modify(std::span<int> data) {
    for (int& x : data) x *= 2;
}

int main() {
    // 从 vector 构造
    std::vector<int> v{1,2,3,4,5};
    process(v);

    // 从 array 构造
    std::array<int,5> arr{10,20,30,40,50};
    process(arr);

    // 从裸数组构造
    int raw[] = {100,200,300};
    process(raw);

    // 静态 extent（编译期大小）
    std::span<int, 5> fixed{v};    // 编译期知道 size=5
    std::cout << "fixed size: " << fixed.size() << "\n";  // 5

    // 子 span
    auto sub = std::span{v}.subspan(1, 3);  // v[1..3]
    process(sub);   // n=3 sum=9

    // 修改
    modify(v);
    for (int x : v) std::cout << x << " ";  // 2 4 6 8 10
    std::cout << "\n";

    // span 不拥有数据：不要返回指向局部变量的 span
    auto make_span = []() -> std::span<int> {
        // static 或 caller 拥有的数据才能安全返回
        static std::array<int,3> local{1,2,3};
        return local;
    };
    auto s = make_span();
    process(s);
}
```

**解析**：
- `std::span<T>` 是对**连续内存**的非 owning 视图，类比 `string_view` 但可写（`T` 非 const 时）。
- 统一接受 `vector`/`array`/`C数组`/`span` 本身——消除了历史上 `T* data, size_t n` 的不安全 API。
- `span<int, N>` 静态 extent：大小在编译期固定，`sizeof(span<int,5>)` 等于 `sizeof(pointer)`（无需存 size）。
- `span<int>` 动态 extent：存 pointer + size，`sizeof == 2 * sizeof(pointer)`。
- **生命周期**：span 不延长原容器的生命周期，函数返回 span 指向局部容器是 UB；返回静态/外部数据的 span 安全。

---

### 题目 71：std::bitset 的编译期与运行期操作

```cpp
#include <iostream>
#include <bitset>
#include <string>
#include <bit>

int main() {
    // 编译期大小固定
    std::bitset<16> bs;
    bs.set(3);    // bit 3 = 1
    bs.set(7);
    bs.flip(7);   // bit 7 toggle -> 0
    bs.set(15);

    std::cout << bs << "\n";              // 1000000000001000
    std::cout << bs.count() << "\n";      // 2 (bit 3, bit 15)
    std::cout << bs.to_ulong() << "\n";   // 32776 = 2^3 + 2^15

    // 位运算
    std::bitset<8> a{"11001010"};
    std::bitset<8> b{"10110011"};
    std::cout << (a & b) << "\n";   // 10000010
    std::cout << (a | b) << "\n";   // 11111011
    std::cout << (a ^ b) << "\n";   // 01111001
    std::cout << (~a)    << "\n";   // 00110101
    std::cout << (a << 2)<< "\n";   // 00101000

    // C++20 <bit> 的 popcount/clz/ctz
    unsigned x = 0b1010'1100;
    std::cout << std::popcount(x) << "\n";         // 4 (4个1)
    std::cout << std::countl_zero(x) << "\n";      // 24 (32位，前24位为0)
    std::cout << std::countr_zero(x) << "\n";      // 2 (末尾2个0)
    std::cout << std::has_single_bit(x) << "\n";   // 0 (不是2的幂)
    std::cout << std::bit_width(x) << "\n";         // 8 (最高位在第8位)

    // 布隆过滤器概念演示
    constexpr int M = 64;
    std::bitset<M> bloom;
    auto h1 = [](int x){ return std::hash<int>{}(x) % M; };
    auto h2 = [](int x){ return (std::hash<int>{}(x*2+1)) % M; };
    auto add = [&](int x){ bloom.set(h1(x)); bloom.set(h2(x)); };
    auto might_contain = [&](int x){ return bloom.test(h1(x)) && bloom.test(h2(x)); };

    add(42); add(100);
    std::cout << might_contain(42)  << "\n";  // 1
    std::cout << might_contain(999) << "\n";  // 0（大概率）
}
```

**解析**：
- `std::bitset<N>` 大小 N 是**编译期常量**，底层用若干 `unsigned long` 存储，比 `vector<bool>` 更高效（不涉及堆分配）。
- `count()`、`any()`、`all()`、`none()` 是常用状态查询；`set(i)`/`reset(i)`/`flip(i)` 修改单个 bit。
- C++20 `<bit>` 提供了硬件指令级的 bit 操作：`popcount`（POPCNT 指令）、`countl_zero`（BSR/CLZ 指令），性能远优于手写循环。
- `std::has_single_bit(x)` 等价于 `x != 0 && (x & (x-1)) == 0`（2 的幂检测）。
- 布隆过滤器的核心：多个哈希函数对应多个 bit，空间效率高，但有假阳性（false positive），无假阴性。

---

### 题目 72：std::map::extract 与节点转移

```cpp
#include <iostream>
#include <map>
#include <set>
#include <string>

int main() {
    std::map<int, std::string> src{{1,"a"},{2,"b"},{3,"c"},{4,"d"}};
    std::map<int, std::string> dst{{2,"B"},{5,"e"}};

    // extract：从容器中取出节点，不拷贝/移动 value
    auto node = src.extract(2);       // 取出 key=2 的节点
    std::cout << node.key()   << "\n";  // 2
    std::cout << node.mapped()<< "\n";  // b

    // 修改 key（这是 extract 的独特能力，普通 erase+insert 无法原地改 key）
    node.key() = 20;
    dst.insert(std::move(node));      // 插入到 dst，key=20

    for (auto& [k,v] : dst) std::cout << k << ":" << v << " ";
    // 2:B 5:e 20:b
    std::cout << "\n";

    // merge：批量从 src 移动到 dst（冲突的留在 src）
    std::map<int,std::string> m1{{1,"x"},{3,"y"},{5,"z"}};
    std::map<int,std::string> m2{{3,"Y"},{7,"w"}};
    m2.merge(m1);   // key=3 冲突，留在 m1；key=1,5 移到 m2

    std::cout << "m1 remaining: ";
    for (auto& [k,v] : m1) std::cout << k << ":" << v << " ";
    std::cout << "\n";  // 3:y

    std::cout << "m2 after merge: ";
    for (auto& [k,v] : m2) std::cout << k << ":" << v << " ";
    std::cout << "\n";  // 1:x 3:Y 5:z 7:w

    // set 同样支持 extract/merge
    std::set<int> s1{1,2,3}, s2{3,4,5};
    s1.merge(s2);   // key=3 冲突，留在 s2
    for (int x : s1) std::cout << x << " ";  // 1 2 3 4 5
    std::cout << "\n";
}
```

**解析**：
- `extract(key)`（C++17）从关联容器取出节点句柄，取出过程中**不拷贝、不移动**存储的 value，O(log n)。
- 节点句柄允许修改 `key()`（map 场景），这在 C++17 前只能 erase + 重新 insert（会导致 value 拷贝/移动）。
- `merge(source)`（C++17）将 source 中不冲突的节点批量转移到 this，O(n log n)，节点内部 `std::string` 等对象不拷贝。
- 节点句柄管理资源，析构时若未 insert 则自动释放；insert 失败（key 冲突）时节点句柄保留资源（`insert_return_type.node` 中）。
- 同类型容器间（`map<K,V>` 互相，或 `map<K,V>` 到 `multimap<K,V>`）均可 merge。

---

### 题目 73：std::tuple 的 apply 与结构化绑定进阶

```cpp
#include <iostream>
#include <tuple>
#include <string>
#include <functional>
#include <type_traits>

// tuple_for_each：对每个元素调用 f
template<typename Tuple, typename F, std::size_t... Is>
void tuple_for_each_impl(Tuple&& t, F&& f, std::index_sequence<Is...>) {
    (f(std::get<Is>(std::forward<Tuple>(t))), ...);
}

template<typename Tuple, typename F>
void tuple_for_each(Tuple&& t, F&& f) {
    constexpr std::size_t N = std::tuple_size_v<std::decay_t<Tuple>>;
    tuple_for_each_impl(std::forward<Tuple>(t), std::forward<F>(f),
                        std::make_index_sequence<N>{});
}

// tuple_transform：映射每个元素
template<typename Tuple, typename F, std::size_t... Is>
auto tuple_transform_impl(Tuple&& t, F&& f, std::index_sequence<Is...>) {
    return std::make_tuple(f(std::get<Is>(std::forward<Tuple>(t)))...);
}

template<typename Tuple, typename F>
auto tuple_transform(Tuple&& t, F&& f) {
    constexpr std::size_t N = std::tuple_size_v<std::decay_t<Tuple>>;
    return tuple_transform_impl(std::forward<Tuple>(t), std::forward<F>(f),
                                std::make_index_sequence<N>{});
}

int main() {
    auto t = std::make_tuple(1, 3.14, std::string("hi"), true);

    // 打印所有元素（不同类型）
    tuple_for_each(t, [](const auto& x){ std::cout << x << " "; });
    std::cout << "\n";  // 1 3.14 hi 1

    // apply：展开 tuple 作为函数参数
    auto result = std::apply([](int a, double b, std::string s, bool c) {
        return std::to_string(a) + " " + std::to_string(b) + " " + s + " " + (c?"T":"F");
    }, t);
    std::cout << result << "\n";

    // 结构化绑定绑定 tuple
    auto [i, d, s, b] = t;
    std::cout << i << " " << d << " " << s << " " << b << "\n";

    // 结构化绑定绑定引用：修改原 tuple
    auto& [ri, rd, rs, rb] = t;
    rs = "world";
    std::cout << std::get<2>(t) << "\n";  // world

    // tuple_cat：连接多个 tuple
    auto t1 = std::make_tuple(1, 2);
    auto t2 = std::make_tuple(3.0, "four");
    auto t3 = std::tuple_cat(t1, t2);
    static_assert(std::tuple_size_v<decltype(t3)> == 4);
}
```

**解析**：
- `std::apply(f, tuple)` 将 tuple 展开为参数列表调用 `f`，C++17 标准库提供，内部用 `index_sequence` 实现。
- `index_sequence` + 折叠表达式是处理 tuple 的核心工具：`(f(std::get<Is>(t)), ...)` 对每个 Is 调用 f。
- 结构化绑定到 `auto&` 时绑定引用，修改绑定名会修改原 tuple 成员——可替代手写 `std::get<N>(t) = ...`。
- `std::tuple_cat` 连接任意多个 tuple，返回类型在编译期推导（所有参数类型的拼接）。
- tuple 可用于**多返回值**（替代 struct 的轻量方案）和**泛型算法**（编译期遍历异构集合）。

---

### 题目 74：std::chrono 的类型安全时间

```cpp
#include <iostream>
#include <chrono>
#include <thread>
#include <ratio>
#include <ctime>

using namespace std::chrono;
using namespace std::chrono_literals;

// 自定义 duration
using frames_per_second = duration<double, std::ratio<1, 60>>;  // 1/60 秒/帧

void type_safety() {
    auto t1 = 100ms;
    auto t2 = 1s;
    auto t3 = t1 + t2;                 // 1100ms（公共单位）
    std::cout << t3.count() << "ms\n"; // 1100

    // 隐式丢精度是编译错误
    // milliseconds ms = 1s;           // 错误：需要显式 cast
    milliseconds ms = duration_cast<milliseconds>(1s);
    std::cout << ms.count() << "\n";   // 1000

    // 自定义帧时间
    frames_per_second frame_time = 16.67ms;  // 约 1/60s
    std::cout << frame_time.count() << " frames\n";  // ~1.0

    // 时间点比较
    auto now = steady_clock::now();
    std::this_thread::sleep_for(10ms);
    auto after = steady_clock::now();
    auto diff = duration_cast<microseconds>(after - now);
    std::cout << "slept ~" << diff.count() << "us\n";  // >=10000us
}

void wall_clock() {
    system_clock::time_point now = system_clock::now();
    auto tt = system_clock::to_time_t(now);
    std::cout << std::ctime(&tt);

    // time_since_epoch
    auto epoch = now.time_since_epoch();
    auto seconds_since_epoch = duration_cast<seconds>(epoch);
    std::cout << "Unix ts: " << seconds_since_epoch.count() << "\n";
}

int main() {
    type_safety();
    wall_clock();
}
```

**解析**：
- `std::chrono` 用强类型 `duration<Rep, Period>` 表示时间，避免了历史上 `int ms = 1; int us = 1000*ms;` 的单位混用。
- `duration_cast` 是显式精度截断；从精度低到高（ms→ns）可以隐式转换，从高到低（ns→ms）必须显式 cast 防止无意丢精度。
- `steady_clock`（单调）用于测量时间间隔；`system_clock`（挂钟）用于获取绝对时间（可跳变，如 NTP 同步）；`high_resolution_clock` 通常是二者之一。
- 自定义 `duration` 用 `std::ratio<N,D>` 指定周期：`ratio<1,60>` 表示每单位为 1/60 秒。
- C++20 新增 `std::chrono::year_month_day`、`time_zone`、`zoned_time` 等日历类型，可处理时区。

---

### 题目 75：std::atomic_ref 与 lock-free 位域操作

```cpp
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>
#include <numeric>

// 对普通整数提供原子操作（不改变数据结构）
struct Statistics {
    long long total_requests = 0;
    long long error_count    = 0;
    long long latency_sum    = 0;
};

void update(Statistics& stats, bool error, long long latency) {
    // atomic_ref：对已有对象原子操作，无需 atomic 成员
    std::atomic_ref<long long> ref_req(stats.total_requests);
    std::atomic_ref<long long> ref_err(stats.error_count);
    std::atomic_ref<long long> ref_lat(stats.latency_sum);

    ref_req.fetch_add(1, std::memory_order_relaxed);
    if (error) ref_err.fetch_add(1, std::memory_order_relaxed);
    ref_lat.fetch_add(latency, std::memory_order_relaxed);
}

int main() {
    Statistics stats{};
    constexpr int N = 10000;

    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&stats, i](){
            for (int j = 0; j < N; ++j)
                update(stats, j % 10 == 0, j % 100);
        });
    }
    for (auto& t : threads) t.join();

    // 最终读取（确保所有写入可见）
    std::atomic_thread_fence(std::memory_order_acquire);
    std::cout << "requests: " << stats.total_requests << "\n"; // 40000
    std::cout << "errors:   " << stats.error_count    << "\n"; // 4000
    std::cout << "avg lat:  "
              << (stats.total_requests ? stats.latency_sum / stats.total_requests : 0)
              << "\n";

    // atomic_ref 要求对象按 required_alignment 对齐
    static_assert(alignof(long long) >= sizeof(long long));
    // std::atomic_ref<long long>::required_alignment == sizeof(long long) on most platforms
}
```

**解析**：
- `std::atomic_ref<T>`（C++20）对**已存在对象**提供原子操作，不需要对象是 `atomic<T>` 成员——可用于 C 结构体、序列化缓冲区。
- `atomic_ref` 不持有所有权，必须保证被引用对象的生命周期长于 `atomic_ref`。
- 被引用对象必须满足 `atomic_ref<T>::required_alignment`（通常等于 `sizeof(T)`），否则行为未定义。
- `memory_order_relaxed` 只保证操作原子性（无 torn read/write），不建立 happens-before；最终用 `atomic_thread_fence(acquire)` 确保全部写入可见。
- **使用场景**：多线程更新 POD 数据结构中的特定字段，而整个结构体不需要全部是原子的。

---

## 第八章：并发与无锁编程（题目 76-100）

---

### 题目 76：std::mutex 的 RAII 与死锁预防

```cpp
#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>
#include <vector>

std::mutex mu1, mu2;
int shared_a = 0, shared_b = 0;

// 死锁示例（注释掉不运行）
void deadlock_demo() {
    // thread1: lock mu1 -> lock mu2
    // thread2: lock mu2 -> lock mu1  => 死锁
}

// 解决方案1：std::scoped_lock（C++17）同时锁多个
void transfer(int& from, int& to, int amount) {
    std::scoped_lock lock(mu1, mu2);  // 原子性获取两个锁，内部用 std::lock 算法
    from -= amount;
    to   += amount;
}

// 解决方案2：统一排序（按地址）
void transfer_ordered(int& from, int& to, int amount,
                      std::mutex& mf, std::mutex& mt) {
    std::mutex* m_lo = &mf < &mt ? &mf : &mt;
    std::mutex* m_hi = &mf < &mt ? &mt : &mf;
    std::lock_guard<std::mutex> l1(*m_lo);
    std::lock_guard<std::mutex> l2(*m_hi);
    from -= amount;
    to   += amount;
}

// unique_lock 的条件变量配合
std::mutex cvm;
std::condition_variable cv;
bool ready = false;
int produced_value = 0;

void producer() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    {
        std::lock_guard lock(cvm);
        produced_value = 42;
        ready = true;
    }
    cv.notify_one();  // 通知消费者
}

void consumer() {
    std::unique_lock lock(cvm);
    // wait：先解锁，被唤醒后重新加锁，再检查谓词
    cv.wait(lock, []{ return ready; });  // 防虚假唤醒
    std::cout << "consumed: " << produced_value << "\n";
}

int main() {
    // transfer 测试
    int a = 1000, b = 0;
    std::vector<std::thread> ts;
    for (int i = 0; i < 10; ++i)
        ts.emplace_back([&]{ transfer(a, b, 10); });
    for (auto& t : ts) t.join();
    std::cout << "a=" << a << " b=" << b << "\n";  // a=900 b=100

    // 条件变量测试
    std::thread tp(producer), tc(consumer);
    tp.join(); tc.join();
}
```

**解析**：
- 死锁必要条件：循环等待。打破它的方法：全局锁顺序（按地址/id 排序）、`std::scoped_lock` 使用内部无死锁算法（`std::lock`）。
- `std::scoped_lock`（C++17）替代旧的 `std::lock` + 两个 `std::adopt_lock` 的三行写法，更简洁且异常安全。
- `condition_variable::wait(lock, pred)` 内部：释放锁 → 阻塞 → 被唤醒 → 重新加锁 → 检查 pred（若假则继续等待），消除虚假唤醒。
- `notify_one` 唤醒一个等待线程；`notify_all` 唤醒所有。广播适用于条件改变影响多个等待者的情况。
- `condition_variable` 必须与 `unique_lock<mutex>` 配合（`lock_guard` 不支持 `wait`，因为 `wait` 需要暂时释放锁）。

---

### 题目 77：std::atomic 的 CAS 循环与自旋锁

```cpp
#include <atomic>
#include <thread>
#include <iostream>
#include <vector>

// 自旋锁实现
class SpinLock {
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
public:
    void lock() noexcept {
        while (flag_.test_and_set(std::memory_order_acquire)) {
            // PAUSE 提示 CPU 进入等待状态，降低功耗、减少流水线冲突
            #if defined(__x86_64__) || defined(_M_X64)
                _mm_pause();
            #elif defined(__aarch64__)
                __asm__ volatile("yield" ::: "memory");
            #endif
        }
    }
    void unlock() noexcept {
        flag_.clear(std::memory_order_release);
    }
};

// CAS 实现无锁计数器
struct LockFreeCounter {
    std::atomic<int> val{0};

    int fetch_increment() {
        int expected = val.load(std::memory_order_relaxed);
        while (!val.compare_exchange_weak(
                expected,
                expected + 1,
                std::memory_order_acq_rel,
                std::memory_order_relaxed))
        {}  // expected 被 CAS 自动更新为实际值
        return expected;  // 返回旧值
    }
};

SpinLock sl;
int protected_val = 0;

int main() {
    // 自旋锁保护共享变量
    std::vector<std::thread> ts;
    for (int i = 0; i < 4; ++i) {
        ts.emplace_back([](){
            for (int j = 0; j < 10000; ++j) {
                sl.lock();
                ++protected_val;
                sl.unlock();
            }
        });
    }
    for (auto& t : ts) t.join();
    std::cout << "spinlock result: " << protected_val << "\n";  // 40000

    // 无锁计数器
    LockFreeCounter counter;
    std::vector<std::thread> ts2;
    for (int i = 0; i < 4; ++i)
        ts2.emplace_back([&](){ for (int j=0; j<10000; ++j) counter.fetch_increment(); });
    for (auto& t : ts2) t.join();
    std::cout << "lockfree counter: " << counter.val << "\n";   // 40000
}
```

**解析**：
- `atomic_flag::test_and_set(acquire)` 原子地将 flag 置 1 并返回旧值；若旧值为 0（锁空闲），当前线程获得锁。
- `memory_order_acquire` 加锁：保证之后所有读写不被重排到锁前；`release` 解锁：保证之前所有读写不被重排到锁后。
- `compare_exchange_weak` 允许虚假失败（spurious failure，在 ARM 等 LL/SC 架构上更高效），必须在循环中使用；`strong` 不允许虚假失败。
- CAS 失败时 `expected` 被自动更新为实际值，下一轮循环用新的 `expected` 重试——避免手动 `load`。
- 自旋锁适合**持锁时间极短**的临界区；持锁时间较长时应用 `std::mutex`（通过 futex 让出 CPU）。

---

### 题目 78：memory_order 的六种语义与实战

```cpp
#include <atomic>
#include <thread>
#include <iostream>
#include <cassert>

// 场景1：release-acquire 消息传递
std::atomic<bool> ready{false};
int data = 0;

void writer() {
    data = 42;                                    // (A) 普通写
    ready.store(true, std::memory_order_release); // (B) release
}
void reader() {
    while (!ready.load(std::memory_order_acquire));// (C) acquire
    assert(data == 42);                            // (D) 保证看到 A
}

// 场景2：relaxed 只保证原子性
std::atomic<int> cnt{0};
void relaxed_inc() {
    for (int i = 0; i < 100000; ++i)
        cnt.fetch_add(1, std::memory_order_relaxed);  // 原子，但不保证顺序
}

// 场景3：seq_cst 全局顺序
std::atomic<int> x2{0}, y2{0};
int rx, ry;
void seqcst_t1() { x2.store(1); ry = y2.load(); }
void seqcst_t2() { y2.store(1); rx = x2.load(); }
// seq_cst 保证：不可能 rx==0 && ry==0

// 场景4：release sequence（允许中间 relaxed 操作）
std::atomic<int> producer_flag{0};
int payload = 0;

void produce() {
    payload = 100;
    producer_flag.store(1, std::memory_order_release);  // release
}
void consume() {
    // 通过 RMW 接力 release sequence
    while (producer_flag.load(std::memory_order_acquire) == 0);
    assert(payload == 100);
}

int main() {
    std::thread w(writer), r(reader);
    w.join(); r.join();

    std::vector<std::thread> ts;
    for (int i = 0; i < 4; ++i)
        ts.emplace_back(relaxed_inc);
    for (auto& t : ts) t.join();
    std::cout << "cnt: " << cnt << "\n";  // 400000（原子，值正确）

    std::cout << "memory_order summary:\n";
    std::cout << "  relaxed : only atomicity\n";
    std::cout << "  consume : data-dependency acquire (deprecated)\n";
    std::cout << "  acquire : no reorder after this load\n";
    std::cout << "  release : no reorder before this store\n";
    std::cout << "  acq_rel : acquire + release (RMW)\n";
    std::cout << "  seq_cst : total order (MFENCE on x86)\n";
}
```

**解析**：
- **release-acquire 配对**：release store 之前的所有写入，对 acquire load 之后的所有读取可见——是 C++ 线程间通信的基础。
- **relaxed**：只保证操作本身原子（不会看到撕裂的值），不阻止重排，用于纯计数（不依赖值的可见性顺序）。
- **seq_cst**（默认）：最强语义，全局唯一线性顺序；在 x86 上 store 需要 MFENCE，比 release 慢；ARM 上 store 需要更多屏障。
- **acq_rel** 用于读-改-写（RMW）操作：对读取端表现为 acquire，对写入端表现为 release。
- **错误1**：用 relaxed 做消息传递（data 写入与 ready 发布无同步）；**错误2**：混淆 release store 与 acquire load 方向（store 不能 acquire，load 不能 release）。

---

### 题目 79：无锁单生产者单消费者队列（SPSC）

```cpp
#include <atomic>
#include <array>
#include <optional>
#include <iostream>
#include <thread>
#include <numeric>

template<typename T, std::size_t N>
class SPSCQueue {
    static_assert((N & (N-1)) == 0, "N must be power of 2");
    static constexpr std::size_t MASK = N - 1;

    std::array<T, N> buf_{};
    // head 和 tail 放在不同缓存行，避免 false sharing
    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};

public:
    // 生产者调用（单线程）
    bool push(T val) noexcept {
        const std::size_t h = head_.load(std::memory_order_relaxed);
        const std::size_t next = (h + 1) & MASK;
        if (next == tail_.load(std::memory_order_acquire))
            return false;  // 队列满
        buf_[h] = std::move(val);
        head_.store(next, std::memory_order_release);
        return true;
    }

    // 消费者调用（单线程）
    std::optional<T> pop() noexcept {
        const std::size_t t = tail_.load(std::memory_order_relaxed);
        if (t == head_.load(std::memory_order_acquire))
            return std::nullopt;  // 队列空
        T val = std::move(buf_[t]);
        tail_.store((t + 1) & MASK, std::memory_order_release);
        return val;
    }

    bool empty() const noexcept {
        return tail_.load(std::memory_order_acquire) ==
               head_.load(std::memory_order_acquire);
    }
};

int main() {
    SPSCQueue<int, 1024> q;
    constexpr int N = 100000;
    long long sum_produced = 0, sum_consumed = 0;

    std::thread producer([&](){
        for (int i = 1; i <= N; ++i) {
            while (!q.push(i)) std::this_thread::yield();
            sum_produced += i;
        }
    });

    std::thread consumer([&](){
        int received = 0;
        while (received < N) {
            if (auto v = q.pop()) {
                sum_consumed += *v;
                ++received;
            }
        }
    });

    producer.join();
    consumer.join();
    std::cout << "produced=" << sum_produced << " consumed=" << sum_consumed << "\n";
    std::cout << "match: " << (sum_produced == sum_consumed) << "\n";  // 1
}
```

**解析**：
- SPSC（Single Producer Single Consumer）是最简单的无锁队列，只需一对 acquire/release 即可保证正确性。
- `head_` 由生产者写、消费者读；`tail_` 由消费者写、生产者读——分离到不同缓存行（`alignas(64)`）消除 false sharing。
- `push` 中：`head_.load(relaxed)` 生产者读自己的 head（不需要同步）；`tail_.load(acquire)` 获取消费者最新写入的 tail。
- `pop` 中：`tail_.load(relaxed)` 消费者读自己的 tail；`head_.load(acquire)` 获取生产者最新写入的 head。
- 环形缓冲区大小**必须是 2 的幂**：`& MASK` 比 `% N` 快一个数量级（避免除法）。

---

### 题目 80：epoch-based reclamation（内存安全回收）

```cpp
#include <atomic>
#include <thread>
#include <vector>
#include <iostream>

// 简化版 epoch-based reclamation 框架
// 三个 epoch：0, 1, 2（循环）
// 每个线程记录自己"进入"时的 epoch
// GC 线程回收"所有线程都已离开"的 epoch 的垃圾

struct EBRManager {
    std::atomic<int> global_epoch{0};
    struct ThreadState {
        std::atomic<int> local_epoch{-1};  // -1 = 不在临界区
        std::vector<void*> retire[3];       // 每 epoch 的待回收列表
    };

    static constexpr int EPOCHS = 3;

    void enter(ThreadState& ts) {
        ts.local_epoch.store(global_epoch.load(std::memory_order_acquire),
                              std::memory_order_release);
    }

    void exit(ThreadState& ts) {
        ts.local_epoch.store(-1, std::memory_order_release);
    }

    // 尝试推进 epoch 并回收
    void try_advance(std::vector<ThreadState*>& all_threads,
                     ThreadState& ts) {
        int epoch = global_epoch.load(std::memory_order_relaxed);
        // 检查所有线程是否都已离开旧 epoch
        for (auto* t : all_threads)
            if (t->local_epoch.load() != -1 && t->local_epoch.load() < epoch)
                return;  // 有线程还在旧 epoch

        // 可以推进 epoch
        int next_epoch = (epoch + 1) % EPOCHS;
        global_epoch.store(next_epoch, std::memory_order_release);

        // 回收 (epoch - 2) 的垃圾
        int safe_epoch = (epoch + 1) % EPOCHS;
        for (auto* ptr : ts.retire[safe_epoch]) {
            delete static_cast<int*>(ptr);
        }
        ts.retire[safe_epoch].clear();
    }
};

int main() {
    std::cout << "Epoch-Based Reclamation:\n";
    std::cout << "  enter() -> record current epoch\n";
    std::cout << "  exit()  -> mark as not in critical section\n";
    std::cout << "  retire(ptr) -> add to current epoch's retire list\n";
    std::cout << "  advance() -> if all threads past epoch N, reclaim epoch N-2\n";
    std::cout << "\nThis allows lock-free data structures to safely reclaim\n";
    std::cout << "memory without hazard pointers or ABA counters.\n";
}
```

**解析**：
- EBR 是无锁数据结构中解决安全内存回收的三大方案之一（另两个：Hazard Pointer、Reference Counting）。
- 核心思路：全局 epoch 周期推进；线程进入临界区记录当前 epoch；退出后标记 -1；当所有线程都离开了 epoch E，说明没有人在访问 E 期间 retire 的指针，可以安全 `delete`。
- EBR 的优点：开销极低（只需 atomic load/store），无需 per-pointer 操作；缺点：一个线程停止推进会阻塞全局回收（线程暂停问题）。
- 实际使用：Folly/libcds 等库提供完整实现，一般不建议手写。
- 对比 Hazard Pointer：EBR 批量回收，HP 精确回收；EBR 延迟更大，HP 每次操作需要 HP 扫描。

---

### 题目 81：std::latch 与 std::barrier 的分阶段同步

```cpp
#include <latch>
#include <barrier>
#include <thread>
#include <vector>
#include <iostream>
#include <numeric>
#include <mutex>

std::mutex print_mu;
void safe_print(const std::string& s) {
    std::lock_guard lock(print_mu);
    std::cout << s;
}

int main() {
    // --- latch：一次性倒计数，所有线程等待计数归零 ---
    {
        constexpr int N = 5;
        std::latch start_gate(1);     // 主线程倒一次，同时释放所有 worker
        std::latch finish_line(N);    // N 个 worker 各倒一次

        std::vector<std::thread> workers;
        for (int i = 0; i < N; ++i) {
            workers.emplace_back([&, i](){
                start_gate.wait();    // 等起跑信号
                safe_print("Worker " + std::to_string(i) + " started\n");
                finish_line.count_down();
            });
        }
        safe_print("Ready... Go!\n");
        start_gate.count_down();      // 同时释放所有 worker
        finish_line.wait();           // 等所有 worker 完成
        for (auto& t : workers) t.join();
        safe_print("All workers done\n");
    }

    // --- barrier：可重用的阶段同步 ---
    {
        constexpr int N = 4;
        std::vector<int> data(N, 0);
        int phase = 0;

        auto on_completion = [&]() noexcept {
            // 每轮结束时在这里汇总（单线程执行，无需锁）
            int sum = std::accumulate(data.begin(), data.end(), 0);
            safe_print("Phase " + std::to_string(++phase) + " sum=" + std::to_string(sum) + "\n");
        };

        std::barrier sync(N, on_completion);
        std::vector<std::thread> workers;

        for (int i = 0; i < N; ++i) {
            workers.emplace_back([&, i](){
                for (int round = 0; round < 3; ++round) {
                    data[i] = (round + 1) * (i + 1);  // 生产数据
                    sync.arrive_and_wait();             // 同步点：等所有线程完成本轮
                    // 此时所有线程的数据已就绪，可安全读取
                }
            });
        }
        for (auto& t : workers) t.join();
    }
}
```

**解析**：
- `std::latch`（C++20）：一次性倒计数 barrier，`count_down()` 减 1，`wait()` 阻塞直到 0，不可重置。
- `std::barrier`（C++20）：可重用的阶段同步点，所有线程 `arrive_and_wait()` 后统一放行；每轮结束执行 `on_completion` 回调（单线程、noexcept 要求）。
- latch 适合"一次性"事件（如等待初始化完成）；barrier 适合"多轮"阶段计算（如并行排序的分治合并）。
- `arrive_and_drop()`：线程提前退出 barrier 组（减少计数，后续轮不再参与）。
- 相比 `condition_variable` 手写屏障：latch/barrier 更难误用（无虚假唤醒问题、不需要 predicate）。

---

### 题目 82：std::jthread 与 stop_token 协作取消

```cpp
#include <thread>
#include <stop_token>
#include <iostream>
#include <chrono>
#include <condition_variable>
#include <mutex>

using namespace std::chrono_literals;

// 工作线程：周期性执行，支持取消
void periodic_worker(std::stop_token st, int id, std::chrono::milliseconds interval) {
    std::cout << "Worker " << id << " started\n";
    while (!st.stop_requested()) {
        std::cout << "Worker " << id << " working\n";
        // 使用 stop_token 的 wait：被取消时立即唤醒，不需要 sleep 完整周期
        std::this_thread::sleep_for(interval);
    }
    std::cout << "Worker " << id << " stopped gracefully\n";
}

// 条件变量与 stop_token 配合
void cv_worker(std::stop_token st, std::condition_variable_any& cv,
               std::mutex& mu, bool& ready) {
    std::unique_lock lock(mu);
    // 使用 stop_token 版 wait：stop 时也会唤醒
    cv.wait(lock, st, [&ready]{ return ready; });
    if (st.stop_requested()) {
        std::cout << "CV worker cancelled\n";
    } else {
        std::cout << "CV worker got signal\n";
    }
}

int main() {
    // jthread 析构时自动 request_stop + join
    {
        std::jthread jt1(periodic_worker, 1, 100ms);
        std::jthread jt2(periodic_worker, 2, 150ms);
        std::this_thread::sleep_for(350ms);
        // jt1, jt2 离开作用域 → 自动 request_stop() + join()
    }

    // 手动取消
    std::jthread jt3(periodic_worker, 3, 50ms);
    std::this_thread::sleep_for(200ms);
    jt3.request_stop();   // 发送取消信号
    jt3.join();           // 等待线程退出

    // condition_variable_any 与 stop_token
    std::mutex mu;
    std::condition_variable_any cv;
    bool ready = false;
    std::jthread cv_jt(cv_worker, std::ref(cv), std::ref(mu), std::ref(ready));
    std::this_thread::sleep_for(100ms);
    cv_jt.request_stop();  // 取消而不发 signal
    cv_jt.join();
}
```

**解析**：
- `std::jthread`（C++20）是 `std::thread` 的 RAII 包装：析构时自动调用 `request_stop()` + `join()`，消除了 `thread` 析构时仍 joinable 导致 `terminate` 的问题。
- `std::stop_token` 是轻量只读句柄，`stop_requested()` 原子地检查是否有取消请求；`std::stop_source` 发出取消信号。
- `std::condition_variable_any::wait(lock, stop_token, pred)` 三参数版（C++20）：stop 时也会唤醒，避免线程无限等待。
- `stop_callback`：在 stop 请求发出时异步调用回调，用于唤醒阻塞在 IO 或 sleep 的线程。
- 协作取消比强制 `pthread_cancel` 更安全：线程自己决定何时退出，资源清理有保障。

---

### 题目 83：std::async 的陷阱与 launch 策略

```cpp
#include <future>
#include <iostream>
#include <thread>
#include <chrono>
#include <stdexcept>

int compute(int x) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (x < 0) throw std::invalid_argument("negative");
    return x * x;
}

int main() {
    // 陷阱1：默认 launch（async|deferred），不保证新线程
    {
        auto f = std::async(compute, 5);  // 可能在当前线程延迟执行
        // 若是 deferred，f.get() 前工作未开始
        std::cout << f.get() << "\n";  // 25
    }

    // 强制新线程：std::launch::async
    {
        auto f = std::async(std::launch::async, compute, 6);
        // 无论如何，已在另一个线程运行
        std::cout << f.get() << "\n";  // 36
    }

    // 陷阱2：async 返回的 future 析构时阻塞（launch::async 时）
    {
        auto t0 = std::chrono::steady_clock::now();
        {
            auto f = std::async(std::launch::async, [](){
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            });
            // f 析构 → 等待 200ms（若不 get/wait）
        }
        auto elapsed = std::chrono::steady_clock::now() - t0;
        std::cout << "blocked ~"
                  << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
                  << "ms\n";  // ~200ms
    }

    // 异常传播
    {
        auto f = std::async(std::launch::async, compute, -1);
        try {
            f.get();  // 重新抛出子线程的异常
        } catch (const std::invalid_argument& e) {
            std::cout << "exception: " << e.what() << "\n";
        }
    }

    // 正确的"fire and forget"：detach 或 packaged_task
    {
        std::packaged_task<int(int)> task(compute);
        auto fut = task.get_future();
        std::thread(std::move(task), 7).detach();
        std::cout << fut.get() << "\n";  // 49
    }
}
```

**解析**：
- `std::async` 默认策略 `async|deferred`：实现可自由选择新线程或延迟执行——不能保证**立即**开新线程。
- **最重要陷阱**：`std::async(launch::async, ...)` 返回的 `future` 在析构时**阻塞等待任务完成**（不同于 `thread::detach`）——将 future 赋值到变量是必要的。
- `launch::deferred`：首次调用 `get()`/`wait()` 时才在调用线程执行（惰性求值），适合按需计算。
- 异常在子线程中被捕获并存储，在主线程 `get()` 时重新抛出（通过 `exception_ptr` 跨线程传递）。
- `packaged_task` + `detach` 实现真正的 fire-and-forget，但 future 超出作用域后无法再获取结果。

---

### 题目 84：std::shared_future 的广播模式

```cpp
#include <future>
#include <thread>
#include <vector>
#include <iostream>
#include <chrono>

// 使用 shared_future 广播配置到多个工作线程
void demo_broadcast() {
    std::promise<std::string> config_promise;
    // shared_future 可被多个线程 get()
    std::shared_future<std::string> config = config_promise.get_future().share();

    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([config, i]() mutable {
            // 每个线程独立等待，共享结果
            std::string cfg = config.get();  // 可多次调用
            std::cout << "Worker " << i << " got config: " << cfg << "\n";
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    config_promise.set_value("hostname=server1;port=8080");  // 广播

    for (auto& t : workers) t.join();
}

// future 链式组合（手动实现 then）
template<typename T, typename F>
auto then(std::future<T> fut, F func) -> std::future<std::invoke_result_t<F,T>> {
    return std::async(std::launch::async, [f=std::move(fut), func]() mutable {
        return func(f.get());
    });
}

int main() {
    demo_broadcast();

    // 链式异步计算
    auto step1 = std::async(std::launch::async, []{ return 10; });
    auto step2 = then(std::move(step1), [](int x){ return x * 2; });
    auto step3 = then(std::move(step2), [](int x){ return x + 5; });
    std::cout << "chain result: " << step3.get() << "\n";  // 25
}
```

**解析**：
- `future::share()` 返回 `shared_future`，可被**多个线程**并发 `get()`，而 `future::get()` 只能调用一次（之后变 invalid）。
- `shared_future` 拷贝后多个副本共享同一 shared state；`future` 只能移动（所有权唯一）。
- `then` 的手动实现：`async(launch::async, lambda)` 内部等待上一步，完成后执行下一步——这是 C++ 缺少 `future::then` 的常见补救方案（C++ executor 提案仍在讨论中）。
- `promise::set_exception(current_exception())` 将异常跨线程传递；`set_value_at_thread_exit` 确保在线程退出时才设值（所有 TLS 析构完成后）。
- C++23 的 `std::execution` sender/receiver 模型是 future/promise 的演进，提供更强大的异步组合。

---

### 题目 85：lock-free 无锁栈（Michael-Scott 风格）

```cpp
#include <atomic>
#include <memory>
#include <optional>
#include <iostream>
#include <thread>
#include <vector>

template<typename T>
class LockFreeStack {
    struct Node {
        T data;
        Node* next = nullptr;
        explicit Node(T d) : data(std::move(d)) {}
    };
    std::atomic<Node*> head_{nullptr};

public:
    void push(T val) {
        Node* n = new Node(std::move(val));
        n->next = head_.load(std::memory_order_relaxed);
        // CAS：若 head 未变，将新节点设为 head
        while (!head_.compare_exchange_weak(
                n->next, n,
                std::memory_order_release,
                std::memory_order_relaxed));
    }

    std::optional<T> pop() {
        Node* old = head_.load(std::memory_order_acquire);
        while (old) {
            if (head_.compare_exchange_weak(
                    old, old->next,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire)) {
                T val = std::move(old->data);
                // 安全问题：delete old 时其他线程可能仍在访问 old->next
                // 生产代码需要 EBR / hazard pointer
                delete old;
                return val;
            }
            // CAS 失败：old 已被更新为实际 head，下次重试
        }
        return std::nullopt;
    }

    ~LockFreeStack() {
        while (pop()) {}
    }
};

int main() {
    LockFreeStack<int> s;
    constexpr int N = 10000;

    std::vector<std::thread> producers, consumers;
    std::atomic<int> produced{0}, consumed{0};

    for (int i = 0; i < 4; ++i)
        producers.emplace_back([&](){
            for (int j = 0; j < N; ++j) {
                s.push(j);
                produced.fetch_add(1, std::memory_order_relaxed);
            }
        });

    for (int i = 0; i < 4; ++i)
        consumers.emplace_back([&](){
            while (consumed.load(std::memory_order_relaxed) < 4*N) {
                if (auto v = s.pop())
                    consumed.fetch_add(1, std::memory_order_relaxed);
            }
        });

    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();
    std::cout << "produced=" << produced << " consumed=" << consumed << "\n";
}
```

**解析**：
- push 中 `compare_exchange_weak` 的 `n->next` 作为 expected：CAS 失败时，`n->next` 被更新为实际 head，下一轮继续尝试——无需显式 `load`。
- pop 中的 ABA 问题：线程 A 读到 head=P，暂停；线程 B pop P 后再 push 一个新节点，碰巧地址仍为 P；A 的 CAS 成功，但 `P->next` 已不是原来的值——栈结构破坏。
- ABA 解决：带版本号的指针（tagged pointer）；`std::atomic<std::pair<Node*, uint64_t>>`（128-bit CAS）；EBR / hazard pointer。
- `delete old` 在 pop 中是简化版，生产代码必须用安全回收机制（见题目 80）。
- 无锁栈 push/pop 均 O(1) 均摊，但在高争用时 CAS 失败率高，性能可能不如带锁版本。

---

### 题目 86：读写锁（shared_mutex）与升级锁问题

```cpp
#include <shared_mutex>
#include <thread>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <chrono>

class ThreadSafeConfig {
    mutable std::shared_mutex rw_;
    std::map<std::string, std::string> data_;
    int version_ = 0;

public:
    // 读：可多线程并发
    std::string get(const std::string& key) const {
        std::shared_lock lock(rw_);
        auto it = data_.find(key);
        return it != data_.end() ? it->second : "";
    }

    // 写：独占
    void set(std::string key, std::string val) {
        std::unique_lock lock(rw_);
        data_[std::move(key)] = std::move(val);
        ++version_;
    }

    // 条件写（TOCTOU 陷阱 + 正确解法）
    bool set_if_absent(const std::string& key, std::string val) {
        // 错误写法（TOCTOU）：
        // {shared_lock: check} -> {unique_lock: set}
        // 两个锁之间有空隙，另一线程可能插入

        // 正确写法：直接用 unique_lock
        std::unique_lock lock(rw_);
        if (data_.count(key)) return false;
        data_[key] = std::move(val);
        ++version_;
        return true;
    }

    int version() const {
        std::shared_lock lock(rw_);
        return version_;
    }
};

int main() {
    ThreadSafeConfig cfg;
    cfg.set("host", "localhost");
    cfg.set("port", "8080");

    std::vector<std::thread> readers, writers;

    for (int i = 0; i < 8; ++i)
        readers.emplace_back([&cfg](){
            for (int j = 0; j < 1000; ++j) {
                auto h = cfg.get("host");
                auto p = cfg.get("port");
                (void)(h + p);
            }
        });

    for (int i = 0; i < 2; ++i)
        writers.emplace_back([&cfg, i](){
            for (int j = 0; j < 100; ++j) {
                cfg.set("key" + std::to_string(i), std::to_string(j));
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });

    for (auto& t : readers) t.join();
    for (auto& t : writers) t.join();

    std::cout << "version: " << cfg.version() << "\n";
    std::cout << "host: " << cfg.get("host") << "\n";
}
```

**解析**：
- `std::shared_mutex` 实现读写锁：`shared_lock`（读锁）允许并发，`unique_lock`（写锁）独占——读多写少时性能优于纯 `mutex`。
- **TOCTOU（Time Of Check To Time Of Use）**：检查与操作之间的竞争窗口。`set_if_absent` 若先 shared_lock 检查再 unique_lock 写入，两锁之间另一线程可能已写入——正确做法是直接 unique_lock 一步完成。
- 标准 C++ **不支持锁升级**（shared_lock → unique_lock），必须先释放 shared_lock 再获取 unique_lock——这期间的状态变化必须重新检查。
- 读者写者饥饿：若读者持续涌入，写者可能长时间等待（写者优先策略需额外实现）。
- C++17 `shared_mutex`；C++14 `shared_timed_mutex`（额外支持 try_lock_for/until）。

---

### 题目 87：std::counting_semaphore 限流

```cpp
#include <semaphore>
#include <thread>
#include <iostream>
#include <vector>
#include <chrono>
#include <atomic>

using namespace std::chrono_literals;

// 数据库连接池：最多 3 个并发连接
class ConnectionPool {
    std::counting_semaphore<3> sem_{3};  // 初始值=最大值=3
    std::atomic<int> active_{0};

public:
    struct Connection {
        ConnectionPool* pool;
        int id;
        ~Connection() { pool->release(id); }
    };

    std::unique_ptr<Connection> acquire() {
        sem_.acquire();  // 阻塞直到有可用"令牌"
        int id = active_.fetch_add(1);
        std::cout << "Connection " << id << " acquired (active=" << active_ << ")\n";
        return std::make_unique<Connection>(Connection{this, id});
    }

    void release(int id) {
        active_.fetch_sub(1);
        std::cout << "Connection " << id << " released (active=" << active_ << ")\n";
        sem_.release();  // 归还"令牌"
    }
};

int main() {
    ConnectionPool pool;
    std::vector<std::thread> workers;

    for (int i = 0; i < 8; ++i) {
        workers.emplace_back([&pool, i](){
            auto conn = pool.acquire();  // 超过 3 个会阻塞
            std::cout << "Worker " << i << " using connection\n";
            std::this_thread::sleep_for(100ms);
            // conn 析构时自动 release
        });
    }

    for (auto& t : workers) t.join();

    // binary_semaphore：最大值 1，等价于 mutex 语义的信号量
    std::binary_semaphore bs(0);  // 初始未 release
    std::thread signaler([&bs](){
        std::this_thread::sleep_for(50ms);
        bs.release();  // signal
    });
    bs.acquire();  // wait
    std::cout << "Signal received\n";
    signaler.join();
}
```

**解析**：
- `std::counting_semaphore<LeastMaxValue>` 模板参数是最大值的**下界提示**，实际上限可更大，但设置合理值让编译器优化。
- `acquire()` 原子地将计数 -1（若为 0 则阻塞）；`release(n=1)` 原子地将计数 +n 并唤醒等待线程。
- 语义：信号量计数 = "可用资源数"，与 `mutex` 的区别是信号量可以在一个线程 release，另一个线程 acquire（mutex 通常在同一线程加解锁）。
- `binary_semaphore` 是 `counting_semaphore<1>` 的别名，常用作线程间信号（不是互斥锁的等价物）。
- C++20 引入；在此之前用 `condition_variable` + `count` 手写。

---

### 题目 88：线程池的核心实现

```cpp
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <vector>
#include <iostream>

class ThreadPool {
    std::vector<std::thread>          workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex                        mu_;
    std::condition_variable           cv_;
    bool                              stop_ = false;

public:
    explicit ThreadPool(std::size_t n) {
        for (std::size_t i = 0; i < n; ++i) {
            workers_.emplace_back([this](){
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock lock(mu_);
                        cv_.wait(lock, [this]{
                            return stop_ || !tasks_.empty();
                        });
                        if (stop_ && tasks_.empty()) return;
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    task();  // 在锁外执行，避免持锁运行用户代码
                }
            });
        }
    }

    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>
    {
        using R = std::invoke_result_t<F, Args...>;
        auto task = std::make_shared<std::packaged_task<R()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        auto fut = task->get_future();
        {
            std::lock_guard lock(mu_);
            if (stop_) throw std::runtime_error("pool stopped");
            tasks_.emplace([task]{ (*task)(); });
        }
        cv_.notify_one();
        return fut;
    }

    ~ThreadPool() {
        { std::lock_guard lock(mu_); stop_ = true; }
        cv_.notify_all();
        for (auto& w : workers_) w.join();
    }
};

int main() {
    ThreadPool pool(4);

    std::vector<std::future<int>> results;
    for (int i = 0; i < 20; ++i)
        results.push_back(pool.submit([i]{ return i * i; }));

    int sum = 0;
    for (auto& f : results) sum += f.get();
    std::cout << "sum of squares 0..19: " << sum << "\n";  // 2470

    // 提交带参数的任务
    auto f = pool.submit([](int a, int b){ return a + b; }, 10, 20);
    std::cout << "10+20=" << f.get() << "\n";  // 30
}
```

**解析**：
- 线程池核心：工作线程 `cv_.wait(pred)` 阻塞等待任务，主线程提交任务后 `notify_one` 唤醒一个工作线程。
- `packaged_task` 包装 callable，`get_future()` 获取 future；任务在 lambda 内调用 `(*task)()` 执行。
- `stop_` 置为 true 后，`notify_all()` 唤醒所有等待线程，它们发现 `stop_ && tasks_.empty()` 后退出。
- 任务在锁**外**执行（lock 已释放）：避免用户代码持锁，防止死锁。
- `submit` 用 `shared_ptr<packaged_task>` 而非直接 `packaged_task`：因为 lambda 需要拷贝（`tasks_.emplace` 存的是 `function<void()>`，需要 CopyConstructible）。

---

### 题目 89：false sharing 与 cache-line 对齐

```cpp
#include <atomic>
#include <thread>
#include <iostream>
#include <chrono>
#include <vector>

constexpr int CACHE_LINE = 64;

// 存在 false sharing：4 个计数器在同一缓存行
struct NopadCounters {
    std::atomic<long long> cnt[4] {};
};

// 消除 false sharing：每个计数器独占一个缓存行
struct alignas(CACHE_LINE) PaddedCounter {
    std::atomic<long long> val {0};
    char pad[CACHE_LINE - sizeof(std::atomic<long long>)];
};

void bench(const char* label, auto update_fn, int N) {
    auto t0 = std::chrono::high_resolution_clock::now();
    update_fn(N);
    auto t1 = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1-t0).count();
    std::cout << label << ": " << ms << "ms\n";
}

int main() {
    constexpr int N = 50'000'000;

    // 有 false sharing：4 线程各写各的计数器，但争用同一缓存行
    NopadCounters nopad{};
    bench("false sharing", [&nopad](int n){
        std::vector<std::thread> ts;
        for (int i = 0; i < 4; ++i)
            ts.emplace_back([&nopad, i, n](){
                for (int j = 0; j < n/4; ++j)
                    nopad.cnt[i].fetch_add(1, std::memory_order_relaxed);
            });
        for (auto& t : ts) t.join();
    }, N);

    // 无 false sharing
    PaddedCounter padded[4];
    bench("no false sharing", [&padded](int n){
        std::vector<std::thread> ts;
        for (int i = 0; i < 4; ++i)
            ts.emplace_back([&padded, i, n](){
                for (int j = 0; j < n/4; ++j)
                    padded[i].val.fetch_add(1, std::memory_order_relaxed);
            });
        for (auto& t : ts) t.join();
    }, N);

    // sizeof 验证
    std::cout << "NopadCounters size: " << sizeof(NopadCounters) << "\n";  // 32
    std::cout << "PaddedCounter size: " << sizeof(PaddedCounter) << "\n";  // 64
    // 性能差异通常 2-5x（取决于 CPU）
}
```

**解析**：
- **False Sharing**：多个线程写**不同变量**，但这些变量在同一缓存行（64 字节）内，导致缓存一致性协议频繁失效整行——性能大幅下降（2-10x）。
- `alignas(CACHE_LINE)` 确保每个 `PaddedCounter` 对象起始地址对齐到 64 字节，且 padding 使其大小恰好 64 字节——不同线程的计数器不共享缓存行。
- 注意：`char pad[]` 数组大小需精确计算（`CACHE_LINE - sizeof(atomic<long long>)`），若算错会导致对象大小超出或不足一个缓存行。
- C++17 `hardware_destructive_interference_size` 是标准获取缓存行大小的方式（部分编译器支持）。
- 读-读不产生 false sharing（共享缓存行只读时没有 invalidation）；写-写必须分离缓存行。

---

### 题目 90：std::memory_order 的 load/store 组合矩阵

```cpp
#include <atomic>
#include <iostream>

// 完整演示各种 memory_order 的配对规则
std::atomic<int> x{0}, y{0};

// Case 1：relaxed + relaxed = 无同步（不保证看到对方的写入）
void case1_writer() { x.store(1, std::memory_order_relaxed); }
void case1_reader() { int v = x.load(std::memory_order_relaxed); (void)v; }

// Case 2：release + acquire = 单向同步
// store(release) 之前的写入对 load(acquire) 之后的读取可见
void case2_writer() {
    y = 10;                              // (A) 普通写
    x.store(1, std::memory_order_release); // (B) release store
}
void case2_reader() {
    while (!x.load(std::memory_order_acquire)); // (C) acquire load
    // 此处 y == 10 有保证（A happens-before D）
}

// Case 3：seq_cst 全局有序（最强，所有 seq_cst 操作有全局唯一顺序）
std::atomic<bool> flag_x{false}, flag_y{false};
void case3_t1() {
    flag_x.store(true);                      // seq_cst store
    bool vy = flag_y.load();                 // seq_cst load
    std::cout << "T1 sees flag_y=" << vy << "\n";
}
void case3_t2() {
    flag_y.store(true);
    bool vx = flag_x.load();
    std::cout << "T2 sees flag_x=" << vx << "\n";
}
// seq_cst 保证：不会同时输出 false 和 false

int main() {
    // 演示 release-acquire 同步链
    std::atomic<int> a{0}, b{0};
    int data1 = 0, data2 = 0;

    std::thread t1([&](){
        data1 = 42;
        a.store(1, std::memory_order_release);
    });
    std::thread t2([&](){
        while (!a.load(std::memory_order_acquire));
        data2 = data1;                    // 可见 data1 = 42
        b.store(1, std::memory_order_release);
    });
    std::thread t3([&](){
        while (!b.load(std::memory_order_acquire));
        std::cout << "data2=" << data2 << "\n";  // 保证 42，release sequence
    });
    t1.join(); t2.join(); t3.join();

    std::cout << "\nmemory_order summary table:\n";
    std::cout << "  relaxed:  atomic only, no ordering\n";
    std::cout << "  acquire:  no reads/writes after this move before it\n";
    std::cout << "  release:  no reads/writes before this move after it\n";
    std::cout << "  acq_rel:  both (for RMW operations)\n";
    std::cout << "  seq_cst:  total global order + acq_rel\n";
}
```

**解析**：
- release-acquire 同步形成**传递链**：t1 release → t2 acquire → t2 release → t3 acquire，三个线程构成 happens-before 链，data1 的值在 t3 中可见。
- seq_cst 是最强屏障，保证所有 seq_cst 操作有**全局唯一线性顺序**（如同单线程执行），消除所有重排——在 x86 上几乎免费（TSO 架构），在 ARM 上需要 DMB/ISH 屏障。
- relaxed 只保证操作原子性，不阻止 CPU 或编译器对操作重排——仅适合单纯计数器、不参与同步的标志位。
- **acq_rel** 用于 RMW（fetch_add、CAS 等）：读取端 acquire，写入端 release，不形成全局顺序。
- 错误常见模式：用 relaxed store 发布数据，用 relaxed load 检测——接收方看到 flag=1 但 data 仍为旧值。

---

### 题目 91：C++20 协程 Generator 完整实现

```cpp
#include <coroutine>
#include <iostream>
#include <optional>
#include <exception>
#include <ranges>

template<typename T>
class Generator {
public:
    struct promise_type {
        std::optional<T>   value_;
        std::exception_ptr exception_;

        Generator             get_return_object() {
            return Generator{Handle::from_promise(*this)};
        }
        std::suspend_always   initial_suspend() noexcept { return {}; }
        std::suspend_always   final_suspend()   noexcept { return {}; }
        std::suspend_always   yield_value(T v) {
            value_ = std::move(v);
            return {};
        }
        void return_void()      {}
        void unhandled_exception() { exception_ = std::current_exception(); }
    };

    using Handle = std::coroutine_handle<promise_type>;

    struct iterator {
        Handle h_;
        bool   done_;

        iterator& operator++() {
            h_.resume();
            done_ = h_.done();
            if (!done_ && h_.promise().exception_)
                std::rethrow_exception(h_.promise().exception_);
            return *this;
        }
        T           operator*()  const { return *h_.promise().value_; }
        bool        operator!=(const iterator& o) const { return done_ != o.done_; }
    };

    iterator begin() {
        h_.resume();                      // 运行到第一个 co_yield
        if (h_.promise().exception_)
            std::rethrow_exception(h_.promise().exception_);
        return {h_, h_.done()};
    }
    iterator end()   { return {h_, true}; }

    explicit Generator(Handle h) : h_(h) {}
    ~Generator() { if (h_) h_.destroy(); }
    Generator(Generator&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    Generator(const Generator&) = delete;

private:
    Handle h_;
};

// 使用
Generator<int> range(int from, int to, int step = 1) {
    for (int i = from; i < to; i += step)
        co_yield i;
}

Generator<int> fibonacci() {
    int a = 0, b = 1;
    while (true) {
        co_yield a;
        auto c = a + b;
        a = b; b = c;
    }
}

int main() {
    for (int v : range(0, 10, 2))
        std::cout << v << " ";   // 0 2 4 6 8
    std::cout << "\n";

    int count = 0;
    for (int v : fibonacci()) {
        if (count++ >= 8) break;
        std::cout << v << " ";   // 0 1 1 2 3 5 8 13
    }
    std::cout << "\n";
}
```

**解析**：
- `initial_suspend = suspend_always`：协程构造后立即挂起，等待 `begin()` 的 `resume()` 才开始执行。
- `final_suspend = suspend_always`：协程 `co_return` 后挂起，而非立即销毁——Generator 析构时才 `h_.destroy()`，避免双重销毁。
- `yield_value(v)` 把值存入 `promise_.value_`，然后 `suspend_always` 挂起；迭代器 `++` 调用 `resume()` 继续执行到下一个 yield。
- 异常在协程内通过 `unhandled_exception()` 捕获，存为 `exception_ptr`，在迭代器 `++` 或 `begin()` 时重抛——实现跨协程边界的异常传播。
- C++23 标准库提供 `std::generator<T>`，本题演示了其底层原理。

---

### 题目 92：协程 Task 类型（单次异步任务）

```cpp
#include <coroutine>
#include <iostream>
#include <functional>
#include <exception>

// 表示一个异步任务，完成后通知 continuation
struct Task {
    struct promise_type {
        std::exception_ptr exception_;
        std::function<void()> continuation_;

        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_never initial_suspend() noexcept { return {}; }  // 立即开始
        auto final_suspend() noexcept {
            struct Awaiter {
                bool await_ready() noexcept { return false; }
                void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                    if (h.promise().continuation_) h.promise().continuation_();
                }
                void await_resume() noexcept {}
            };
            return Awaiter{};
        }
        void return_void() {}
        void unhandled_exception() { exception_ = std::current_exception(); }
    };
    using Handle = std::coroutine_handle<promise_type>;

    // co_await Task：挂起当前协程，等待 Task 完成后恢复
    bool await_ready() noexcept { return h_.done(); }
    void await_suspend(std::coroutine_handle<> cont) noexcept {
        h_.promise().continuation_ = [cont]{ cont.resume(); };
    }
    void await_resume() {
        if (h_.promise().exception_)
            std::rethrow_exception(h_.promise().exception_);
    }

    explicit Task(Handle h) : h_(h) {}
    ~Task() { if (h_) h_.destroy(); }
    Task(Task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    Task(const Task&) = delete;

private:
    Handle h_;
};

Task fetch_data(int id) {
    std::cout << "fetching " << id << "\n";
    // 模拟异步操作
    std::cout << "got data for " << id << "\n";
    co_return;  // 触发 final_suspend → 调用 continuation_
}

Task process() {
    co_await fetch_data(1);  // 等待 fetch_data(1) 完成
    co_await fetch_data(2);
    std::cout << "all done\n";
}

int main() {
    auto t = process();  // initial_suspend=never，立即运行
    // 在真正的异步运行时中，这里会有事件循环
}
```

**解析**：
- `initial_suspend = suspend_never`：Task 创建后立即开始执行，直到遇到 `co_await` 或 `co_return`。
- `final_suspend` 中的 Awaiter：协程完成时调用 `continuation_`（上层协程的 `resume`），实现链式执行。
- `co_await Task` 触发 `await_suspend(cont)`：把 continuation 注册到当前 Task 的 promise，当 Task 完成时自动 resume 调用者。
- 生产级 Task 需要执行器（Executor）调度 `resume` 到特定线程，本题简化为直接调用。
- 协程帧在堆上分配（默认 `operator new`）；可在 `promise_type` 中重载 `operator new` 使用自定义分配器。

---

### 题目 93：UB 全景：有符号溢出与编译器优化

```cpp
#include <iostream>
#include <climits>
#include <cstdint>

// 有符号溢出 UB：编译器假设永不溢出，可做激进优化
bool can_add_without_overflow(int a, int b) {
    // 错误写法（UB）：编译器可能将 a+b 优化为永远不溢出
    // if (a + b < a) return false;  // 加法若溢出，结果是 UB，编译器删除此判断

    // 正确写法
    if (b > 0 && a > INT_MAX - b) return false;  // 正数溢出
    if (b < 0 && a < INT_MIN - b) return false;  // 负数溢出
    return true;
}

// 循环优化：编译器假设 i++ 不溢出，可将循环转为无条件代码
void ub_loop_opt() {
    // 若 n = INT_MAX, n+1 溢出是 UB
    // 编译器: 循环必然终止（因为无溢出假设），可优化
    for (int i = 0; i < 10; ++i) {
        // 编译器假设 i 永远不会环绕到负值
    }
}

// 指针越界：UB，编译器可删除"无用"的边界检查
int arr[10];
int read_element(int idx) {
    // if (idx >= 10) return -1;  // 若编译器证明 arr[idx] 会被执行，可能删此检查
    return arr[idx];  // 实际必须保证 idx in [0,9]
}

// 整数提升：char 运算时提升为 int
void promotion_trap() {
    unsigned char a = 200, b = 200;
    // a + b = 400，但 unsigned char 最大 255
    // 实际：a 和 b 提升为 int，结果是 int(400)，不是 char(144)
    int result = a + b;
    std::cout << result << "\n";       // 400，不是 144

    // 比较时的陷阱
    int neg = -1;
    unsigned int pos = 1;
    // -1 < 1 看起来为真，但：
    if ((unsigned int)neg > pos)       // -1 转为 UINT_MAX，远大于 1
        std::cout << "unsigned comparison trap!\n";
}

int main() {
    std::cout << can_add_without_overflow(INT_MAX, 1) << "\n";  // 0
    std::cout << can_add_without_overflow(100, 200)   << "\n";  // 1
    promotion_trap();
}
```

**解析**：
- 有符号整数溢出是 **UB**：C++ 标准允许编译器假设它永不发生，从而删除溢出检测代码，导致安全漏洞（如整数溢出→缓冲区溢出）。
- 无符号整数溢出是**模 2^n 的定义良好行为**（wraparound），编译器不能假设它不发生。
- **整数提升**：`char`/`short` 在参与算术运算前提升为 `int`；`unsigned char` + `unsigned char` 的结果是 `int`（非 `unsigned char`）。
- **有符号/无符号比较**：`(signed) < (unsigned)` 时，signed 被隐式转为 unsigned，负数变成大正数——是经典 bug 来源。
- 检测工具：`-fsanitize=undefined` 可在运行时捕获有符号溢出；`-fsanitize=signed-integer-overflow` 专项。

---

### 题目 94：UB：use-after-free 与 dangling reference

```cpp
#include <iostream>
#include <memory>
#include <string>
#include <functional>

// 1. use-after-free
void uaf_demo() {
    int* p = new int(42);
    delete p;
    // *p = 99;   // UB：写已释放内存
    // std::cout << *p;  // UB：读已释放内存
    // delete p;  // UB：double free
}

// 2. 悬垂引用
const int& dangling_ref() {
    int local = 100;
    return local;   // UB：返回局部变量引用，函数返回后 local 析构
}

// 3. 悬垂指针（string_view/span 的常见场景）
std::string_view create_view() {
    std::string s = "temporary";
    return s;   // UB：s 销毁后 view 悬垂
}

// 4. lambda 捕获引用的陷阱
std::function<int()> make_lambda() {
    int local = 42;
    return [&local](){ return local; };  // UB：local 在函数返回后销毁
}

// 5. 智能指针正确用法
void smart_ptr_safe() {
    auto p = std::make_shared<std::string>("hello");
    std::weak_ptr<std::string> wp = p;

    p.reset();   // string 销毁

    if (auto sp = wp.lock()) {  // 检查是否还活着
        std::cout << *sp << "\n";
    } else {
        std::cout << "object expired\n";  // 输出这个
    }
}

int main() {
    uaf_demo();          // 行为未定义，可能崩溃也可能"正常"
    smart_ptr_safe();

    // const int& ref = dangling_ref();  // UB：绑定悬垂引用
    // auto sv = create_view();          // UB：sv 指向销毁的 string
    // auto fn = make_lambda();          // fn() 是 UB
}
```

**解析**：
- **use-after-free**：`delete` 后内存被 OS 或其他分配器回收/重用，访问是 UB；程序可能崩溃、输出错误值，或表面上"正常"（最危险的情况）。
- **悬垂引用**：返回局部变量引用后，引用绑定的内存栈帧已释放，读写是 UB。
- **string_view 悬垂**：临时 `string` 在表达式结束后析构，持有其 `string_view` 立即失效——是 C++17 引入 `string_view` 后最常见的新 bug。
- **lambda 捕获引用**：`[&local]` 捕获的是引用，若 lambda 生命周期超过 `local`，调用时是 UB——应改为按值捕获 `[local]`。
- 检测工具：`-fsanitize=address`（AddressSanitizer）能可靠检测 UAF、堆溢出、悬垂引用。

---

### 题目 95：UB：strict aliasing 与 reinterpret_cast

```cpp
#include <iostream>
#include <cstring>
#include <bit>
#include <cstdint>

// 非法：违反 strict aliasing（通过不兼容类型指针访问）
float int_to_float_ub(std::uint32_t x) {
    // return *reinterpret_cast<float*>(&x);  // UB！strict aliasing 违反
    return {};
}

// 合法1：memcpy（标准保证安全）
float int_to_float_memcpy(std::uint32_t x) {
    float f;
    std::memcpy(&f, &x, sizeof(float));
    return f;
}

// 合法2：std::bit_cast（C++20，编译期安全）
float int_to_float_bitcast(std::uint32_t x) {
    return std::bit_cast<float>(x);
}

// char* 是例外：可以 alias 任何类型
void inspect_bytes(const float& f) {
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(&f);
    std::cout << std::hex;
    for (std::size_t i = 0; i < sizeof(float); ++i)
        std::cout << (int)bytes[i] << " ";
    std::cout << std::dec << "\n";
}

// union 的 type punning（C++ 标准不完全支持，C99 支持）
union FloatInt {
    float f;
    std::uint32_t i;
};

int main() {
    std::uint32_t bits = 0x3F800000;  // 1.0f 的 IEEE 754 表示

    std::cout << int_to_float_memcpy(bits)   << "\n";  // 1.0
    std::cout << int_to_float_bitcast(bits)  << "\n";  // 1.0

    float pi = 3.14159f;
    inspect_bytes(pi);  // 查看 pi 的字节表示

    // union type punning（实现定义，多数编译器支持但非标准）
    FloatInt fi; fi.f = 1.0f;
    std::cout << std::hex << fi.i << std::dec << "\n";  // 3f800000

    // bit_cast 的类型约束
    // std::bit_cast<float>(0.0);  // 错误：double(8字节) → float(4字节) 大小不同
    static_assert(sizeof(float) == sizeof(std::uint32_t));
    auto back = std::bit_cast<std::uint32_t>(1.0f);
    std::cout << std::hex << back << "\n";  // 3f800000
}
```

**解析**：
- **Strict Aliasing Rule**：C++ 规定，通过不兼容类型的指针访问对象是 UB——编译器据此假设不同类型的指针不互 alias，做激进别名分析优化。
- `char*`/`unsigned char*`/`std::byte*` 是例外，可以安全访问任意类型的字节表示（inspect bytes）。
- `std::memcpy` 是合法的 type punning：标准保证 trivially copyable 类型可以 memcpy 并还原。
- `std::bit_cast<T>(x)`（C++20）：编译期类型安全的位模式重解释，要求 `sizeof(T)==sizeof(U)` 且两者均 trivially copyable。
- union type punning 在 C++ 标准中属实现定义，但 GCC/Clang/MSVC 均明确支持（比 memcpy 编译器有时优化更好）。

---

### 题目 96：RAII 与异常安全的三层保证

```cpp
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>

class DataStore {
    std::vector<std::string> items_;
    std::string log_;

public:
    // nothrow 保证：绝不抛出异常
    std::size_t size() const noexcept { return items_.size(); }

    // 基本保证：失败时对象仍有效（但状态可能改变）
    void append_basic(std::string val) {
        items_.push_back(std::move(val));  // 若此处抛出，items_ 可能部分修改
        log_ += "added; ";                  // 若此处抛出，items_ 已修改但 log_ 未更新
    }

    // 强保证：失败时状态不变（事务语义）
    void append_strong(std::string val) {
        // copy-then-swap
        auto new_items = items_;               // 拷贝（可能抛 bad_alloc）
        new_items.push_back(std::move(val));   // 在副本上操作
        std::string new_log = log_ + "added; ";
        // 以下操作不会抛出（noexcept swap）
        items_.swap(new_items);
        log_.swap(new_log);
    }

    // 演示：批量操作的强保证
    void append_batch(const std::vector<std::string>& vals) {
        auto new_items = items_;
        for (const auto& v : vals)
            new_items.push_back(v);   // 全部成功后才 swap
        items_.swap(new_items);        // noexcept
    }

    void print() const {
        for (auto& s : items_) std::cout << s << " ";
        std::cout << "\n";
    }
};

int main() {
    DataStore ds;
    ds.append_strong("hello");
    ds.append_strong("world");
    ds.print();  // hello world

    try {
        // 模拟强保证：异常前状态不变
        ds.append_strong("trigger_exception");
        // 若 push_back 抛出，items_ 保持原来 2 个元素
    } catch (...) {
        ds.print();  // 仍是 hello world
    }

    // 演示 noexcept 的重要性
    std::vector<DataStore> vec;
    vec.reserve(1);
    vec.push_back(DataStore{});
    // push_back 扩容时：若 DataStore 的移动构造不 noexcept，会用拷贝
}
```

**解析**：
- **nothrow 保证**：最强，`noexcept` 标注；析构函数、swap、移动应尽量 noexcept。
- **强保证**：操作要么完全成功，要么对象状态不变（rollback 语义）——用 copy-then-swap 实现：先在副本上操作，最后原子地 swap（swap 通常 noexcept）。
- **基本保证**：失败时对象处于有效但未指定状态（可继续使用，但状态未知）——STL 容器的 push_back 等操作通常满足基本保证。
- **无保证**：失败可能导致资源泄漏、不变量破坏——手写代码最容易陷入此困境。
- RAII 是实现异常安全的基础：资源在构造时获取，析构时释放，即使路径有异常也能正确清理。

---

### 题目 97：move 语义与 noexcept 的正确写法

```cpp
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <type_traits>

class Resource {
    std::string name_;
    std::vector<int> data_;
    static int id_counter;
    int id_;

public:
    Resource(std::string name, std::size_t n)
        : name_(std::move(name)), data_(n, 0), id_(++id_counter) {
        std::cout << "Ctor[" << id_ << "] " << name_ << "\n";
    }

    ~Resource() { std::cout << "Dtor[" << id_ << "] " << name_ << "\n"; }

    Resource(const Resource& o)
        : name_(o.name_), data_(o.data_), id_(++id_counter) {
        std::cout << "Copy[" << id_ << "] from [" << o.id_ << "]\n";
    }

    Resource(Resource&& o) noexcept  // noexcept 是关键！
        : name_(std::move(o.name_))
        , data_(std::move(o.data_))
        , id_(o.id_) {
        o.id_ = -1;
        std::cout << "Move[" << id_ << "]\n";
    }

    Resource& operator=(const Resource& o) {
        Resource tmp(o);
        swap(tmp);
        return *this;
    }
    Resource& operator=(Resource&& o) noexcept {
        swap(o);
        return *this;
    }

    void swap(Resource& o) noexcept {
        std::swap(name_, o.name_);
        std::swap(data_, o.data_);
        std::swap(id_,   o.id_);
    }

    friend void swap(Resource& a, Resource& b) noexcept { a.swap(b); }
};
int Resource::id_counter = 0;

int main() {
    std::vector<Resource> vec;
    vec.reserve(1);

    std::cout << "--- emplace_back ---\n";
    vec.emplace_back("first", 10);

    std::cout << "--- push_back rvalue (move) ---\n";
    vec.push_back(Resource("second", 5));

    std::cout << "--- reallocation ---\n";
    vec.emplace_back("third", 3);  // 触发扩容：move 还是 copy？
    // 因为 noexcept 移动 → vector 用 move，无 copy

    // 验证 move_if_noexcept
    static_assert(std::is_nothrow_move_constructible_v<Resource>);
    Resource r("test", 2);
    Resource r2 = std::move_if_noexcept(r);  // noexcept → move
}
```

**解析**：
- 移动构造函数 `noexcept` 至关重要：`std::vector` 扩容时，若移动构造不 noexcept，则**强制使用拷贝**（以保证强异常安全）——对大型对象代价极高。
- `std::move_if_noexcept(x)` 原理：`is_nothrow_move_constructible_v<T>` 为真时返回 `move(x)`，否则返回 `x`（触发拷贝）。
- **copy-and-swap 赋值运算符**：按值接受参数（拷贝或移动），再 swap——实现强异常安全，且一个重载同时处理拷贝赋值和移动赋值。
- `emplace_back` vs `push_back`：前者在容器内原地构造，后者先构造临时对象再移动/拷贝——有移动构造时性能相近。
- `swap` 必须 noexcept：copy-and-swap 依赖 swap 不抛，标准容器的 `swap` 均 noexcept。

---

### 题目 98：explicit 转换与强类型 ID

```cpp
#include <iostream>
#include <type_traits>

// 强类型 ID：避免将 UserId 误传给 OrderId
template<typename Tag, typename T = int>
class StrongId {
    T value_;
public:
    explicit StrongId(T v) : value_(v) {}
    explicit operator T() const { return value_; }

    bool operator==(const StrongId& o) const { return value_ == o.value_; }
    bool operator< (const StrongId& o) const { return value_ <  o.value_; }

    friend std::ostream& operator<<(std::ostream& os, const StrongId& id) {
        return os << id.value_;
    }
};

struct UserTag {};
struct OrderTag {};
using UserId  = StrongId<UserTag>;
using OrderId = StrongId<OrderTag>;

void process(UserId uid, OrderId oid) {
    std::cout << "user=" << uid << " order=" << oid << "\n";
}

// explicit operator bool 的细节
struct Handle {
    void* ptr_ = nullptr;
    explicit operator bool() const { return ptr_ != nullptr; }
};

int main() {
    UserId  uid{42};
    OrderId oid{100};

    process(uid, oid);           // OK
    // process(oid, uid);        // 编译错误：类型不匹配

    // explicit 防止隐式转换
    // int id = uid;             // 错误：operator int 是 explicit
    int id = static_cast<int>(uid);  // OK：显式转换
    std::cout << id << "\n";    // 42

    // explicit operator bool
    Handle h{nullptr};
    if (h) {}                    // OK：bool context 触发 explicit operator bool
    // int x = h;               // 错误：explicit operator bool 不能隐式转 int
    // bool b = h;              // C++11：直接初始化 OK，拷贝初始化 OK（有些争议）

    // explicit 构造函数
    // UserId uid2 = 99;        // 错误：explicit 构造，不能拷贝初始化
    UserId uid2{99};            // OK：直接初始化
    std::cout << uid2 << "\n";
}
```

**解析**：
- `explicit` 构造函数：禁止隐式转换和拷贝初始化（`T t = value`），只允许直接初始化（`T t(value)` 或 `T t{value}`）。
- `explicit operator T()`：禁止隐式转换，只允许 `static_cast`、直接初始化和 bool 上下文（`if`/`while`/`!` 等）。
- `explicit operator bool` 允许 `if (obj)` 但禁止 `int x = obj`——消除了 C++03 中 `operator void*` 的歧义（void* 可转 int）。
- **Strong Typedef**：用 tag + `explicit` 包装基础类型，让编译器在类型检查时区分语义上不同的 ID，完全零运行时开销。
- C++23 引入 `std::strong_ordering` 等比较类别，与 `<=>` 配合可简化 StrongId 的比较运算符。

---

### 题目 99：initializer_list 的生命周期陷阱

```cpp
#include <iostream>
#include <vector>
#include <initializer_list>

// 1. initializer_list 的底层是编译器生成的 const 数组
void inspect_ilist(std::initializer_list<int> il) {
    std::cout << "size=" << il.size() << " begin=" << il.begin() << "\n";
    for (int x : il) std::cout << x << " ";
    std::cout << "\n";
}

// 2. 悬垂的 initializer_list（经典陷阱）
std::initializer_list<int> make_ilist() {
    // return {1, 2, 3};  // 危险！临时数组在函数返回后析构
    // 以下等价于：
    // const int arr[] = {1, 2, 3};
    // return std::initializer_list<int>(arr, arr+3);  // arr 是局部，悬垂
    return {};
}

// 3. vector{} vs vector() 的区别
void vector_init_trap() {
    std::vector<int> v1(3, 1);    // 3 个 1：[1,1,1]
    std::vector<int> v2{3, 1};    // 2 个元素 3 和 1：[3,1]
    std::cout << v1.size() << "\n";  // 3
    std::cout << v2.size() << "\n";  // 2

    // 若构造函数有 initializer_list 重载，花括号优先匹配它
    std::vector<std::vector<int>> vv{3};  // ？
    // {3} 匹配 initializer_list<vector<int>>：1 个元素（空 vector）
    // 而非 vector<vector<int>>(3)：3 个空 vector
    std::cout << vv.size() << "\n";  // 1
}

// 4. 参数推导时 initializer_list 无法推导模板 T
template<typename T>
void f(T x) {}
// f({1,2,3});  // 错误：无法推导 T

template<typename T>
void g(std::initializer_list<T> il) { std::cout << il.size() << "\n"; }
// g({1,2,3});  // OK：T=int

int main() {
    inspect_ilist({1, 2, 3, 4, 5});

    // auto + initializer_list
    auto il = {1, 2, 3};  // C++11: auto 推导为 initializer_list<int>
    static_assert(std::is_same_v<decltype(il), std::initializer_list<int>>);

    vector_init_trap();
    g({10, 20, 30});  // 3
}
```

**解析**：
- `initializer_list<T>` 底层是编译器生成的 `const T[]` 数组的视图；**生命周期绑定到创建它的 full-expression**，函数返回时数组析构，返回的 `initializer_list` 立即悬垂。
- `{3}` 在 `vector<int>` 构造时：若存在 `initializer_list` 重载，**优先匹配**，等价于 `vector<int>{initializer_list<int>{3}}`（1 个元素 3），而非 `vector<int>(3)`（3 个 0）。
- `auto x = {1,2,3}` 推导为 `initializer_list<int>`；`auto x = {1}` 推导为 `initializer_list<int>`（C++17 后 `auto x{1}` 推导为 `int`）。
- 模板参数 `T` 无法从 `{1,2,3}` 推导（brace-enclosed initializer 不是表达式），需要显式写 `std::initializer_list<int>` 形参。
- `initializer_list` 的元素永远是 `const` 的，不能修改。

---

### 题目 100：编译期设计——静态反射预演（constexpr + 结构体字段）

```cpp
#include <iostream>
#include <string_view>
#include <tuple>
#include <type_traits>

// 模拟 C++26 静态反射：通过 Boost.PFR 技术访问聚合类型字段
// 原理：聚合初始化 + 过载决议 + 结构化绑定

struct Point { int x; double y; std::string_view name; };

// 将聚合类型转为 tuple（仅适用于聚合，且字段数已知）
template<typename T>
auto to_tuple(T& t) {
    // C++20 风格：手动针对 3 字段结构体
    if constexpr (requires { auto& [a,b,c] = t; }) {
        auto& [a,b,c] = t;
        return std::tie(a,b,c);
    } else if constexpr (requires { auto& [a,b] = t; }) {
        auto& [a,b] = t;
        return std::tie(a,b);
    }
}

// 对结构体每个字段应用 f
template<typename T, typename F>
void for_each_field(T& obj, F&& f) {
    auto tup = to_tuple(obj);
    std::apply([&f](auto&... fields) {
        (f(fields), ...);
    }, tup);
}

// 字段计数（利用 requires + SFINAE）
template<typename T, typename... Args>
concept AggregateInitWith = requires { T{std::declval<Args>()...}; };

struct AnyType { template<typename T> operator T() const; };

template<typename T, std::size_t N = 0, typename = std::make_index_sequence<N>>
struct FieldCount;

// 简化版：实际 PFR 用更复杂的过载决议技术
constexpr std::size_t field_count_of_Point = 3;  // 手动标注

int main() {
    Point p{42, 3.14, "origin"};

    // 遍历所有字段并打印
    for_each_field(p, [](auto& field) {
        std::cout << field << " ";
    });
    std::cout << "\n";  // 42 3.14 origin

    // 修改字段
    for_each_field(p, [](auto& field) {
        using T = std::decay_t<decltype(field)>;
        if constexpr (std::is_arithmetic_v<T>)
            field *= 2;
    });
    std::cout << p.x << " " << p.y << "\n";  // 84 6.28

    // 转为 tuple 后用 get
    auto t = to_tuple(p);
    std::cout << std::get<0>(t) << "\n";  // 84
}
```

**解析**：
- C++17 结构化绑定 + `std::tie` 实现 aggregate → tuple 转换，但需手动针对字段数量写模板特化（或用 `if constexpr requires` 多路分支）。
- Boost.PFR（2017+）用精妙的过载决议自动推导字段数，无需宏/手动注册；原理是：创建聚合初始化参数数量从 N 递减，直到能成功初始化的最大 N 即字段数。
- C++26 静态反射（P2996）将原生支持 `std::meta::members_of`、`std::meta::identifier_of` 等操作，无需这些 hack。
- `std::apply(f, tuple)` 将 tuple 展开为参数调用 f，配合折叠表达式实现对每个字段的操作。
- 实际应用：自动序列化（结构体 → JSON/protobuf）、单元测试的自动对比、日志自动打印结构体。

---
