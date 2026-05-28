# MmapTest 知识详解

本文档系统讲解 `mmaptest` 项目中涉及的核心知识：Linux/Android 内存映射、相关系统调用、JNI 集成，以及 9 个演示场景的设计原理。

配套图示：

- [执行流程图](./mmap_flowchart.svg)
- [系统架构图](./mmap_architecture.svg)

---

## 目录

1. [项目概述](#1-项目概述)
2. [系统架构](#2-系统架构)
3. [虚拟内存与 mmap 基础](#3-虚拟内存与-mmap-基础)
4. [核心 API 详解](#4-核心-api-详解)
5. [九个演示场景详解](#5-九个演示场景详解)
6. [Android JNI 集成](#6-android-jni-集成)
7. [页、对齐与 RAII](#7-页对齐与-raii)
8. [Android 平台注意事项](#8-android-平台注意事项)
9. [常见问题与调试](#9-常见问题与调试)
10. [延伸阅读](#10-延伸阅读)

---

## 1. 项目概述

`mmaptest` 是一个 Android NDK 示例应用，通过 Native C++ 代码演示 `mmap` 及其配套系统调用的复杂用法，Kotlin 层负责 UI 触发与结果展示。

### 1.1 目录结构

```
mmaptest/
├── app/src/main/
│   ├── java/.../MainActivity.kt      # UI + JNI 声明
│   ├── cpp/
│   │   ├── native-lib.cpp            # JNI 入口
│   │   ├── mmap_demo.cpp             # 9 个演示实现
│   │   ├── mmap_demo.h
│   │   ├── mmap_utils.h              # 工具函数
│   │   └── CMakeLists.txt
│   └── res/layout/activity_main.xml
└── docs/
    ├── mmap_flowchart.svg
    ├── mmap_architecture.svg
    └── mmap_knowledge.md             # 本文档
```

### 1.2 运行方式

- 点击 **「运行全部」**：顺序执行 Demo 1–9
- 点击数字按钮 **1–9**：执行单项演示
- 结果输出到界面 `ScrollView`，Logcat 过滤标签 `MmapDemo`

---

## 2. 系统架构

项目采用经典 **分层架构**，自顶向下：

| 层级 | 组件 | 职责 |
|------|------|------|
| Presentation | `MainActivity.kt` / `activity_main.xml` | 用户交互、后台线程、UI 回显 |
| JNI Bridge | `native-lib.cpp` / `libmyapplication.so` | Java/Kotlin 与 C++ 字符串编组 |
| Native Logic | `mmap_demo.cpp` | 演示调度、业务逻辑 |
| Utility | `mmap_utils.h` | 页大小、对齐、hex dump、日志 |
| Kernel | Bionic libc + Linux 内核 | `mmap`、`msync` 等系统调用 |

### 2.1 数据流

```
用户点击
  → MainActivity.runDemo() [后台 Thread]
  → JNI: runAllMmapDemos(filesDir) / runSingleMmapDemo(filesDir, id)
  → mmap_demo.cpp 执行演示
  → 返回 std::string 报告
  → JNI 转为 jstring
  → runOnUiThread 更新 TextView
```

### 2.2 为何使用后台线程

`mmap` 演示涉及文件 I/O、`msync` 同步刷盘、多线程计数等可能耗时操作。在 `MainActivity` 中通过 `Thread { ... }.start()` 执行，避免阻塞 Android 主线程（UI 线程），否则会导致 ANR（Application Not Responding）。

---

## 3. 虚拟内存与 mmap 基础

### 3.1 什么是 mmap

`mmap`（memory map）将文件或设备映射到进程的**虚拟地址空间**，使访问文件内容可以像访问内存数组一样通过指针读写，而无需反复调用 `read`/`write`。

```c
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
```

### 3.2 内存层次模型

```
┌─────────────────────────────────────┐
│  进程虚拟地址空间 (Virtual Address)   │  ← 程序看到的指针地址
├─────────────────────────────────────┤
│  VMA (Virtual Memory Area)          │  ← 内核维护的映射区域描述
├─────────────────────────────────────┤
│  页表 (Page Table)                   │  ← 虚拟页 → 物理页 / 缺页
├─────────────────────────────────────┤
│  物理页 / Page Cache                 │  ← 实际 RAM 或文件缓存
├─────────────────────────────────────┤
│  存储后端 (文件 / swap / 匿名)        │  ← 磁盘、交换分区
└─────────────────────────────────────┘
```

### 3.3 mmap 与 read/write 对比

| 特性 | read/write | mmap |
|------|------------|------|
| 数据拷贝 | 用户态 ↔ 内核态缓冲区，通常多一次拷贝 | 直接映射页缓存，减少拷贝 |
| 随机访问 | 每次需 `lseek` + `read` | 指针偏移即可 |
| 大文件 | 需分块读入内存 | 可按需映射部分区域（lazy loading） |
| 进程间共享 | 需额外 IPC 机制 | `MAP_SHARED` 天然共享页缓存 |
| 生命周期 | 关闭 fd 后缓冲区独立 | 映射独立于 fd（映射后 fd 可关闭） |

### 3.4 页（Page）

内存管理的基本单位。Android arm64 设备通常为 **4096 字节（4 KiB）**。本项目通过 `sysconf(_SC_PAGESIZE)` 获取。

**重要约束：**

- `mmap` 的 `offset` 必须是页大小的整数倍
- `mprotect` 的地址和长度通常需页对齐
- 映射长度会被内核向上取整到页边界

---

## 4. 核心 API 详解

### 4.1 mmap

```c
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
```

| 参数 | 含义 |
|------|------|
| `addr` | 期望映射地址；通常传 `NULL` 由内核选择 |
| `length` | 映射字节数（会按页对齐） |
| `prot` | 保护标志：`PROT_READ`、`PROT_WRITE`、`PROT_EXEC`、`PROT_NONE` |
| `flags` | 映射类型与行为（见下表） |
| `fd` | 文件描述符；匿名映射时传 `-1` |
| `offset` | 文件内起始偏移，**必须页对齐** |

**返回值：** 成功返回映射起始地址；失败返回 `MAP_FAILED`（即 `(void*)-1`）。

#### flags 常用组合

| flags | 含义 |
|-------|------|
| `MAP_SHARED` | 映射修改对其他进程可见，会写回文件（经 page cache） |
| `MAP_PRIVATE` | 写时复制（COW）；修改仅对本进程私有副本可见 |
| `MAP_ANONYMOUS` | 不关联文件，映射零初始化内存（或 `MAP_UNINITIALIZED`） |
| `MAP_FIXED` | 强制映射到指定 `addr`（危险，一般避免） |
| `MAP_SHARED \| MAP_ANONYMOUS` | 匿名共享内存（进程内多线程/或通过 fd 传递后跨进程） |

> Android Bionic 中 `MAP_ANONYMOUS` 也可写作 `MAP_ANON`，二者等价。

### 4.2 munmap

```c
int munmap(void *addr, size_t length);
```

解除映射，释放虚拟地址空间。`length` 必须与创建映射时一致（或覆盖完整映射区域）。未 `munmap` 的映射在进程退出时由内核回收，但长期运行服务应主动释放。

### 4.3 msync

```c
int msync(void *addr, size_t length, int flags);
```

将 `MAP_SHARED` 映射的修改刷入磁盘（经由 page cache 写回）。

| flags | 行为 |
|-------|------|
| `MS_SYNC` | 同步刷盘，调用返回时 I/O 已完成 |
| `MS_ASYNC` | 异步调度写回，立即返回 |
| `MS_INVALIDATE` | 使其他映射的缓存失效（较少用） |

> `MAP_PRIVATE` 的修改不会写回文件，`msync` 对其无持久化效果。

### 4.4 mprotect

```c
int mprotect(void *addr, size_t len, int prot);
```

动态修改**已映射**内存区域的访问权限。

典型场景：

- **JIT 编译器**：生成代码时 `RW`，执行前改为 `RX`（W^X 安全策略）
- **安全沙箱**：将敏感数据区域设为 `PROT_READ`
- **Guard page**：用 `PROT_NONE` 检测越界访问（触发 SIGSEGV）

写入只读页会触发 **SIGSEGV**（段错误）。Demo 4 故意跳过写只读页的操作。

### 4.5 madvise

```c
int madvise(void *addr, size_t length, int advice);
```

向内核提供访问模式**建议**，不改变语义，仅影响性能与内存回收策略。

| advice | 含义 |
|--------|------|
| `MADV_SEQUENTIAL` | 预期顺序访问，内核可提前释放已读页 |
| `MADV_RANDOM` | 预期随机访问，减少预读 |
| `MADV_WILLNEED` | 提示即将访问，内核可预加载 |
| `MADV_DONTNEED` | 丢弃物理页内容；虚拟映射仍有效，再次访问时 zero-fill 或重新 fault |

`MADV_DONTNEED` 常用于内存压力下的主动释放，或大块临时缓冲用完即弃。

### 4.6 mincore

```c
int mincore(void *addr, size_t length, unsigned char *vec);
```

查询映射区域中每一页是否当前在物理内存（RAM）中。`vec[i]` 最低位为 1 表示该页 resident。

用途：内存诊断、监控 lazy allocation 行为。未 touch 的匿名页可能尚未分配物理页，mincore 返回 `no`。

---

## 5. 九个演示场景详解

### Demo 1：MAP_SHARED 文件映射

**源文件：** `demoFileMapShared()`  
**对应按钮：** 1

#### 做了什么

1. 在 `filesDir` 创建 `mmap_shared.dat`，`ftruncate` 至 4 页大小
2. `mmap(..., MAP_SHARED, fd, 0)` 建立共享映射
3. 写入 magic number `0xDEADBEEF` 和字符串 payload
4. `msync(MS_SYNC)` 同步刷盘
5. `munmap` 后重新只读 `mmap` 验证持久化

#### 核心知识

- **MAP_SHARED + 写操作**：修改直接作用于 page cache，对所有共享该文件的映射可见
- **msync**：即使内核有 writeback 机制，关键数据仍建议显式 `msync` 确保落盘
- **重新 mmap**：验证数据确实写入存储，而非仅存在于旧映射的内存视图

#### 关键代码路径

```
open → ftruncate → mmap(MAP_SHARED) → 指针写入 → msync → munmap → mmap(只读验证)
```

---

### Demo 2：MAP_PRIVATE 写时复制（COW）

**源文件：** `demoMapPrivateCow()`  
**对应按钮：** 2

#### 做了什么

1. 创建文件并写入 `ORIGINAL_PAGE_DATA_COW_TEST`
2. `mmap(..., MAP_PRIVATE, fd, 0)` 建立私有映射
3. 通过映射修改内容为 `MODIFIED_IN_PRIVATE_MAP`
4. 用 `read(fd)` 直接读文件，验证磁盘内容未变

#### 核心知识：Copy-On-Write

```
初始状态：
  文件页 ──映射──→ 进程虚拟页（共享同一物理页，只读）

写入 MAP_PRIVATE 映射时：
  内核复制物理页 → 新物理页（私有副本）
  修改仅在新页上，原文件页不变
```

- 多个 `MAP_PRIVATE` 映射同一文件时，读取共享同一物理页
- 任一映射写入时，仅该映射触发 COW
- 适合 `fork()` 后父子进程隔离写入的场景

---

### Demo 3：MAP_ANONYMOUS | MAP_SHARED

**源文件：** `demoAnonymousShared()`  
**对应按钮：** 3

#### 做了什么

1. `mmap(..., MAP_ANONYMOUS | MAP_SHARED, -1, 0)` 分配 2 页匿名共享内存
2. 在映射区 placement new 构造 `SharedCounter`（含 `std::atomic<int>`）
3. 4 个 C++ 线程各执行 1000 次 `fetch_add`
4. 验证最终计数为 4000

#### 核心知识

- **MAP_ANONYMOUS**：无文件后端，内容初始为零（除非 `MAP_UNINITIALIZED`）
- **MAP_SHARED（匿名）**：同一进程内多线程共享同一物理页；若通过 `memfd_create` / `ASharedMemory` 传递 fd，可跨进程共享
- **atomic + 共享内存**：多线程计数需原子操作；本项目用 `memory_order_relaxed` 即可（仅计数，无其他数据依赖）

#### 与 malloc 的区别

| | malloc | mmap |
|--|--------|------|
| 粒度 | 任意字节（带 overhead） | 页对齐 |
| 释放 | free | munmap |
| 跨进程 | 不支持 | MAP_SHARED / ashmem 等可以 |
| 大分配 | 可能触发 brk | 适合大块、长期映射 |

---

### Demo 4：mprotect 动态页权限

**源文件：** `demoMprotect()`  
**对应按钮：** 4

#### 做了什么

1. 匿名映射 3 页，全部 `PROT_READ | PROT_WRITE`
2. 第 2 页改为 `PROT_READ`（只读）
3. 第 3 页改为 `PROT_NONE`（不可访问）
4. 读取只读页成功；跳过写只读页（否则 SIGSEGV）

#### 核心知识

- 权限以**页**为单位生效
- `PROT_EXEC` 在 Android 上受 W^X 限制：不可同时 Writable + Executable（安全缓解 ROP/JIT 攻击）
- SELinux + 内核 NX bit 共同 enforce 执行权限

#### Android 典型应用

ART 编译、WebView V8 JIT、自定义脚本引擎均涉及 **RW → RX** 权限切换。

---

### Demo 5：madvise 内存建议

**源文件：** `demoMadvise()`  
**对应按钮：** 5

#### 做了什么

1. 映射 8 页匿名私有内存并 memset
2. `MADV_SEQUENTIAL` 提示顺序访问
3. 后半 4 页 `MADV_DONTNEED` 丢弃物理页
4. 重新 touch 被 discard 区域，验证可正常写入

#### 核心知识

- `MADV_DONTNEED` 后虚拟地址**仍然有效**，但物理页被回收
- 再次访问触发 **page fault**，内核分配新物理页（内容为零）
- 适合：大缓冲区用完即弃、图像解码临时缓冲、内存压力主动降级

---

### Demo 6：带 offset 的部分映射

**源文件：** `demoPartialMapWithOffset()`  
**对应按钮：** 6

#### 做了什么

1. 创建 8 页文件，每页填充不同字符（A–H）
2. 仅映射 `offset = 3 * pageSize` 起的 2 页（对应字符 D、E）
3. 验证映射区首字节为 `'D'`，末字节为 `'E'`

#### 核心知识

- 不必映射整个文件，可只映射需要的区间（类似 `Array.slice`）
- **offset 必须页对齐**（4096 的倍数 on arm64）
- 大文件（数据库、日志、视频索引）常用 partial mmap 降低 VA 占用

#### 计算关系

```
文件布局:  | page0 | page1 | page2 | page3 | page4 | ...
映射 offset=3*ps, len=2*ps  →  覆盖 page3 和 page4
映射区 [0]     = 文件 byte[3*ps]
映射区 [2*ps-1] = 文件 byte[5*ps-1]
```

---

### Demo 7：双 MAP_SHARED 映射

**源文件：** `demoDualMapping()`  
**对应按钮：** 7

#### 做了什么

1. 对同一 fd 调用两次 `mmap(MAP_SHARED)`，得到 `mapA` 和 `mapB`（不同虚拟地址）
2. 通过 `mapA` 写入字符串
3. 通过 `mapB` 读取，验证立即可见

#### 核心知识

- 两个虚拟地址映射同一 inode 的 page cache
- **MAP_SHARED 的一致性**：一处写入，另一映射立即可见（同一物理页）
- 若使用 **MAP_PRIVATE**，则各自 COW，互不可见

```
mapA (VA 0x7a...) ──┐
                     ├──→ 同一物理页 ←── page cache ←── 文件
mapB (VA 0x7b...) ──┘
```

---

### Demo 8：mmap 环形缓冲区

**源文件：** `demoMmapRingBuffer()` / `MmapRingBuffer`  
**对应按钮：** 8

#### 做了什么

1. 用匿名私有 mmap 分配 Header + 256 字节数据区（向上对齐到页大小）
2. 实现 SPSC（单生产者单消费者）环形队列：`push` / `pop`
3. push 0–199，pop 全部，验证 sum = 19900

#### 数据结构

```cpp
struct Header {
    atomic<uint32_t> head;   // 写指针
    atomic<uint32_t> tail;   // 读指针
    uint32_t capacity;       // 256
    uint32_t mask;           // capacity - 1，用于位运算取模
};
```

#### 核心知识

- **capacity 必须为 2 的幂**：`(index + 1) & mask` 代替 `%`，避免除法
- **memory_order**：`tail` acquire 读 `head` 保证看到最新写入；`head` release 写保证数据可见
- 用 mmap 而非 malloc：便于后续扩展为共享内存队列（改为 `MAP_SHARED` + fd 传递）
- 本项目为单线程 push+pop 演示；真正 SPSC 需 pin 线程到核 + 避免 false sharing

---

### Demo 9：mincore 页驻留查询

**源文件：** `demoMincore()`  
**对应按钮：** 9

#### 做了什么

1. 映射 4 页匿名内存
2. 仅 touch 前 2 页（memset）
3. `mincore` 查询每页是否在 RAM 中

#### 核心知识

- **Lazy allocation**：`mmap` 返回时未必分配物理页，首次访问才 page fault
- 未 touch 的页 mincore 通常为 `no`
- 用于内存 profiling、排查 OOM、验证大映射的实际 RAM 占用

---

## 6. Android JNI 集成

### 6.1 加载 Native 库

```kotlin
companion object {
    init {
        System.loadLibrary("myapplication")  // 对应 libmyapplication.so
    }
}
```

CMake 中 `project("myapplication")` 生成 `libmyapplication.so`，Gradle 打包进 APK 的 `lib/arm64-v8a/` 等目录。

### 6.2 声明 Native 方法

```kotlin
external fun runAllMmapDemos(filesDir: String): String
external fun runSingleMmapDemo(filesDir: String, demoId: Int): String
```

`external` 表示实现在 Native 层，JNI 函数名须严格匹配：

```
Java_com_example_myapplication_MainActivity_runAllMmapDemos
Java_com_example_myapplication_MainActivity_runSingleMmapDemo
```

### 6.3 JNI 字符串转换

```cpp
const char* path = env->GetStringUTFChars(filesDir, nullptr);
// ... 使用 path ...
env->ReleaseStringUTFChars(filesDir, path);
return env->NewStringUTF(result.c_str());
```

| 函数 | 作用 |
|------|------|
| `GetStringUTFChars` | Java String → UTF-8 C 字符串 |
| `ReleaseStringUTFChars` | 释放 Get 获取的缓冲区 |
| `NewStringUTF` | C 字符串 → Java String 返回给 Kotlin |

> 必须配对 Release，否则内存泄漏。返回的 `jstring` 由 JVM 管理。

### 6.4 filesDir 路径

```kotlin
val filesDir = applicationContext.filesDir.absolutePath
// 例: /data/user/0/com.example.myapplication/files
```

应用私有目录，无需存储权限，适合演示文件 backed mmap。Native 层在此创建 `*.dat` 测试文件。

---

## 7. 页、对齐与 RAII

### 7.1 pageSize()

```cpp
static const size_t ps = sysconf(_SC_PAGESIZE);  // 通常 4096
```

所有演示以页为单位规划映射大小，满足内核对 offset 和对齐的要求。

### 7.2 alignUp()

```cpp
size_t alignUp(size_t value, size_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}
```

环形缓冲区将 `sizeof(Header) + capacity` 向上对齐到页边界，避免跨页结构体带来的权限/性能问题。

### 7.3 RAII 资源管理

```cpp
struct ScopedFd {
    int fd = -1;
    ~ScopedFd() { if (fd >= 0) close(fd); }
};

struct ScopedMmap {
    void* addr = MAP_FAILED;
    size_t len = 0;
    ~ScopedMmap() { if (addr != MAP_FAILED) munmap(addr, len); }
};
```

C++ 析构函数保证异常路径下 fd 和映射仍被释放。生产代码建议使用 `unique_ptr` 自定义 deleter 或 C++20 `scope_exit`。

---

## 8. Android 平台注意事项

### 8.1 W^X 与 SELinux

- 不能将用户 Writable 内存同时标记 Executable（防代码注入）
- `/data` 分区文件 mmap 通常不允许 `PROT_EXEC`
- 系统库、APK 内 `.so` 由 loader 以正确权限映射

### 8.2 匿名映射与内存限制

- 进程虚拟内存受 `RLIMIT_AS` 和设备 RAM 约束
- 映射过大可能触发 LMK（Low Memory Killer）杀进程
- 32 位进程 VA 空间有限（~3GB），大映射需注意

### 8.3 跨进程共享内存（扩展）

本项目 Demo 3 为进程内共享。Android 跨进程方案：

| 机制 | API 级别 | 说明 |
|------|----------|------|
| `ASharedMemory` | 26+ | 推荐，创建 ashmem/memfd 风格共享段 |
| `MemoryFile` | 旧版 | 内部基于 ashmem |
| `memfd_create` | Linux 3.17+ | Bionic 新版本支持，可 sealed memfd |

### 8.4 与 Java MappedByteBuffer 的关系

Java NIO 的 `FileChannel.map()` 底层即调用 `mmap`：

```java
MappedByteBuffer buf = channel.map(MapMode.READ_WRITE, 0, size);
```

Native 层直接 `mmap` 可精细控制 flags、`madvise`、`mprotect`，适合性能敏感或特殊语义场景。

### 8.5 持久化与崩溃

- `MAP_SHARED` 修改在 page cache 中，内核异步 writeback
- 断电/强杀可能丢失未 msync 的数据
- 数据库（SQLite WAL）、日志系统通常有自己的 fsync 策略

---

## 9. 常见问题与调试

### 9.1 mmap 返回 MAP_FAILED

```cpp
if (map == MAP_FAILED) {
    LOGE("mmap failed: %s", strerror(errno));
}
```

| errno | 常见原因 |
|-------|----------|
| `EINVAL` | offset 未页对齐、flags 组合非法、length 为 0 |
| `ENOMEM` | 虚拟/物理内存不足 |
| `EBADF` | fd 无效（匿名映射应传 -1） |
| `EACCES` | 文件打开模式与 prot 不匹配（如只读 fd + PROT_WRITE） |

### 9.2 SIGSEGV

- 写只读映射（Demo 4）
- 访问 `PROT_NONE` 区域
- `munmap` 后继续解引用野指针
- 映射长度外越界（未分配 VMA 的区域）

调试：`adb logcat | grep DEBUG` 查看 tombstone / crash stack。

### 9.3 验证映射内容

```bash
adb shell run-as com.example.myapplication ls files/
adb shell run-as com.example.myapplication cat files/mmap_shared.dat | xxd
```

### 9.4 查看进程 maps

```bash
adb shell pidof com.example.myapplication
adb shell cat /proc/<pid>/maps
```

可看到每个 mmap 区域的地址范围、权限、`/path/to/file` 或 `[anon]`。

---

## 10. 延伸阅读

### 10.1 手册页（man pages）

```bash
man 2 mmap
man 2 msync
man 2 mprotect
man 2 madvise
man 2 mincore
man 2 munmap
```

Android 设备可在线查阅 [man7.org](https://man7.org/linux/man-pages/) 或 NDK 文档。

### 10.2 相关内核概念

- **VMA**（`vm_area_struct`）：/proc/pid/maps 每一行对应一个 VMA
- **Page Fault**：首次访问、COW、swap 换入均触发
- **Page Cache**：文件 IO 缓存层，MAP_SHARED 直接与之交互
- **TLB flush**：`mprotect` / `munmap` 后硬件 TLB 需刷新

### 10.3 演示与知识点对照表

| 按钮 | 函数 | 核心 flags/API | 知识点 |
|------|------|----------------|--------|
| 1 | `demoFileMapShared` | MAP_SHARED, msync | 文件写穿、持久化 |
| 2 | `demoMapPrivateCow` | MAP_PRIVATE | 写时复制 |
| 3 | `demoAnonymousShared` | MAP_ANONYMOUS \| MAP_SHARED | 匿名共享、atomic |
| 4 | `demoMprotect` | mprotect | 页权限、W^X |
| 5 | `demoMadvise` | madvise | 内存建议、释放物理页 |
| 6 | `demoPartialMapWithOffset` | offset mmap | 部分映射、页对齐 offset |
| 7 | `demoDualMapping` | 双 MAP_SHARED | 页缓存一致性 |
| 8 | `demoMmapRingBuffer` | 匿名 mmap + atomic | 环形队列、内存对齐 |
| 9 | `demoMincore` | mincore | 页驻留、lazy alloc |

### 10.4 推荐实践清单

1. 始终检查 `mmap` 返回值是否为 `MAP_FAILED`
2. 配对使用 `munmap`，长度与映射一致
3. 文件 backed 映射：`open` 模式与 `prot` 一致
4. 关键数据 `MAP_SHARED` 后调用 `msync(MS_SYNC)`
5. 不在 UI 线程执行阻塞式 mmap/msync
6. 使用 `pageSize()` 做 offset/长度对齐
7. 避免 `MAP_FIXED` 除非完全掌控地址空间布局
8. 生产环境考虑 `mmap` 失败降级到 read/write

---

*文档版本：与 mmaptest 项目同步。如有 API 行为差异，以目标 Android 版本 Bionic 实现为准。*
