#include "mmap_utils.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>

namespace {

struct ScopedFd {
    int fd = -1;
    ~ScopedFd() {
        if (fd >= 0) {
            close(fd);
        }
    }
};

struct ScopedMmap {
    void* addr = MAP_FAILED;
    size_t len = 0;
    ~ScopedMmap() {
        if (addr != MAP_FAILED && len > 0) {
            munmap(addr, len);
        }
    }
};

std::string appendLine(std::string& out, const std::string& line) {
    out += line;
    out += '\n';
    return out;
}

// ---------------------------------------------------------------------------
// 1. MAP_SHARED 文件映射：写穿内存，msync 持久化
// ---------------------------------------------------------------------------
std::string demoFileMapShared(const std::string& basePath) {
    std::string report;
    const std::string path = basePath + "/mmap_shared.dat";
    const size_t ps = pageSize();
    const size_t fileSize = ps * 4;

    ScopedFd fd;
    fd.fd = open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd.fd < 0) {
        return "MAP_SHARED: open failed: " + errnoStr();
    }
    if (ftruncate(fd.fd, static_cast<off_t>(fileSize)) != 0) {
        return "MAP_SHARED: ftruncate failed: " + errnoStr();
    }

    void* map = mmap(nullptr, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd.fd, 0);
    if (map == MAP_FAILED) {
        return "MAP_SHARED: mmap failed: " + errnoStr();
    }

    auto* header = static_cast<uint32_t*>(map);
    header[0] = 0xDEADBEEF;
    std::memcpy(static_cast<char*>(map) + ps, "Hello MAP_SHARED from mmaptest", 31);

    // msync 三种模式演示
    if (msync(map, fileSize, MS_SYNC) != 0) {
        appendLine(report, "msync(MS_SYNC) failed: " + errnoStr());
    } else {
        appendLine(report, "msync(MS_SYNC) ok — 同步刷盘完成");
    }

    // 从磁盘重新 mmap 只读验证
    munmap(map, fileSize);

    void* verify = mmap(nullptr, fileSize, PROT_READ, MAP_SHARED, fd.fd, 0);
    if (verify == MAP_FAILED) {
        appendLine(report, "verify mmap failed: " + errnoStr());
        return report;
    }

    const uint32_t magic = *static_cast<uint32_t*>(verify);
    char buf[32] = {};
    std::memcpy(buf, static_cast<char*>(verify) + ps, 31);

    appendLine(report, "=== 1. MAP_SHARED 文件映射 ===");
    appendLine(report, "path: " + path);
    appendLine(report, "fileSize: " + std::to_string(fileSize) + " (4 pages)");
    appendLine(report, "magic @ offset 0: 0x" + [&]() {
        char hex[16];
        snprintf(hex, sizeof(hex), "%08x", magic);
        return std::string(hex);
    }());
    appendLine(report, "payload @ page1: " + std::string(buf));
    appendLine(report, "hex page0: " + hexDump(verify, ps));

    munmap(verify, fileSize);
    return report;
}

// ---------------------------------------------------------------------------
// 2. MAP_PRIVATE 写时复制 (COW)
// ---------------------------------------------------------------------------
std::string demoMapPrivateCow(const std::string& basePath) {
    std::string report;
    const std::string path = basePath + "/mmap_private_cow.dat";
    const size_t ps = pageSize();
    const size_t len = ps;

    ScopedFd fd;
    fd.fd = open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd.fd < 0) {
        return "MAP_PRIVATE: open failed: " + errnoStr();
    }

    const char original[] = "ORIGINAL_PAGE_DATA_COW_TEST";
    if (write(fd.fd, original, sizeof(original)) < 0) {
        return "MAP_PRIVATE: write failed: " + errnoStr();
    }

    void* map = mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd.fd, 0);
    if (map == MAP_FAILED) {
        return "MAP_PRIVATE: mmap failed: " + errnoStr();
    }

    char beforeWrite[32] = {};
    std::memcpy(beforeWrite, map, sizeof(beforeWrite) - 1);

    // 修改私有映射 — 触发 COW，不应写回文件
    std::memcpy(map, "MODIFIED_IN_PRIVATE_MAP", 24);

    char fileBuf[32] = {};
    lseek(fd.fd, 0, SEEK_SET);
    read(fd.fd, fileBuf, sizeof(fileBuf) - 1);

    appendLine(report, "=== 2. MAP_PRIVATE 写时复制 ===");
    appendLine(report, "映射后读取: " + std::string(beforeWrite));
    appendLine(report, "私有映射写入后，映射内: " + std::string(static_cast<char*>(map)));
    appendLine(report, "磁盘文件内容(应仍为 ORIGINAL): " + std::string(fileBuf));
    appendLine(report, fileBuf[0] == 'O' ? "COW 验证通过 ✓" : "COW 验证失败 ✗");

    munmap(map, len);
    return report;
}

