# socketipc 项目完整流程文档

> Binder vs Socket IPC 对比 Demo。本文档列举所有主要流程，包含涉及的**文件**与**函数**调用链。

---

## 目录

1. [项目概览](#1-项目概览)
2. [文件索引](#2-文件索引)
3. [App 启动流程](#3-app-启动流程)
4. [Socket 服务绑定流程](#4-socket-服务绑定流程)
5. [Binder 服务绑定流程](#5-binder-服务绑定流程)
6. [Socket 服务端启动流程](#6-socket-服务端启动流程)
7. [Socket 就绪检测流程](#7-socket-就绪检测流程)
8. [Socket Add 流程](#8-socket-add-流程)
9. [Binder Add 流程](#9-binder-add-流程)
10. [Socket 复杂对象流程](#10-socket-复杂对象流程)
11. [Binder 复杂对象流程](#11-binder-复杂对象流程)
12. [序列化压测流程](#12-序列化压测流程)
13. [Socket IPC 压测流程](#13-socket-ipc-压测流程)
14. [Binder IPC 压测流程](#14-binder-ipc-压测流程)
15. [App 销毁流程](#15-app-销毁流程)
16. [Socket 协议与线程模型](#16-socket-协议与线程模型)
17. [JNI 函数对照表](#17-jni-函数对照表)

---

## 1. 项目概览

### 1.1 三进程架构

| 进程 | 组件 | 角色 |
|------|------|------|
| **主进程** | `MainActivity` | UI + Socket 客户端 + Binder 客户端 |
| **`:socket_remote`** | `SocketServerService` | Socket 服务端（Native C++） |
| **`:binder_remote`** | `CalculatorService` | Binder 服务端（AIDL Stub） |

### 1.2 两条 IPC 通道

```
主进程                          :socket_remote
  SocketClient ──AF_UNIX──→     SocketServer
  (socketipc_calc)

主进程                          :binder_remote
  ICalculator 代理 ──Binder──→  ICalculator.Stub
  (/dev/binder)
```

### 1.3 Native 库

- 库名：`libmyapplication.so`
- 构建：`app/src/main/cpp/CMakeLists.txt`
- 加载点：`MainActivity`、`SocketServerService`、`NativeIpc` 的 `companion object init`

---

## 2. 文件索引

### 2.1 Java / Kotlin 层

| 文件 | 说明 |
|------|------|
| `app/src/main/AndroidManifest.xml` | 声明三进程 Service |
| `app/src/main/java/.../MainActivity.kt` | 主界面、绑定服务、按钮触发 IPC |
| `app/src/main/java/.../ipc/SocketServerService.kt` | Socket 服务端 Android 壳 |
| `app/src/main/java/.../ipc/CalculatorService.kt` | Binder 服务端 |
| `app/src/main/java/.../ipc/NativeIpc.kt` | Binder 服务端 Native 编解码 JNI |
| `app/src/main/java/.../ipc/BinderBenchmark.kt` | Binder 多线程压测 |
| `app/src/main/aidl/.../ICalculator.aidl` | Binder 接口定义 |

### 2.2 Native C++ 层

| 文件 | 说明 |
|------|------|
| `app/src/main/cpp/jni/jni_bridge.cpp` | 全部 JNI 入口 |
| `app/src/main/cpp/src/jni_util.cpp` | JNI ↔ C++ 类型转换 |
| `app/src/main/cpp/src/socket_client.cpp` | Socket 客户端 |
| `app/src/main/cpp/src/socket_server.cpp` | Socket 服务端 |
| `app/src/main/cpp/src/socket_protocol.cpp` | 请求/响应帧编解码 |
| `app/src/main/cpp/src/custom_codec.cpp` | 自定义二进制序列化 |
| `app/src/main/cpp/src/parcel_codec.cpp` | 仿 Parcel 布局序列化 |
| `app/src/main/cpp/src/types.cpp` | `UserProfile` 数据模型 |
| `app/src/main/cpp/src/benchmark.cpp` | 序列化 & Socket 压测 |

### 2.3 头文件

| 文件 | 说明 |
|------|------|
| `include/ipc/socket_protocol.hpp` | `Opcode`、`Frame`、`Response` |
| `include/ipc/socket_client.hpp` | `SocketClient` |
| `include/ipc/socket_server.hpp` | `SocketServer` |
| `include/ipc/custom_codec.hpp` | `CustomCodec` |
| `include/ipc/parcel_codec.hpp` | `ParcelCodec` |
| `include/ipc/types.hpp` | `UserProfile`、`BenchmarkStats` |
| `include/ipc/benchmark.hpp` | 压测枚举与入口 |
| `include/ipc/binary_buffer.hpp` | Custom 编解码读写器 |
| `include/jni/jni_util.hpp` | JNI 工具函数声明 |

---

## 3. App 启动流程

**触发**：用户打开 App → 系统启动 `MainActivity`

```
MainActivity.onCreate(savedInstanceState)
├── super.onCreate(savedInstanceState)
├── ActivityMainBinding.inflate(layoutInflater)
├── setContentView(binding.root)
├── binding.tvProcessInfo.text = "主进程 PID: ..."     // Process.myPid()
├── startRemoteServices()                              // → 流程 4
├── bindCalculatorService()                            // → 流程 5
└── 注册 9 个按钮 setOnClickListener
    ├── btnBinderCalc      → runBinderAdd()
    ├── btnSocketCalc      → runSocketAdd()
    ├── btnBinderCustom    → runBinderComplex(false)
    ├── btnBinderParcel    → runBinderComplex(true)
    ├── btnSocketCustom    → runSocketComplex(false)
    ├── btnSocketParcel    → runSocketComplex(true)
    ├── btnBenchSerialization → runSerializationBenchmark()
    ├── btnBenchBinder     → runBinderBenchmark()
    └── btnBenchSocket     → runSocketBenchmark()
```

**文件**：`MainActivity.kt`

**说明**：`onCreate` 结束时界面已显示，远程服务绑定是**异步**的，尚未就绪。

---

## 4. Socket 服务绑定流程

**目的**：拉起 `:socket_remote` 进程，启动 Native Socket 服务端

```
MainActivity.startRemoteServices()
└── bindService(
        Intent(this, SocketServerService::class.java),
        socketServiceConnection,
        Context.BIND_AUTO_CREATE
    )
    └── Log.d(TAG, "[1] bindService 已调用...")        // 同步，立刻返回

（系统异步，:socket_remote 进程）
SocketServerService.onCreate()                         // → 流程 6
SocketServerService.onBind(intent)
└── return localBinder

（主进程异步回调）
socketServiceConnection.onServiceConnected(name, service)
├── Log.d(TAG, "[4] onServiceConnected...")
├── socketServiceBound = true
└── waitForSocketServerAsync()                         // → 流程 7
```

| 步骤 | 文件 | 函数 |
|------|------|------|
| 发起绑定 | `MainActivity.kt` | `startRemoteServices()` |
| 系统创建 Service | `SocketServerService.kt` | `onCreate()` |
| 返回 Binder | `SocketServerService.kt` | `onBind()` |
| 绑定成功回调 | `MainActivity.kt` | `socketServiceConnection.onServiceConnected()` |

**Logcat 顺序**：`[1]` → `[2]` → `[3]` → `[4]`

---

## 5. Binder 服务绑定流程

**目的**：拉起 `:binder_remote` 进程，获取 `ICalculator` 代理

```
MainActivity.bindCalculatorService()
└── bindService(
        Intent(this, CalculatorService::class.java),
        binderServiceConnection,
        Context.BIND_AUTO_CREATE
    )

（系统异步，:binder_remote 进程）
CalculatorService.onBind(intent)
└── return binder                                    // ICalculator.Stub 实例

（主进程异步回调）
binderServiceConnection.onServiceConnected(name, service)
├── calculator = ICalculator.Stub.asInterface(service)
├── binderBound = true
└── appendLog("Binder 服务已连接 (:binder_remote)")
```

| 步骤 | 文件 | 函数 |
|------|------|------|
| 发起绑定 | `MainActivity.kt` | `bindCalculatorService()` |
| 返回 Stub | `CalculatorService.kt` | `onBind()` → `binder` |
| 转为代理 | `MainActivity.kt` | `ICalculator.Stub.asInterface(service)` |
| 绑定成功回调 | `MainActivity.kt` | `binderServiceConnection.onServiceConnected()` |

**AIDL 接口**（`ICalculator.aidl`）：

- `add(int a, int b)`
- `getProtocol()`
- `processCustomPayload(byte[] payload)`
- `processParcelPayload(byte[] payload)`

---

## 6. Socket 服务端启动流程

**触发**：`:socket_remote` 进程中 `SocketServerService.onCreate()`

```
SocketServerService.onCreate()
├── super.onCreate()
├── Log.d(TAG, "[2] onCreate: pid=...")
└── nativeStartSocketServer()
    └── jni_bridge.cpp
        Java_..._SocketServerService_nativeStartSocketServer()
        └── ipc::global_socket_server().start("socketipc_calc")
            └── socket_server.cpp
                SocketServer::start(socket_name)
                ├── running_ 已为 true → return true
                ├── std::thread([this]{ loop(); }).detach()    // 监听线程
                └── 轮询 200×10ms 等待 running_ == true

                SocketServer::loop()                           // detach 线程中
                ├── socket(AF_UNIX, SOCK_STREAM, 0)
                ├── setup_abstract_address("socketipc_calc")
                ├── bind(server_fd_, addr)
                ├── listen(server_fd_, 64)
                ├── running_.store(true)
                └── while (running_)
                    ├── accept(server_fd_) → client_fd
                    └── std::thread(handle_client, client_fd).detach()
```

| 步骤 | 文件 | 函数 |
|------|------|------|
| Kotlin 入口 | `SocketServerService.kt` | `onCreate()` → `nativeStartSocketServer()` |
| JNI | `jni_bridge.cpp` | `Java_..._nativeStartSocketServer()` |
| 启动监听 | `socket_server.cpp` | `global_socket_server().start()` |
| 后台监听 | `socket_server.cpp` | `SocketServer::loop()` |
| 处理连接 | `socket_server.cpp` | `handle_client(client_fd)` |

**Socket 地址**：Linux 抽象命名空间 UDS，名称 `socketipc_calc`（`sun_path[0]='\0'`）

---

## 7. Socket 就绪检测流程

**触发**：`onServiceConnected` 之后（Service 绑定 ≠ Socket 已 listen）

```
MainActivity.waitForSocketServerAsync()
└── backgroundExecutor.execute { ... }                 // 后台单线程
    └── nativeWaitForSocketServer(10_000)
        └── jni_bridge.cpp
            Java_..._MainActivity_nativeWaitForSocketServer()
            └── global_socket_client().ping()          // 每 50ms 重试，最多 10s
                └── socket_client.cpp
                    SocketClient::ping()
                    └── transact(Frame{opcode=kPing})
                        └── socket_server.cpp
                            handle_frame() case kPing
                            └── response.payload = {'P','O','N','G'}

    └── runOnUiThread { appendLog("Socket 服务已就绪" / "连接超时") }
```

| 步骤 | 文件 | 函数 |
|------|------|------|
| 异步等待 | `MainActivity.kt` | `waitForSocketServerAsync()` |
| JNI | `jni_bridge.cpp` | `nativeWaitForSocketServer()` |
| 客户端 ping | `socket_client.cpp` | `SocketClient::ping()` → `transact()` |
| 服务端响应 | `socket_server.cpp` | `handle_frame()` → `Opcode::kPing` |

---

## 8. Socket Add 流程

**触发**：点击 `btnSocketCalc` → `runSocketAdd()`

```
MainActivity.runSocketAdd()
├── readInputNumbers() → (a, b)
└── nativeSocketAdd(a, b)
    └── jni_bridge.cpp
        Java_..._MainActivity_nativeSocketAdd()
        └── global_socket_client().add(a, b)
            └── socket_client.cpp
                SocketClient::add(a, b)
                ├── 构造 Frame{opcode=kAdd, payload=[a,b]}
                └── transact(frame)
                    ├── socket(AF_UNIX) + connect("socketipc_calc")
                    ├── SocketProtocol::encode_request(frame)
                    ├── SocketProtocol::write_fully(fd, encoded)
                    ├── SocketProtocol::read_fully(fd)
                    ├── SocketProtocol::decode_response(buffer)
                    └── close(fd)

            （:socket_remote 进程，并行）
            SocketServer::loop() → accept() → handle_client(client_fd)
            ├── SocketProtocol::read_fully(client_fd)
            ├── SocketProtocol::decode_request(buffer)
            ├── handle_frame(request)
            │   └── case Opcode::kAdd: sum = a + b
            ├── SocketProtocol::encode_response(response)
            ├── SocketProtocol::write_fully(client_fd, encoded)
            └── close(client_fd)

    └── appendLog("Socket Add: a + b = result (X ms)")
```

| 层级 | 文件 | 函数 |
|------|------|------|
| UI | `MainActivity.kt` | `runSocketAdd()` |
| JNI | `jni_bridge.cpp` | `nativeSocketAdd()` |
| 客户端 | `socket_client.cpp` | `SocketClient::add()` → `transact()` |
| 协议 | `socket_protocol.cpp` | `encode_request` / `decode_response` / `read_fully` / `write_fully` |
| 服务端 | `socket_server.cpp` | `handle_client()` → `handle_frame()` |

---

## 9. Binder Add 流程

**触发**：点击 `btnBinderCalc` → `runBinderAdd()`

```
MainActivity.runBinderAdd()
├── readInputNumbers() → (a, b)
├── ensureBinderReady()                                // binderBound && calculator != null
└── calculator!!.add(a, b)                             // ICalculator 代理，跨进程
    └── （AIDL 生成代码，经 Binder 驱动）
        └── :binder_remote 进程
            CalculatorService.binder.add(a, b)
            └── return a + b

    └── appendLog("Binder Add: a + b = result (X ms)")
```

| 层级 | 文件 | 函数 |
|------|------|------|
| UI | `MainActivity.kt` | `runBinderAdd()` |
| 代理调用 | AIDL 生成 `ICalculator` | `add(a, b)` |
| 服务端 Stub | `CalculatorService.kt` | `binder.add(a, b)` |
| 接口定义 | `ICalculator.aidl` | `int add(in int a, in int b)` |

---

## 10. Socket 复杂对象流程

### 10.1 Socket Custom

**触发**：点击 `btnSocketCustom` → `runSocketComplex(false)`

```
MainActivity.runSocketComplex(false)
└── nativeSocketProcessCustom()
    └── jni_bridge.cpp
        Java_..._MainActivity_nativeSocketProcessCustom()
        └── run_payload_demo(env, lambda, "Socket Custom")
            ├── CustomCodec::encode(UserProfile::sample(100))     // types.cpp + custom_codec.cpp
            ├── global_socket_client().process_custom(encoded)
            │   └── transact(Frame{opcode=kProcessCustom, payload})
            └── CustomCodec::decode(processed) → 拼描述字符串

            （:socket_remote）
            handle_frame() case kProcessCustom
            └── CustomCodec::process(frame.payload)
                └── encode(decode(data).transformed())
                    └── UserProfile::transformed()                // types.cpp
```

### 10.2 Socket Parcel

**触发**：点击 `btnSocketParcel` → `runSocketComplex(true)`

```
MainActivity.runSocketComplex(true)
└── nativeSocketProcessParcel()
    └── jni_bridge.cpp
        Java_..._MainActivity_nativeSocketProcessParcel()
        ├── ParcelCodec::encode(UserProfile::sample(100))
        ├── global_socket_client().process_parcel(encoded)
        │   └── transact(Frame{opcode=kProcessParcel, payload})
        └── ParcelCodec::decode(processed)

        （:socket_remote）
        handle_frame() case kProcessParcel
        └── ParcelCodec::process(frame.payload)
```

| 步骤 | 文件 | 函数 |
|------|------|------|
| UI | `MainActivity.kt` | `runSocketComplex()` |
| JNI Custom | `jni_bridge.cpp` | `nativeSocketProcessCustom()` |
| JNI Parcel | `jni_bridge.cpp` | `nativeSocketProcessParcel()` |
| 客户端 | `socket_client.cpp` | `process_custom()` / `process_parcel()` |
| 编解码 | `custom_codec.cpp` / `parcel_codec.cpp` | `encode` / `decode` / `process` |
| 数据变换 | `types.cpp` | `UserProfile::sample()` / `transformed()` |
| 服务端 | `socket_server.cpp` | `handle_frame()` |

---

## 11. Binder 复杂对象流程

**触发**：点击 `btnBinderCustom` / `btnBinderParcel`

### 11.1 Binder Custom

```
MainActivity.runBinderComplex(false)
├── nativeBuildCustomPayload()
│   └── jni_bridge.cpp → CustomCodec::encode(UserProfile::sample(100))
├── calculator!!.processCustomPayload(payload)         // 跨进程 Binder
│   └── :binder_remote
│       CalculatorService.binder.processCustomPayload(payload)
│       └── NativeIpc.nativeProcessCustomPayload(payload)
│           └── jni_bridge.cpp
│               Java_..._NativeIpc_nativeProcessCustomPayload()
│               └── CustomCodec::process(bytes)
├── nativeVerifyCustomPayload(result)
└── NativeIpc.nativeDescribePayload(result, false)
```

### 11.2 Binder Parcel

```
MainActivity.runBinderComplex(true)
├── nativeBuildParcelPayload()
│   └── jni_bridge.cpp → ParcelCodec::encode(UserProfile::sample(100))
├── calculator!!.processParcelPayload(payload)
│   └── :binder_remote
│       CalculatorService.binder.processParcelPayload(payload)
│       └── NativeIpc.nativeProcessParcelPayload(payload)
│           └── ParcelCodec::process(bytes)
├── nativeVerifyParcelPayload(result)
└── NativeIpc.nativeDescribePayload(result, true)
```

| 步骤 | 文件 | 函数 |
|------|------|------|
| UI | `MainActivity.kt` | `runBinderComplex()` |
| 构建 payload | `jni_bridge.cpp` | `nativeBuildCustomPayload()` / `nativeBuildParcelPayload()` |
| Binder 调用 | `CalculatorService.kt` | `processCustomPayload()` / `processParcelPayload()` |
| Native 处理 | `jni_bridge.cpp` | `NativeIpc.nativeProcessCustomPayload()` 等 |
| 编解码 | `custom_codec.cpp` / `parcel_codec.cpp` | `process()` |

---

## 12. 序列化压测流程

**触发**：点击 `btnBenchSerialization` → `runSerializationBenchmark()`

**特点**：纯本地多线程，**不涉及 IPC**

```
MainActivity.runSerializationBenchmark()
├── readBenchmarkConfig() → (threads, iterations)
├── setButtonsEnabled(false)
└── backgroundExecutor.execute {
        nativeRunSerializationBenchmark(threads, iterations, false)   // Custom
        nativeRunSerializationBenchmark(threads, iterations, true)      // Parcel
        runOnUiThread { appendLog(...); setButtonsEnabled(true) }
    }

jni_bridge.cpp → Java_..._nativeRunSerializationBenchmark()
└── ipc::run_serialization_benchmark(kind, thread_count, iterations)
    └── benchmark.cpp
        run_parallel(thread_count, iterations, worker)
        ├── 创建 thread_count 个 std::thread
        └── run_serialization_worker(kind, iterations, ...)
            └── 循环 iterations 次：
                decode(payload) → encode(profile.transformed())
```

| 步骤 | 文件 | 函数 |
|------|------|------|
| UI | `MainActivity.kt` | `runSerializationBenchmark()` |
| JNI | `jni_bridge.cpp` | `nativeRunSerializationBenchmark()` |
| 压测 | `benchmark.cpp` | `run_serialization_benchmark()` → `run_parallel()` |
| 工作负载 | `benchmark.cpp` | `run_serialization_worker()` |
| 编解码 | `custom_codec.cpp` / `parcel_codec.cpp` | `encode` / `decode` |

---

## 13. Socket IPC 压测流程

**触发**：点击 `btnBenchSocket` → `runSocketBenchmark()`

```
MainActivity.runSocketBenchmark()
├── readBenchmarkConfig() → (threads, iterations)
└── backgroundExecutor.execute {
        nativeRunSocketBenchmark(threads, iterations, 0)   // mode=0 Add
        nativeRunSocketBenchmark(threads, iterations, 1)   // mode=1 Custom
        nativeRunSocketBenchmark(threads, iterations, 2)   // mode=2 Parcel
    }

jni_bridge.cpp → Java_..._nativeRunSocketBenchmark()
└── ipc::run_socket_benchmark(mode, thread_count, iterations)
    └── benchmark.cpp
        run_parallel(...) → run_socket_worker(mode, ...)
        └── 每线程每迭代调用：
            mode=0: global_socket_client().add(i, i+1)
            mode=1: global_socket_client().process_custom(payload)
            mode=2: global_socket_client().process_parcel(payload)
```

| mode | 枚举 | 客户端函数 |
|------|------|------------|
| 0 | `SocketBenchmarkMode::kAdd` | `SocketClient::add()` |
| 1 | `SocketBenchmarkMode::kCustom` | `SocketClient::process_custom()` |
| 2 | `SocketBenchmarkMode::kParcel` | `SocketClient::process_parcel()` |

---

## 14. Binder IPC 压测流程

**触发**：点击 `btnBenchBinder` → `runBinderBenchmark()`

```
MainActivity.runBinderBenchmark()
├── ensureBinderReady()
├── nativeBuildCustomPayload() / nativeBuildParcelPayload()
└── backgroundExecutor.execute {
        BinderBenchmark.runAdd(calculator, threads, iterations)
        BinderBenchmark.runCustom(calculator, customPayload, threads, iterations)
        BinderBenchmark.runParcel(calculator, parcelPayload, threads, iterations)
    }

BinderBenchmark.kt
├── runAdd()    → runParallel() { calculator.add(index, index+1) }
├── runCustom() → runParallel() { calculator.processCustomPayload(payload) }
└── runParcel() → runParallel() { calculator.processParcelPayload(payload) }

runParallel()
├── Executors.newFixedThreadPool(threadCount)
├── 每线程 repeat(iterations) { block(index) }
└── BenchmarkStats.from(latencies, success, failure)
```

| 步骤 | 文件 | 函数 |
|------|------|------|
| UI | `MainActivity.kt` | `runBinderBenchmark()` |
| 压测 | `BinderBenchmark.kt` | `runAdd()` / `runCustom()` / `runParcel()` |
| 线程池 | `BinderBenchmark.kt` | `runParallel()` |
| IPC | `CalculatorService.kt` | `binder.add()` 等 |

---

## 15. App 销毁流程

**触发**：用户退出 App / Activity 被销毁

```
MainActivity.onDestroy()
├── if (binderBound) unbindService(binderServiceConnection)
├── if (socketServiceBound) unbindService(socketServiceConnection)
├── backgroundExecutor.shutdownNow()
└── super.onDestroy()

（:socket_remote 进程，若无其他绑定）
SocketServerService.onDestroy()
└── nativeStopSocketServer()
    └── jni_bridge.cpp
        Java_..._nativeStopSocketServer()
        └── global_socket_server().stop()
            └── socket_server.cpp
                SocketServer::stop()
                ├── running_.store(false)
                ├── shutdown(server_fd_, SHUT_RDWR)
                └── close(server_fd_)
                → loop() 中 accept 失败，退出 while
```

| 步骤 | 文件 | 函数 |
|------|------|------|
| 解绑 | `MainActivity.kt` | `onDestroy()` → `unbindService()` |
| 停止 Socket | `SocketServerService.kt` | `onDestroy()` → `nativeStopSocketServer()` |
| Native 停止 | `socket_server.cpp` | `SocketServer::stop()` |

---

## 16. Socket 协议与线程模型

### 16.1 协议帧格式

**请求帧**（`socket_protocol.cpp`）：

```
[opcode: 1B][payload_size: 4B 大端][payload: N B]
```

**响应帧**：

```
[status: 1B][payload_size: 4B 大端][payload: N B]
```

**Opcode**（`socket_protocol.hpp`）：

| 值 | 名称 | 说明 |
|----|------|------|
| 1 | `kAdd` | 两个 int32 相加 |
| 2 | `kProcessCustom` | Custom 编解码处理 |
| 3 | `kProcessParcel` | Parcel 编解码处理 |
| 4 | `kPing` | 健康检查，返回 PONG |

### 16.2 连接模型

- **短连接**：每次 `SocketClient::transact()` 新建 socket → connect → 读写 → close
- **多线程安全**：每线程独立 fd，可并发 `transact()`

### 16.3 线程分布

| 位置 | 线程 |
|------|------|
| 主进程 UI | `MainActivity` 主线程 |
| 主进程后台 | `backgroundExecutor` 单线程 |
| 主进程压测 | C++ `std::thread` × N 或 Java 线程池 |
| `:socket_remote` 监听 | `SocketServer::loop()` detach 线程 |
| `:socket_remote` 处理 | 每连接 `handle_client()` detach 线程 |

### 16.4 CustomCodec::process / ParcelCodec::process 统一逻辑

```
process(data)
└── encode(decode(data).transformed())

UserProfile::transformed()    // types.cpp
├── id += 1
├── name += "-processed"
├── scores 每项 +1
└── rating += 0.1
```

---

## 17. JNI 函数对照表

### 17.1 MainActivity

| Kotlin `external` | JNI 函数 | C++ 调用 |
|-------------------|----------|----------|
| `nativeWaitForSocketServer` | `Java_..._MainActivity_nativeWaitForSocketServer` | `global_socket_client().ping()` |
| `nativeSocketAdd` | `Java_..._MainActivity_nativeSocketAdd` | `global_socket_client().add()` |
| `nativeBuildCustomPayload` | `Java_..._MainActivity_nativeBuildCustomPayload` | `CustomCodec::encode()` |
| `nativeBuildParcelPayload` | `Java_..._MainActivity_nativeBuildParcelPayload` | `ParcelCodec::encode()` |
| `nativeVerifyCustomPayload` | `Java_..._MainActivity_nativeVerifyCustomPayload` | `CustomCodec::decode()` |
| `nativeVerifyParcelPayload` | `Java_..._MainActivity_nativeVerifyParcelPayload` | `ParcelCodec::decode()` |
| `nativeSocketProcessCustom` | `Java_..._MainActivity_nativeSocketProcessCustom` | `global_socket_client().process_custom()` |
| `nativeSocketProcessParcel` | `Java_..._MainActivity_nativeSocketProcessParcel` | `global_socket_client().process_parcel()` |
| `nativeRunSerializationBenchmark` | `Java_..._MainActivity_nativeRunSerializationBenchmark` | `run_serialization_benchmark()` |
| `nativeRunSocketBenchmark` | `Java_..._MainActivity_nativeRunSocketBenchmark` | `run_socket_benchmark()` |

### 17.2 SocketServerService

| Kotlin `external` | JNI 函数 | C++ 调用 |
|-------------------|----------|----------|
| `nativeStartSocketServer` | `Java_..._SocketServerService_nativeStartSocketServer` | `global_socket_server().start()` |
| `nativeStopSocketServer` | `Java_..._SocketServerService_nativeStopSocketServer` | `global_socket_server().stop()` |
| `nativeGetProcessId` | `Java_..._SocketServerService_nativeGetProcessId` | `getpid()` |

### 17.3 NativeIpc

| Kotlin `external` | JNI 函数 | C++ 调用 |
|-------------------|----------|----------|
| `nativeProcessCustomPayload` | `Java_..._NativeIpc_nativeProcessCustomPayload` | `CustomCodec::process()` |
| `nativeProcessParcelPayload` | `Java_..._NativeIpc_nativeProcessParcelPayload` | `ParcelCodec::process()` |
| `nativeDescribePayload` | `Java_..._NativeIpc_nativeDescribePayload` | `CustomCodec::decode()` / `ParcelCodec::decode()` |

### 17.4 JNI 工具（jni_util.cpp）

| 函数 | 作用 |
|------|------|
| `jniutil::to_bytes(env, jbyteArray)` | Java `byte[]` → `vector<uint8_t>` |
| `jniutil::to_jbyte_array(env, span)` | `vector<uint8_t>` → Java `byte[]` |
| `jniutil::to_jstring(env, string)` | C++ `string` → Java `String` |
| `jniutil::to_string(env, jstring)` | Java `String` → C++ `string` |

---

## 附录：全流程时序总览

```
App 启动
  MainActivity.onCreate()
    ├── startRemoteServices()
    │     bindService(SocketServerService)
    │       → :socket_remote 进程
    │         SocketServerService.onCreate() → nativeStartSocketServer() → SocketServer::loop()
    │         SocketServerService.onBind() → LocalBinder
    │       → onServiceConnected() → waitForSocketServerAsync() → ping 就绪
    │
    └── bindCalculatorService()
          bindService(CalculatorService)
            → :binder_remote 进程
              CalculatorService.onBind() → ICalculator.Stub
            → onServiceConnected() → ICalculator 代理

用户操作（9 种按钮 → 见上文各流程章节）

App 退出
  MainActivity.onDestroy()
    ├── unbindService × 2
    └── SocketServerService.onDestroy() → nativeStopSocketServer()
```

---

*文档生成自 socketipc 项目源码，路径：`binder/socketipc/docs/full_flow.md`*
