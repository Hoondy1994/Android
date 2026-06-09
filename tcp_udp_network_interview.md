# TCP/UDP 网络编程面试题精讲（含代码）

---

## 目录

1. [TCP vs UDP 核心区别](#1-tcp-vs-udp-核心区别)
2. [TCP 三次握手 / 四次挥手](#2-tcp-三次握手--四次挥手)
3. [TCP 状态机](#3-tcp-状态机)
4. [TIME_WAIT 详解](#4-time_wait-详解)
5. [TCP 粘包 / 拆包](#5-tcp-粘包--拆包)
6. [TCP 可靠传输机制](#6-tcp-可靠传输机制)
7. [TCP 流量控制 vs 拥塞控制](#7-tcp-流量控制-vs-拥塞控制)
8. [Socket API 编程模型](#8-socket-api-编程模型)
9. [IO 多路复用：select / poll / epoll](#9-io-多路复用selectpollepoll)
10. [epoll ET vs LT 模式](#10-epoll-et-vs-lt-模式)
11. [非阻塞 Socket + epoll 实战](#11-非阻塞-socket--epoll-实战)
12. [UDP 可靠传输实现](#12-udp-可靠传输实现)
13. [SO_REUSEADDR / SO_REUSEPORT](#13-so_reuseaddr--so_reuseport)
14. [Nagle 算法 / TCP_NODELAY](#14-nagle-算法--tcp_nodelay)
15. [Keep-Alive 机制](#15-keep-alive-机制)
16. [零拷贝 sendfile](#16-零拷贝-sendfile)
17. [Android 网络编程特殊问题](#17-android-网络编程特殊问题)
18. [高频手撕代码题](#18-高频手撕代码题)

---

## 1. TCP vs UDP 核心区别

| 特性 | TCP | UDP |
|------|-----|-----|
| 连接 | 面向连接（三次握手） | 无连接 |
| 可靠性 | 可靠（确认、重传、排序） | 不可靠 |
| 顺序 | 保证顺序 | 不保证顺序 |
| 流量控制 | 有（滑动窗口） | 无 |
| 拥塞控制 | 有 | 无 |
| 头部大小 | 20~60 字节 | 8 字节 |
| 传输方式 | 字节流 | 数据报文 |
| 速度 | 较慢 | 较快 |
| 适用场景 | HTTP、FTP、SSH | DNS、视频直播、游戏 |

**面试追问：什么时候选 UDP？**
- 实时性要求高，允许少量丢包（语音/视频通话、游戏）
- 广播/多播场景
- 请求-响应极短，三次握手开销不值得（DNS）
- 自己实现可靠机制（QUIC 就是基于 UDP）

---

## 2. TCP 三次握手 / 四次挥手

### 三次握手

```
Client                        Server
  |------- SYN(seq=x) -------->|   LISTEN → SYN_RCVD
  |<-- SYN+ACK(seq=y,ack=x+1) -|   SYN_SENT → ESTABLISHED
  |------- ACK(ack=y+1) ------->|   → ESTABLISHED
```

**为什么是三次而不是两次？**
- 两次握手服务端无法确认客户端的接收能力
- 防止历史失效连接请求到达服务端，造成资源浪费
- 三次握手可以同步双方的初始序列号 ISN

**为什么 ISN 是随机的？**
- 防止被猜测，避免旧连接的数据包被新连接接收（序列号回绕攻击）

### 四次挥手

```
Client                        Server
  |------- FIN(seq=u) --------->|   → CLOSE_WAIT
  |<------- ACK(ack=u+1) -------|   FIN_WAIT_1 → FIN_WAIT_2
  |<------- FIN(seq=v) ---------|   CLOSE_WAIT → LAST_ACK
  |------- ACK(ack=v+1) ------->|   TIME_WAIT(2MSL)
                                    → CLOSED
```

**为什么挥手需要四次而握手三次？**
- 握手时 SYN+ACK 可以合并
- 挥手时服务端收到 FIN，可能还有数据要发送，ACK 和 FIN 必须分开

---

## 3. TCP 状态机

```
关键状态：
LISTEN      → 等待连接
SYN_SENT    → 客户端发出 SYN，等待服务端 SYN+ACK
SYN_RCVD    → 服务端收到 SYN，发出 SYN+ACK
ESTABLISHED → 连接已建立
FIN_WAIT_1  → 主动关闭，发出 FIN
FIN_WAIT_2  → 收到 ACK，等待对端 FIN
CLOSE_WAIT  → 被动关闭，收到 FIN，等待应用关闭
LAST_ACK    → 被动关闭，发出 FIN，等待最终 ACK
TIME_WAIT   → 主动关闭，等待 2MSL
CLOSED      → 关闭完成
```

**面试高频：服务端出现大量 CLOSE_WAIT 的原因？**
- 应用层没有调用 `close()`，通常是代码 bug（连接对象未释放）

---

## 4. TIME_WAIT 详解

**为什么需要 2MSL 的 TIME_WAIT？**

1. **保证最后一个 ACK 能到达对端**：若 ACK 丢失，对端会重发 FIN，Client 需要能重新发 ACK
2. **让旧连接的报文在网络中消亡**：防止新连接收到旧连接的数据

**TIME_WAIT 过多的危害与解决**

```cpp
// 危害：占用端口资源，导致新连接无法建立

// 解决方案1：SO_REUSEADDR（允许绑定 TIME_WAIT 状态的端口）
int opt = 1;
setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

// 解决方案2：开启 tcp_tw_reuse（仅用于客户端主动连接）
// /proc/sys/net/ipv4/tcp_tw_reuse = 1

// 解决方案3：减小 MSL 时间（不推荐，影响协议安全性）
```

**哪端会进入 TIME_WAIT？**
- 主动发起 FIN 的一端（通常是客户端，但服务端主动关闭也会）

---

## 5. TCP 粘包 / 拆包

**本质原因：TCP 是字节流，没有消息边界。**

| 现象 | 原因 |
|------|------|
| 粘包 | 发送方 Nagle 合并、接收方读取不及时 |
| 拆包 | 单条消息超过 MSS，被分片发送 |

### 解决方案

```cpp
// 方案1：固定长度消息
struct FixedMsg {
    char data[256];
};

// 方案2：分隔符（如 \n、\r\n）
// HTTP/1.1 头部使用此方案

// 方案3：TLV（Type-Length-Value）协议头 ← 最常用
struct MsgHeader {
    uint32_t magic;   // 魔数 0xDEADBEEF
    uint32_t type;    // 消息类型
    uint32_t length;  // payload 长度
};

// 接收端完整读取
bool readFull(int fd, void* buf, size_t len) {
    size_t received = 0;
    while (received < len) {
        ssize_t n = recv(fd, (char*)buf + received, len - received, 0);
        if (n <= 0) return false;
        received += n;
    }
    return true;
}
```

---

## 6. TCP 可靠传输机制

### 序列号 + 确认号
- 每个字节都有序列号，接收方发送 ACK 确认
- 累积确认：ACK=N 表示 N 之前的数据都已收到

### 超时重传
```
RTT（往返时延）→ RTO（重传超时时间）
RTO = SRTT + 4 * RTTVAR  （RFC 6298）
每次超时 RTO 翻倍（指数退避），上限通常 120s
```

### 快速重传
- 收到 3 个重复 ACK → 立即重传，无需等待 RTO

### SACK（选择性确认）
```
// 允许接收方告知发送方哪些段已收到（非连续）
// 避免重传已收到的数据
TCP Option: SACK [1000-2000, 3000-4000]
```

### 滑动窗口
```
|-- 已发已确认 --|-- 已发未确认 --|-- 可发未发 --|-- 不可发 --|
               ^                ^
           SND.UNA           SND.NXT
               |<----- 窗口 ------>|
```

---

## 7. TCP 流量控制 vs 拥塞控制

### 流量控制（端到端）
- **目的**：防止发送方发太快，超过接收方处理能力
- **机制**：接收方通过 TCP 头的 `rwnd`（接收窗口）告知发送方可用缓冲区大小
- **零窗口**：`rwnd=0` 时发送方停止，通过探测包等待恢复

### 拥塞控制（发送方自我约束）

```
cwnd（拥塞窗口）控制实际发送量 = min(cwnd, rwnd)

阶段1 慢启动：cwnd 从1开始，每个 RTT 翻倍（指数增长）
阶段2 拥塞避免：cwnd > ssthresh 后，每 RTT 加1（线性增长）
阶段3 快速恢复（收到3个重复ACK）：
       ssthresh = cwnd/2, cwnd = ssthresh + 3

超时时：ssthresh = cwnd/2, cwnd = 1，重新慢启动
```

**面试区别总结：**
- 流量控制 = 接收方说"我能收多少"
- 拥塞控制 = 发送方猜"网络能承受多少"

---

## 8. Socket API 编程模型

### TCP Server

```cpp
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int tcp_server(int port) {
    // 1. 创建 socket
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    // 2. 地址复用（必须在 bind 之前）
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 3. 绑定
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    bind(listenfd, (sockaddr*)&addr, sizeof(addr));

    // 4. 监听（backlog = 全连接队列大小）
    listen(listenfd, 128);

    // 5. 接受连接
    struct sockaddr_in client{};
    socklen_t len = sizeof(client);
    int connfd = accept(listenfd, (sockaddr*)&client, &len);

    return connfd;
}
```

### TCP Client

```cpp
int tcp_client(const char* ip, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    connect(fd, (sockaddr*)&addr, sizeof(addr));
    return fd;
}
```

### UDP Server / Client

```cpp
// Server
int udp_server(int port) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    bind(fd, (sockaddr*)&addr, sizeof(addr));
    return fd;
}

// 收发
struct sockaddr_in peer{};
socklen_t plen = sizeof(peer);
char buf[1024];
// 接收
recvfrom(fd, buf, sizeof(buf), 0, (sockaddr*)&peer, &plen);
// 发送
sendto(fd, buf, strlen(buf), 0, (sockaddr*)&peer, plen);
```

**面试陷阱：UDP 服务端要不要 connect？**
- UDP `connect()` 只是记录对端地址，之后可用 `send/recv` 代替 `sendto/recvfrom`
- 好处：内核过滤其他来源的包，性能略优

---

## 9. IO 多路复用：select / poll / epoll

| 特性 | select | poll | epoll |
|------|--------|------|-------|
| fd 数量限制 | 1024（FD_SETSIZE） | 无限制 | 无限制 |
| 时间复杂度 | O(n) | O(n) | O(1) |
| 内存拷贝 | 每次全量拷贝 | 每次全量拷贝 | 仅注册时拷贝 |
| 实现 | fd_set 位图 | pollfd 数组 | 红黑树 + 就绪链表 |
| 跨平台 | POSIX 标准 | POSIX 标准 | Linux 专有 |

### epoll 核心 API

```cpp
// 创建 epoll 实例（内核红黑树）
int epfd = epoll_create1(0);

// 注册/修改/删除监听事件
struct epoll_event ev;
ev.events = EPOLLIN | EPOLLET;  // 读事件 + 边沿触发
ev.data.fd = connfd;
epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);

// 等待事件（替代 select/poll）
struct epoll_event events[1024];
int nready = epoll_wait(epfd, events, 1024, -1);  // -1 永久阻塞

for (int i = 0; i < nready; i++) {
    if (events[i].data.fd == listenfd) {
        // 新连接
    } else if (events[i].events & EPOLLIN) {
        // 可读
    }
}
```

---

## 10. epoll ET vs LT 模式

| | LT（水平触发，默认） | ET（边沿触发） |
|--|--|--|
| 触发时机 | 缓冲区有数据就一直触发 | 数据到来时仅触发一次 |
| 读取要求 | 可以分多次读 | 必须一次性读完（循环到 EAGAIN） |
| 性能 | 较低（重复通知） | 较高（通知更少） |
| 编程复杂度 | 简单 | 复杂（必须非阻塞）|

### ET 模式正确写法

```cpp
// ET 模式下必须：1. 非阻塞 fd  2. 循环读到 EAGAIN
void handle_read_et(int fd) {
    char buf[4096];
    while (true) {
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n > 0) {
            // 处理数据
        } else if (n == -1 && errno == EAGAIN) {
            break;  // 数据读完了
        } else if (n == 0 || (n == -1 && errno != EINTR)) {
            close(fd);  // 对端关闭或出错
            break;
        }
    }
}

// 设置非阻塞
void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
```

---

## 11. 非阻塞 Socket + epoll 实战

```cpp
#include <sys/epoll.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

class EpollServer {
    int listenfd_, epfd_;
    static const int MAX_EVENTS = 1024;

    void setNonBlock(int fd) {
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
    }

    void addFd(int fd, bool et = true) {
        epoll_event ev{};
        ev.data.fd = fd;
        ev.events = EPOLLIN | (et ? EPOLLET : 0);
        epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev);
    }

public:
    EpollServer(int port) {
        listenfd_ = socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        bind(listenfd_, (sockaddr*)&addr, sizeof(addr));
        listen(listenfd_, 128);

        epfd_ = epoll_create1(0);
        setNonBlock(listenfd_);
        addFd(listenfd_, false);  // 监听 fd 用 LT 即可
    }

    void run() {
        epoll_event events[MAX_EVENTS];
        while (true) {
            int n = epoll_wait(epfd_, events, MAX_EVENTS, -1);
            for (int i = 0; i < n; i++) {
                int fd = events[i].data.fd;
                if (fd == listenfd_) {
                    // 接受新连接（循环 accept）
                    while (true) {
                        int connfd = accept(listenfd_, nullptr, nullptr);
                        if (connfd < 0) break;
                        setNonBlock(connfd);
                        addFd(connfd, true);  // ET 模式
                    }
                } else if (events[i].events & EPOLLIN) {
                    handle_read(fd);
                }
            }
        }
    }

    void handle_read(int fd) {
        char buf[4096];
        while (true) {
            ssize_t n = recv(fd, buf, sizeof(buf), 0);
            if (n > 0) {
                send(fd, buf, n, 0);  // echo
            } else if (n == 0 || (n < 0 && errno != EAGAIN && errno != EINTR)) {
                epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
                close(fd);
                break;
            } else {
                break;  // EAGAIN，数据读完
            }
        }
    }
};
```

---

## 12. UDP 可靠传输实现

**面试题：如何基于 UDP 实现可靠传输？（QUIC/KCP 原理）**

核心要素：
1. **序列号** - 标识每个报文
2. **ACK 确认** - 接收端回复确认
3. **超时重传** - 未收到 ACK 则重传
4. **去重** - 接收端过滤重复包
5. **乱序重排** - 缓存乱序包，按序交付
6. **滑动窗口** - 控制发送速率

```cpp
struct UdpPacket {
    uint32_t seq;       // 序列号
    uint32_t ack;       // 确认号
    uint16_t flags;     // ACK=0x01, SYN=0x02, FIN=0x04
    uint16_t len;       // 数据长度
    char     data[0];   // payload
};

// 接收端伪代码
void on_recv(UdpPacket* pkt) {
    if (recv_window.contains(pkt->seq)) return;  // 去重
    recv_buffer[pkt->seq] = pkt;
    send_ack(pkt->seq + pkt->len);

    // 按序交付
    while (recv_buffer.count(next_expected)) {
        deliver(recv_buffer[next_expected]);
        next_expected++;
    }
}
```

---

## 13. SO_REUSEADDR / SO_REUSEPORT

```cpp
// SO_REUSEADDR：
// 1. 允许 bind 处于 TIME_WAIT 状态的端口（服务器重启常用）
// 2. 允许多个 socket bind 同一端口（但不同本地IP）
int opt = 1;
setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

// SO_REUSEPORT（Linux 3.9+）：
// 允许多个进程/线程 bind 完全相同的 IP:Port
// 内核负载均衡分发连接，避免惊群问题
// Nginx worker 进程就用此方式
setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
```

**区别总结：**
- `SO_REUSEADDR`：解决 TIME_WAIT 端口占用，单进程场景
- `SO_REUSEPORT`：多进程共同监听同一端口，内核分流

---

## 14. Nagle 算法 / TCP_NODELAY

**Nagle 算法：** 积累小包合并发送，减少网络包数量
- 条件：已发出的数据没有全部被 ACK，则小包暂时缓冲

**何时关闭 Nagle？**
- 低延迟场景（游戏、远程桌面、股票行情）
- 与 `TCP_QUICKACK` 配合使用

```cpp
// 关闭 Nagle 算法，小包立即发送
int flag = 1;
setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

// 关闭延迟 ACK
setsockopt(fd, IPPROTO_TCP, TCP_QUICKACK, &flag, sizeof(flag));
```

**面试陷阱：写-写-读 模式的延迟问题**
```
应用连续调用两次 write（小包），若未设 TCP_NODELAY：
write(fd, head, 4);   // 等待前一个 ACK
write(fd, body, 100); // 等 40ms 延迟 ACK + Nagle 导致延迟
```

---

## 15. Keep-Alive 机制

**TCP Keep-Alive：** 检测空闲连接是否还存活

```cpp
// 开启 Keep-Alive
int keepalive = 1;
setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));

// 空闲 60 秒后开始探测
int idle = 60;
setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));

// 探测间隔 5 秒
int interval = 5;
setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));

// 探测 3 次无响应则关闭
int count = 3;
setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));
```

**应用层心跳 vs TCP Keep-Alive**
- TCP Keep-Alive：只能检测连接是否存活，不能检测应用是否正常
- 应用层心跳：可以检测应用逻辑是否健康（推荐在生产中使用）

---

## 16. 零拷贝 sendfile

**传统文件发送（4次拷贝）：**
```
磁盘 → 内核缓冲区 → 用户缓冲区 → Socket缓冲区 → 网卡
       read()                    write()
```

**sendfile 零拷贝（2次拷贝）：**
```
磁盘 → 内核缓冲区 → 网卡（DMA直接传输，无需用户态拷贝）
```

```cpp
#include <sys/sendfile.h>

// 将文件直接发送到 socket
off_t offset = 0;
sendfile(sockfd, filefd, &offset, file_size);
```

**相关优化：**
- `mmap + write`：减少到 3 次拷贝
- `splice`：管道内核零拷贝
- `MSG_ZEROCOPY`（Linux 4.14+）：发送时不拷贝用户缓冲区

---

## 17. Android 网络编程特殊问题

### Android 不允许主线程网络操作
```java
// 抛出 NetworkOnMainThreadException
// 原因：StrictMode 策略，防止 ANR
// 解决：AsyncTask / Thread / Coroutine / RxJava
```

### Android 权限
```xml
<uses-permission android:name="android.permission.INTERNET"/>
<uses-permission android:name="android.permission.ACCESS_NETWORK_STATE"/>
```

### Native Socket（NDK）在 Android 上注意事项
```cpp
// Android 6.0+ 限制了部分网络 syscall
// 推荐使用 AF_INET，避免 AF_UNIX 的某些用法
// 注意 IPv6 支持（Android 要求 IPv6 兼容）

// 检查网络连接类型（JNI 调用 Java API）
// ConnectivityManager.getActiveNetworkInfo()
```

### UNIX Domain Socket（本地 IPC）
```cpp
// Android Binder 替代方案，高性能本地通信
struct sockaddr_un addr;
addr.sun_family = AF_UNIX;
strcpy(addr.sun_path, "/data/local/tmp/myapp.sock");
```

---

## 18. 高频手撕代码题

### Q1：实现一个简单的 TCP Echo Server（单线程 + epoll）

见第 11 节的 `EpollServer` 实现。

---

### Q2：封装 readn / writen（解决粘包）

```cpp
// 确保读取 n 字节
ssize_t readn(int fd, void* buf, size_t n) {
    size_t nleft = n;
    ssize_t nread;
    char* ptr = static_cast<char*>(buf);
    while (nleft > 0) {
        nread = read(fd, ptr, nleft);
        if (nread < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (nread == 0) break;  // EOF
        nleft -= nread;
        ptr   += nread;
    }
    return n - nleft;
}

// 确保写入 n 字节
ssize_t writen(int fd, const void* buf, size_t n) {
    size_t nleft = n;
    ssize_t nwritten;
    const char* ptr = static_cast<const char*>(buf);
    while (nleft > 0) {
        nwritten = write(fd, ptr, nleft);
        if (nwritten <= 0) {
            if (nwritten < 0 && errno == EINTR) continue;
            return -1;
        }
        nleft -= nwritten;
        ptr   += nwritten;
    }
    return n;
}
```

---

### Q3：UDP 广播

```cpp
// 发送广播
int fd = socket(AF_INET, SOCK_DGRAM, 0);
int broadcast = 1;
setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

sockaddr_in addr{};
addr.sin_family = AF_INET;
addr.sin_port = htons(8888);
inet_pton(AF_INET, "255.255.255.255", &addr.sin_addr);  // 受限广播

const char* msg = "Hello, LAN!";
sendto(fd, msg, strlen(msg), 0, (sockaddr*)&addr, sizeof(addr));
```

---

### Q4：非阻塞 connect

```cpp
int nonblock_connect(const char* ip, int port, int timeout_ms) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    int ret = connect(fd, (sockaddr*)&addr, sizeof(addr));
    if (ret == 0) return fd;  // 立即成功（本地回环）
    if (errno != EINPROGRESS) { close(fd); return -1; }

    // 用 select/epoll 等待连接完成
    fd_set wset;
    FD_ZERO(&wset);
    FD_SET(fd, &wset);
    timeval tv{ timeout_ms / 1000, (timeout_ms % 1000) * 1000 };

    if (select(fd + 1, nullptr, &wset, nullptr, &tv) <= 0) {
        close(fd); return -1;  // 超时或出错
    }

    // 检查连接是否真的成功
    int err = 0;
    socklen_t len = sizeof(err);
    getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
    if (err != 0) { close(fd); return -1; }

    // 恢复阻塞模式（可选）
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK);
    return fd;
}
```

---

### Q5：端口扫描（TCP connect 扫描）

```cpp
#include <future>
#include <vector>

bool is_port_open(const char* ip, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    // 设置超时
    timeval tv{1, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    bool open = (connect(fd, (sockaddr*)&addr, sizeof(addr)) == 0);
    close(fd);
    return open;
}
```

---

## 常见面试问题快速回答

**Q: listen 的 backlog 参数是什么？**
> backlog 是全连接队列（accept queue）的最大长度。还有半连接队列（SYN queue）由 `tcp_max_syn_backlog` 控制。队列满时新的 SYN 会被丢弃，导致客户端重传。

**Q: accept 返回的 fd 和 listen fd 有什么区别？**
> listenfd 是监听套接字，只用于接受连接，生命周期等于服务器运行时间。connfd 是已连接套接字，每个连接一个，用于实际的数据传输。

**Q: close() 和 shutdown() 的区别？**
> `close(fd)` 关闭文件描述符，引用计数减一，归零才真正关闭。`shutdown(fd, SHUT_WR)` 立即发送 FIN，不受引用计数影响，可实现半关闭（单向关闭）。

**Q: 服务端一般用什么方式处理并发连接？**
> 1. 多进程 `fork`（Apache prefork）  
> 2. 多线程（一连接一线程）  
> 3. IO 多路复用（epoll + 事件循环，Nginx/Redis）  
> 4. 线程池 + epoll（生产中最常见）

**Q: TCP 连接建立后，客户端进程崩溃会怎样？**
> OS 会自动发送 RST/FIN 包。若网络中断，服务端不会立即感知，直到超时或 Keep-Alive 探测失败。

**Q: UDP 能保证有序吗？**
> 不能。IP 层路由可能走不同路径，UDP 报文到达顺序不确定。需要应用层用序列号排序。
