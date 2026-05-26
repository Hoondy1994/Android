# PantanalNamespace 项目详解

> 文档版本：基于仓库当前代码整理（含 `preopen_demo` JNI 演示）  
> 包名：`com.oplus.pantanal.namespace`  
> 归属：OPPO（OPLUS）Pantanal 泛在能力生态

---

## 目录

1. [项目是什么](#1-项目是什么)
2. [解决什么问题](#2-解决什么问题)
3. [适用边界：谁会用、谁不会用](#3-适用边界谁会用谁不会用)
4. [核心概念 glossary](#4-核心概念-glossary)
5. [工程模块结构](#5-工程模块结构)
6. [Native 分层架构](#6-native-分层架构)
7. [libpreopen 原理](#7-libpreopen-原理)
8. [Seqmap 多服务隔离](#8-seqmap-多服务隔离)
9. [libnamespace 层](#9-libnamespace-层)
10. [NAPI 与 ULE 文件插件](#10-napi-与-ule-文件插件)
11. [两条调用链路](#11-两条调用链路)
12. [URI 解析模块](#12-uri-解析模块)
13. [构建、产物与发布](#13-构建产物与发布)
14. [测试与验证](#14-测试与验证)
15. [完成度与已知缺口](#15-完成度与已知缺口)
16. [Android 替代方案对比](#16-android-替代方案对比)
17. [面试速记](#17-面试速记)
18. [相关图示](#18-相关图示)

---

## 1. 项目是什么

**PantanalNamespace** 是面向 **OPPO Pantanal 泛在能力平台** 的 **Native SDK**，核心职责是：

> 为运行在 **ULE JS 运行时** 中的 **不可信 JS 插件**，提供 **按服务实例隔离的文件路径访问控制（沙箱）**。

它不是：

- Android 系统全局服务（不像 `ActivityManager` 管全系统）；
- 给所有 OPPO 手机 App 用的公共库；
- 普通文件管理或存储框架。

它是：

- 可选集成的 **AAR + Prefab** 中间件；
- 供 **接了 Pantanal / ULE 的宿主 App** 在进程内限制插件文件访问范围。

---

## 2. 解决什么问题

### 2.1 背景：插件沙箱

Pantanal 上跑的是 **泛在服务 / 卡片 / 插件** 的 **JavaScript 业务代码**（ULE 运行时）。插件若直接调用 POSIX `open("/data/...")`，拥有过大的 **ambient authority**（全局文件系统命名空间访问权），存在越权读写的安全风险。

### 2.2 设计目标

| 目标 | 实现思路 |
|------|----------|
| 最小权限 | 仅允许访问宿主 **预先登记** 的目录 |
| 虚拟路径 | 插件使用 `/virtualX/...`，不暴露真实绝对路径 |
| 多实例隔离 | 每个泛在服务 `uint64_t id` 独立一张映射表 |
| 低改造成本 | 用 `open_` / `access_` 包装 `open` / `access`，而非重写全部 FS API |
| 跨平台验证 | `nodejs-cmake` 在 PC 上用 Node NAPI 先验证绑定 |

### 2.3 理论基础

底层库 **libpreopen** 来自 FreeBSD **Capsicum** 相关研究：在沙箱进程中关闭全局命名空间访问，改为 **pre-open 目录 fd + openat 相对路径**。

---

## 3. 适用边界：谁会用、谁不会用

| 场景 | 是否使用本 SDK |
|------|----------------|
| Pantanal 宿主 + ULE JS 插件 | ✅ |
| 集成 `mylibrary3` / `sdk` AAR 的内部模块 | ✅ |
| 普通第三方 App（未接 Pantanal） | ❌ |
| 全 Android 系统自动生效 | ❌ |
| 仓库内 `app` 演示模块 | 仅开发测试（可 JNI 直连验证 libpreopen） |

**「运行时」在此指：** 宿主进程内 **执行 JS 插件** 的 **ULE 环境**，不是「所有 App 正在运行」。

---

## 4. 核心概念 glossary

| 术语 | 含义 |
|------|------|
| **Pantanal** | OPPO 泛在智慧能力平台（卡片、跨端服务等） |
| **ULE** | 平台内用于执行插件 JS 的运行时（含 `libule_napi.so`） |
| **JS 运行时** | 执行 JavaScript 的环境 = 引擎 + 标准库 + 宿主注入的 Native API，**不是**「App 正在跑」 |
| **NAPI** | Node-API 风格 C 接口，用于 JS ↔ Native 互操作 |
| **libpreopen** | 路径映射与 `open_`/`access_` 等 libc 包装 |
| **po_map** | 单张「真实目录 fd ↔ 虚拟前缀」映射表 |
| **Seqmap** | 按 `service id` 管理多张 `po_map` 的单例 |
| **NamespaceLibc** | 对外的 `ns_open` / `ns_access` 等（设计给 NAPI 层调用） |
| **ule_file** | NAPI 模块名，导出 `init`、`move` 等给 JS |
| **Prefab** | Android 原生依赖发布格式，暴露 C++ 头文件给依赖方 |

---

## 5. 工程模块结构

```
PantanalNamespace/                 # Gradle 根工程
├── app/                           # 演示 App（Kotlin UI + 可选 Demo）
├── sdk/                           # 发布用 SDK 模块（与 mylibrary3 结构平行）
├── mylibrary3/                    # 开发/调试库模块（ule_file1）
├── nodejs-cmake/                  # PC 端 Node NAPI 验证（非 Android 产物）
├── docs/                          # 本文档与 SVG 图
│   ├── PROJECT_GUIDE.md
│   ├── architecture.svg
│   └── flowchart.svg
├── build.gradle
├── settings.gradle                # include :app :sdk :mylibrary3
└── gradle.properties              # OPPO Maven、版本号等
```

### 5.1 模块职责

| 模块 | 类型 | 职责 |
|------|------|------|
| **app** | Application | UI 壳；依赖 `mylibrary3`；`FirstFragment` 可触发 `PreopenBridge` Demo |
| **mylibrary3** | Android Library | Native 源码主阵地；产出 `libnamespace.so`、`libule_file1.so`、`libpreopen_demo.so` 等 |
| **sdk** | Android Library | 与 mylibrary3 同构，用于 Maven 发布（`publish.gradle` 指向 sdk release） |
| **nodejs-cmake** | 桌面验证 | `cmake-js` + `binding.c` + `index.js` 调用 move/copy 等 |

### 5.2 包名与品牌

- `com.oplus.pantanal.namespace` — App
- `com.oplus.pantanal.namespace.mylibrary3` — 库
- `oplus` = OPPO 系品牌；`pantanal` = 泛在平台

---

## 6. Native 分层架构

详见 [`architecture.svg`](architecture.svg)。

自下而上：

```
┌─────────────────────────────────────────────────────────┐
│  宿主 / 插件层：ULE JS、Kotlin Demo、Node 桌面测试        │
├─────────────────────────────────────────────────────────┤
│  桥接层：NAPI (ule_file)、JNI (PreopenBridge)、Node binding│
├─────────────────────────────────────────────────────────┤
│  libnamespace：ns_* API、URI、ns_client_add/remove       │
├─────────────────────────────────────────────────────────┤
│  Seqmap (libseqmap)：按 id 管理 po_map                    │
├─────────────────────────────────────────────────────────┤
│  libpreopen：po_find、open_、access_、po_map 生命周期     │
├─────────────────────────────────────────────────────────┤
│  Linux/Android：openat、faccessat、文件系统               │
└─────────────────────────────────────────────────────────┘
```

### 6.1 CMake 产物（mylibrary3）

| 目标名 | 输出 so | 说明 |
|--------|---------|------|
| `namespace` | `libnamespace.so` | NamespaceLibc + URI + Namespace |
| `preopen` | `libpreopen.a` | 静态库，链入 namespace / preopen_demo |
| `ule_file1` | `libule_file1.so` | FilePlugin NAPI 模块 |
| `preopen_demo` | `libpreopen_demo.so` | JNI Demo，仅链 preopen |
| `ulenapi` | 导入 `libule_napi.so` | ULE 提供的 NAPI 实现 |

**链接关系：**

- `namespace` → `preopen`, `ulenapi`, `utils`
- `ule_file1` → `namespace`, `ulenapi`, `utils`
- `preopen_demo` → `preopen`, `utils`, `log`

---

## 7. libpreopen 原理

### 7.1 核心数据结构

```c
struct po_relpath {
    int dirfd;              // 已 pre-open 的目录 fd，或 -1 表示无匹配
    const char *relative_path;  // 相对该目录的路径
};
```

- **po_map**：多条映射项，每项含真实路径名、虚拟前缀 `pathmap`、目录 `fd`。
- **po_find(map, path)**：对请求路径做**最长前缀匹配**，得到 `dirfd` + 相对路径。

### 7.2 open_ 包装逻辑（简化）

```
open_(serviceId, "/virtualA/file.txt", flags)
    → find_relative(path, serviceId)
    → Seqmap::GetRel → po_find
    → 若匹配：openat(dirfd, "file.txt", flags)
    → 若无匹配（dirfd=-1）：openat 失败，权限控制生效
```

### 7.3 源码位置

| 文件 | 内容 |
|------|------|
| `libpreopen/include/libpreopen.h` | 公共头文件 |
| `libpreopen/lib/libpreopen.cpp` | `po_isprefix` 等 |
| `libpreopen/lib/po_libc_wrappers.cpp` | `open_`, `access_`, `stat_` 等 |
| `libpreopen/lib/po_err.cpp` | 错误处理 |
| `libpreopen/README.md` | 上游项目说明（Capsicum 背景） |

---

## 8. Seqmap 多服务隔离

### 8.1 类定义

`libpreopen/include/libseqmap.h` — `Seqmap` 继承 `pantanal::ns::Singleton<Seqmap>`。

### 8.2 关键 API

| 方法 | 作用 |
|------|------|
| `InsertMap(id, realDir, virtualPrefix)` | 为服务 id 创建/扩展 po_map；`openat` 打开真实目录 |
| `DeleteMap(id)` | 删除并 `po_map_release` |
| `GetRel(path, id)` | 查映射，供 `find_relative` 使用 |

### 8.3 示例

```cpp
uint64_t map_id = 1000;
Seqmap::GetInstance().InsertMap(
    map_id,
    "/data/user/0/.../files/preopen_demo/real_root",  // 真实目录
    "/vdemo"                                           // 虚拟前缀
);
int fd = open_(map_id, "/vdemo/hello.txt", O_RDONLY);  // 允许
int bad = open_(map_id, "/outside/x", O_RDONLY);       // 拒绝
```

### 8.4 实现文件

`libpreopen/lib/libseqmap.cpp`

---

## 9. libnamespace 层

### 9.1 Namespace.h — 服务生命周期（设计）

```c
int ns_client_add(uint64_t id, void *data);    // 注册泛在服务命名空间环境
int ns_client_remove(uint64_t id);             // 移除
```

当前 `Namespace.cpp` 中为 **桩实现**（`return 0`），完整逻辑应由平台宿主注入 `data`（含路径映射配置）。

### 9.2 NamespaceLibc.h — 文件 API（设计）

面向 **NAPI 环境** 的包装：

- `ns_access`, `ns_open`, `ns_openat`, `ns_stat`, `ns_unlink`, `ns_rename` …
- 参数含 `napi_env *env`，便于回调 JS。

当前 `NamespaceLibc.cpp` 中 **全部为桩**（直接 `return 0`），尚未转发到 `open_` / `Seqmap`。

### 9.3 uri.cpp — URI 解析

`URI` 类解析：

- `scheme:`（如 `file:`、`http:` 或业务自定义 scheme）
- `//authority`、userinfo、host、port
- path、query、fragment

用于将 **逻辑资源地址** 与 **物理/虚拟路径** 解耦；与 Seqmap 的自动串联在仓库中 **尚未完成**。

---

## 10. NAPI 与 ULE 文件插件

### 10.1 FilePlugin.cpp

- 模块名：`ule_file`（`napi_module_register` + constructor `FileRegister`）
- 导出：`init`, `move`
- `MoveFile`：从 JS 取 src/dest 字符串与 success 回调；内部演示性调用 `InsertMap` + `open_`

### 10.2 ule-napi

- 预编译 `libule_napi.so`（按 ABI 放在 `ule-napi/src/${ANDROID_ABI}/`）
- `mylibrary3/build.gradle` 的 `packagingOptions` **排除** 打包 `libule_napi.so`（由宿主或运行时提供）

### 10.3 nodejs-cmake（桌面）

- `binding.c`：完整文件 API（copy/move/list/read/write/access/mkdir/rmdir…）
- `index.js`：Node 侧调用示例
- 用途：在无 Android 设备时验证 NAPI 绑定逻辑

---

## 11. 两条调用链路

详见 [`flowchart.svg`](flowchart.svg)。

### 11.1 正式设计路径（ULE 插件）

```
JS 插件代码
  → ule_file.move(src, dest, onSuccess, onFail, ...)
  → FilePlugin.cpp (NAPI)
  → [设计] ns_open / NamespaceLibc
  → open_(serviceId, virtualPath, flags)
  → Seqmap::GetRel → po_find → openat
```

### 11.2 当前 Demo 路径（App 内 JNI）

```
FirstFragment 按钮
  → PreopenBridge.runDemo(filesDir)
  → preopen_jni.cpp (JNI)
  → InsertMap + open_ / access_
  → 结果显示在 TextView；Logcat 标签 PreopenDemo
```

**区别：** Demo **不经过** ULE / JS 运行时，仅验证 **libpreopen 权限 enforcement** 核心逻辑。

---

## 12. URI 解析模块

| 类/方法 | 说明 |
|---------|------|
| `URI::URI(string)` | 构造并 `ParseUri` |
| `GetScheme/GetHost/GetPath/...` | 取值 |
| `IsValid()` | 解析是否合法 |

路径：`mylibrary3/src/main/cpp/libnamespace/uri.cpp`  
头文件：`libnamespace/include/namespace/uri.h`

---

## 13. 构建、产物与发布

### 13.1 构建命令

```bash
# 演示 App
./gradlew :app:assembleOppoDebug

# 库模块
./gradlew :mylibrary3:assembleDebug
```

### 13.2 关键配置

- **NDK + CMake 3.18**，C++17
- **ABI**：`armeabi-v7a`, `arm64-v8a`
- **Prefab**：发布 `namespace` 模块头文件（`libnamespace/include`）
- **OPPO 流水线**：`opipeline.gradle`、`obuildplugin`（可选）
- **Maven**：`publish.gradle` 发布 `sdk` release AAR 到内部 Nexus

### 13.3 版本属性（gradle.properties 摘录）

- `prop_sdkVersionName=1.3`
- `prop_archivesGroupName=com.oplus.pantanal.namespace`

---

## 14. 测试与验证

| 方式 | 位置 | 内容 |
|------|------|------|
| GoogleTest | `mylibrary3/src/main/cpp/tests/libpreopen_tests.cpp` | InsertMap、open_、access_、delete 等 |
| URI 测试 | `tests/liburi_test.cpp` | URI 解析 |
| Node 桌面 | `nodejs-cmake/index.js` | binding API 冒烟 |
| App JNI Demo | `PreopenBridge` + `preopen_jni.cpp` | 允许/拒绝路径对比 |

启用单元测试：在 `local.properties` 设置 `prop_enable_unittests=true`（见根 `build.gradle` `getUnitTestsEnable()`）。

---

## 15. 完成度与已知缺口

| 组件 | 状态 |
|------|------|
| libpreopen + Seqmap + open_/access_ | ✅ 可用 |
| libpreopen 单元测试 | ✅ |
| FilePlugin NAPI 注册 + move 演示 | ⚠️ 部分（硬编码路径） |
| NamespaceLibc ns_* | ❌ 桩 |
| ns_client_add/remove | ❌ 桩 |
| URI ↔ Seqmap 自动路由 | ❌ 未串联 |
| 宿主注入映射表 | 📋 平台侧，不在本仓库 |
| App ↔ ULE 完整集成 | ❌ App 仅为 JNI Demo |

---

## 16. Android 替代方案对比

| 方案 | 粒度 | 与本项目关系 |
|------|------|----------------|
| 应用沙箱 UID | 包级 | 插件同进程时不够 |
| Scoped Storage / SAF | App + 用户授权 | 不管虚拟路径 |
| FileProvider | Uri 级分享 | 不适合高频 POSIX 风格 API |
| 独立进程 + IPC | 进程级 | 更重，延迟更高 |
| 小程序式 JS FS API | API 级 | 不暴露路径；我们是 Native 包装 |
| WASI preopen | 目录 preopen | 思想几乎相同 |
| seccomp / Landlock | 内核级 | 可互补，非替代 |

---

## 17. 面试速记

**一句话：** OPPO Pantanal 里给 ULE JS 插件做文件沙箱的 Native SDK；虚拟路径 + 按服务 id 的 preopen 映射 + openat。

**不是：** 全 Android 公共服务、所有 OPPO App 必装组件。

**核心库：** libpreopen（能力） + Seqmap（多租户） + libnamespace（对外 API，部分未实现） + ule_file（NAPI）。

**我做过：** Seqmap/libpreopen 联调、NAPI FilePlugin、URI、CMake/Prefab、JNI Demo。

---

## 18. 相关图示

| 文件 | 说明 |
|------|------|
| [architecture.svg](architecture.svg) | **详细版**分层架构图：Gradle 模块、CMake 依赖、五层架构、数据结构、映射示例、方案对比 |
| [flowchart.svg](flowchart.svg) | **详细版**流程图：服务生命周期、路径 A/B 逐步、InsertMap、open_/po_find、libc 包装表、测试矩阵、源码索引 |

---

## 附录 A：关键文件索引

```
mylibrary3/src/main/cpp/
├── CMakeLists.txt
├── FilePlugin.cpp              # NAPI ule_file
├── preopen_jni.cpp             # JNI Demo
├── libpreopen/                 # 沙箱核心
├── libnamespace/               # ns_* + uri
├── ule-napi/                   # libule_napi.so 导入
├── utils/                      # Log, Singleton
└── tests/                      # gtest

mylibrary3/src/main/java/.../demo/
└── PreopenBridge.kt

app/src/main/java/.../FirstFragment.kt
```

---

## 附录 B：修订记录

| 日期 | 说明 |
|------|------|
| 2026-05-20 | 初版：含项目详解、architecture.svg、flowchart.svg |