// ---------------------------------------------------------------------------
// 3. 匿名映射 MAP_ANONYMOUS + MAP_SHARED（进程内共享堆）
// ---------------------------------------------------------------------------
std::string demoAnonymousShared() {
    std::string report;
    const size_t len = pageSize() * 2;

    void* map = mmap(nullptr, len, PROT_READ | PROT_WRITE,
                     MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    if (map == MAP_FAILED) {
        return "ANON_SHARED: mmap failed: " + errnoStr();
    }

    struct SharedCounter {
        std::atomic<int> value;
        char padding[60];
    };
    static_assert(sizeof(SharedCounter) <= 64, "fits in cache line");

    auto* counter = new (map) SharedCounter();
    counter->value.store(0);

    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([counter]() {
            for (int i = 0; i < 1000; ++i) {
                counter->value.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }

    appendLine(report, "=== 3. MAP_ANONYMOUS | MAP_SHARED ===");
    appendLine(report, "4 线程各 +1000，atomic 计数 = " +
                         std::to_string(counter->value.load()));
    appendLine(report, "用途: 无文件 backing 的进程内共享内存 / 自管理堆");

    munmap(map, len);
    return report;
}

// ---------------------------------------------------------------------------
// 4. mprotect 动态改页权限
// ---------------------------------------------------------------------------
std::string demoMprotect() {
    std::string report;
    const size_t ps = pageSize();
    const size_t len = ps * 3;

    void* map = mmap(nullptr, len, PROT_READ | PROT_WRITE,
                     MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (map == MAP_FAILED) {
        return "mprotect: mmap failed: " + errnoStr();
    }

    std::memset(map, 0xAA, len);

    // 中间一页设为只读
    void* roPage = static_cast<char*>(map) + ps;
    if (mprotect(roPage, ps, PROT_READ) != 0) {
        appendLine(report, "mprotect(PROT_READ) failed: " + errnoStr());
    } else {
        appendLine(report, "mprotect: 第 2 页已设为只读");
    }

    // 第三页设为不可访问
    void* noPage = static_cast<char*>(map) + ps * 2;
    if (mprotect(noPage, ps, PROT_NONE) != 0) {
        appendLine(report, "mprotect(PROT_NONE) failed: " + errnoStr());
    } else {
        appendLine(report, "mprotect: 第 3 页已设为 PROT_NONE");
    }

    // 读只读页 OK
    const uint8_t roByte = *static_cast<uint8_t*>(roPage);
    appendLine(report, "只读页首字节: 0x" + [&]() {
        char hex[8];
        snprintf(hex, sizeof(hex), "%02x", roByte);
        return std::string(hex);
    }());

    // 尝试写只读页 — 会 SIGSEGV，这里只演示 API，不实际写入
    appendLine(report, "（跳过写只读页，否则会 SIGSEGV）");

    appendLine(report, "=== 4. mprotect 页权限 ===");
    appendLine(report, "典型场景: JIT 代码页 RW→RX, 安全沙箱, 延迟权限收紧");

    munmap(map, len);
    return report;
}

// ---------------------------------------------------------------------------
// 5. madvise 内存建议 + MADV_DONTNEED 释放物理页
// ---------------------------------------------------------------------------
std::string demoMadvise() {
    std::string report;
    const size_t ps = pageSize();
    const size_t len = ps * 8;

    void* map = mmap(nullptr, len, PROT_READ | PROT_WRITE,
                     MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (map == MAP_FAILED) {
        return "madvise: mmap failed: " + errnoStr();
    }

    std::memset(map, 0x55, len);

    if (madvise(map, len, MADV_SEQUENTIAL) != 0) {
        appendLine(report, "MADV_SEQUENTIAL failed: " + errnoStr());
    }
    appendLine(report, "MADV_SEQUENTIAL: 顺序访问提示已设置");

    // 释放后半部分物理页（虚拟地址仍有效，再次访问会 zero-fill）
    const size_t half = len / 2;
    if (madvise(static_cast<char*>(map) + half, half, MADV_DONTNEED) != 0) {
        appendLine(report, "MADV_DONTNEED failed: " + errnoStr());
    } else {
        appendLine(report, "MADV_DONTNEED: 后半 " + std::to_string(half) + " 字节物理页已丢弃");
    }

    // 重新 touch 被 discard 的区域
    std::memset(static_cast<char*>(map) + half, 0xCC, ps);
    const uint8_t touched = static_cast<uint8_t*>(map)[half];

    appendLine(report, "=== 5. madvise ===");
    appendLine(report, "重新 touch 后 byte@half = 0x" + [&]() {
        char hex[8];
        snprintf(hex, sizeof(hex), "%02x", touched);
        return std::string(hex);
    }());
    appendLine(report, "MADV_RANDOM / MADV_WILLNEED 可用于预读策略");

    munmap(map, len);
    return report;
}

// ---------------------------------------------------------------------------
// 6. 非零 offset 映射（页对齐 offset）
// ---------------------------------------------------------------------------
std::string demoPartialMapWithOffset(const std::string& basePath) {
    std::string report;
    const std::string path = basePath + "/mmap_offset.dat";
    const size_t ps = pageSize();
    const size_t totalPages = 8;
    const size_t fileSize = ps * totalPages;
    const off_t mapOffset = static_cast<off_t>(ps * 3);  // 从第 4 页开始映射
    const size_t mapLen = ps * 2;

    ScopedFd fd;
    fd.fd = open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd.fd < 0) {
        return "offset mmap: open failed: " + errnoStr();
    }
    if (ftruncate(fd.fd, static_cast<off_t>(fileSize)) != 0) {
        return "offset mmap: ftruncate failed: " + errnoStr();
    }

    // 填充每页 distinct pattern
    void* full = mmap(nullptr, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd.fd, 0);
    if (full == MAP_FAILED) {
        return "offset mmap: full map failed: " + errnoStr();
    }
    for (size_t p = 0; p < totalPages; ++p) {
        std::memset(static_cast<char*>(full) + p * ps, static_cast<int>('A' + p), ps);
    }
    msync(full, fileSize, MS_ASYNC);
    munmap(full, fileSize);

    void* partial = mmap(nullptr, mapLen, PROT_READ | PROT_WRITE,
                         MAP_SHARED, fd.fd, mapOffset);
    if (partial == MAP_FAILED) {
        return "offset mmap: partial map failed: " + errnoStr();
    }

    appendLine(report, "=== 6. 带 offset 的部分映射 ===");
    appendLine(report, "fileSize=" + std::to_string(fileSize) +
                         ", offset=" + std::to_string(mapOffset) +
                         ", mapLen=" + std::to_string(mapLen));
    appendLine(report, "映射区首字节应为 'D'(page3): " +
                         std::string(1, *static_cast<char*>(partial)));
    appendLine(report, "映射区末字节应为 'E'(page4 末): " +
                         std::string(1, static_cast<char*>(partial)[mapLen - 1]));
    appendLine(report, "hex 前 16 字节: " + hexDump(partial, 16));

    munmap(partial, mapLen);
    return report;
}

// ---------------------------------------------------------------------------
// 7. 双映射同一文件（两个虚拟地址指向同一 inode 页缓存）
// ---------------------------------------------------------------------------
std::string demoDualMapping(const std::string& basePath) {
    std::string report;
    const std::string path = basePath + "/mmap_dual.dat";
    const size_t ps = pageSize();

    ScopedFd fd;
    fd.fd = open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd.fd < 0) {
        return "dual map: open failed: " + errnoStr();
    }
    if (ftruncate(fd.fd, static_cast<off_t>(ps)) != 0) {
        return "dual map: ftruncate failed: " + errnoStr();
    }

    void* mapA = mmap(nullptr, ps, PROT_READ | PROT_WRITE, MAP_SHARED, fd.fd, 0);
    void* mapB = mmap(nullptr, ps, PROT_READ | PROT_WRITE, MAP_SHARED, fd.fd, 0);
    if (mapA == MAP_FAILED || mapB == MAP_FAILED) {
        if (mapA != MAP_FAILED) munmap(mapA, ps);
        if (mapB != MAP_FAILED) munmap(mapB, ps);
        return "dual map: mmap failed: " + errnoStr();
    }

    std::memcpy(mapA, "written_via_mapA", 17);
    // mapB 应立即可见（同一 page cache）
    const char* seen = static_cast<const char*>(mapB);

    appendLine(report, "=== 7. 双 MAP_SHARED 映射 ===");
    appendLine(report, "mapA @ " + std::to_string(reinterpret_cast<uintptr_t>(mapA)));
    appendLine(report, "mapB @ " + std::to_string(reinterpret_cast<uintptr_t>(mapB)));
    appendLine(report, "mapA 写入后 mapB 读到: " + std::string(seen));
    appendLine(report, std::strcmp(seen, "written_via_mapA") == 0 ?
                         "页缓存一致性验证通过 ✓" : "验证失败 ✗");

    munmap(mapA, ps);
    munmap(mapB, ps);
    return report;
}

// ---------------------------------------------------------------------------
// 8. mmap 环形缓冲区（单生产者单消费者，power-of-two size）
// ---------------------------------------------------------------------------
struct MmapRingBuffer {
    struct Header {
        std::atomic<uint32_t> head;
        std::atomic<uint32_t> tail;
        uint32_t capacity;
        uint32_t mask;
    };

    Header* header = nullptr;
    uint8_t* data = nullptr;
    size_t totalSize = 0;
    void* base = MAP_FAILED;

    bool init(size_t capacity) {
        const size_t ps = pageSize();
        totalSize = alignUp(sizeof(Header) + capacity, ps);
        base = mmap(nullptr, totalSize, PROT_READ | PROT_WRITE,
                    MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if (base == MAP_FAILED) {
            return false;
        }
        header = static_cast<Header*>(base);
        data = static_cast<uint8_t*>(base) + sizeof(Header);
        new (header) Header{};
        header->capacity = static_cast<uint32_t>(capacity);
        header->mask = static_cast<uint32_t>(capacity - 1);
        return true;
    }

    bool push(uint8_t byte) {
        const uint32_t head = header->head.load(std::memory_order_relaxed);
        const uint32_t next = (head + 1) & header->mask;
        if (next == header->tail.load(std::memory_order_acquire)) {
            return false;  // full
        }
        data[head] = byte;
        header->head.store(next, std::memory_order_release);
        return true;
    }

    bool pop(uint8_t& byte) {
        const uint32_t tail = header->tail.load(std::memory_order_relaxed);
        if (tail == header->head.load(std::memory_order_acquire)) {
            return false;  // empty
        }
        byte = data[tail];
        header->tail.store((tail + 1) & header->mask, std::memory_order_release);
        return true;
    }

    ~MmapRingBuffer() {
        if (base != MAP_FAILED) {
            munmap(base, totalSize);
        }
    }
};

std::string demoMmapRingBuffer() {
    std::string report;
    MmapRingBuffer ring;
    const size_t cap = 256;  // power of 2

    if (!ring.init(cap)) {
        return "ring buffer: mmap failed: " + errnoStr();
    }

    for (uint8_t i = 0; i < 200; ++i) {
        ring.push(i);
    }

    uint32_t sum = 0;
    uint8_t b;
    int count = 0;
    while (ring.pop(b)) {
        sum += b;
        ++count;
    }

    appendLine(report, "=== 8. mmap 环形缓冲区 ===");
    appendLine(report, "capacity=" + std::to_string(cap) +
                         ", total mmap=" + std::to_string(ring.totalSize));
    appendLine(report, "push 200 bytes, pop " + std::to_string(count));
    appendLine(report, "sum(0..199) = " + std::to_string(sum) +
                         " (expected 19900)");
    appendLine(report, sum == 19900 ? "环形缓冲验证通过 ✓" : "验证失败 ✗");

    return report;
}

// ---------------------------------------------------------------------------
// 9. mincore — 查询页是否在物理内存中
// ---------------------------------------------------------------------------
std::string demoMincore() {
    std::string report;
    const size_t ps = pageSize();
    const size_t pages = 4;
    const size_t len = ps * pages;

    void* map = mmap(nullptr, len, PROT_READ | PROT_WRITE,
                     MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (map == MAP_FAILED) {
        return "mincore: mmap failed: " + errnoStr();
    }

    // touch 前两页
    std::memset(map, 1, ps * 2);

    std::vector<unsigned char> vec(pages);
    if (mincore(map, len, vec.data()) != 0) {
        appendLine(report, "mincore failed: " + errnoStr());
        munmap(map, len);
        return report;
    }

    appendLine(report, "=== 9. mincore 页驻留查询 ===");
    for (size_t i = 0; i < pages; ++i) {
        appendLine(report, "page[" + std::to_string(i) + "] in core: " +
                             (vec[i] & 1 ? "yes" : "no"));
    }
    appendLine(report, "（touch 过的页通常为 yes，未 touch 可能为 no）");

    munmap(map, len);
    return report;
}

}  // namespace

std::string runAllMmapDemos(const std::string& filesDir) {
    std::string all;
    appendLine(all, "========== Android mmap 复杂用法演示 ==========");
    appendLine(all, "page size = " + std::to_string(pageSize()) + " bytes");
    appendLine(all, "");

    appendLine(all, demoFileMapShared(filesDir));
    appendLine(all, "");
    appendLine(all, demoMapPrivateCow(filesDir));
    appendLine(all, "");
    appendLine(all, demoAnonymousShared());
    appendLine(all, "");
    appendLine(all, demoMprotect());
    appendLine(all, "");
    appendLine(all, demoMadvise());
    appendLine(all, "");
    appendLine(all, demoPartialMapWithOffset(filesDir));
    appendLine(all, "");
    appendLine(all, demoDualMapping(filesDir));
    appendLine(all, "");
    appendLine(all, demoMmapRingBuffer());
    appendLine(all, "");
    appendLine(all, demoMincore());
    appendLine(all, "");
    appendLine(all, "========== 全部演示完成 ==========");

    return all;
}

std::string runSingleMmapDemo(const std::string& filesDir, int demoId) {
    switch (demoId) {
        case 1: return demoFileMapShared(filesDir);
        case 2: return demoMapPrivateCow(filesDir);
        case 3: return demoAnonymousShared();
        case 4: return demoMprotect();
        case 5: return demoMadvise();
        case 6: return demoPartialMapWithOffset(filesDir);
        case 7: return demoDualMapping(filesDir);
        case 8: return demoMmapRingBuffer();
        case 9: return demoMincore();
        default: return "未知 demo id: " + std::to_string(demoId);
    }
}
