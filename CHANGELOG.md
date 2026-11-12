# CHANGELOG

> 本项目（`reactor`）是一个参考 muduo 设计的 C++17 主从 Reactor TCP 服务器。
> 本文件记录主要版本变更、问题根因与验证数据，便于复盘与面试引用。
>
> 测试环境：WSL2 Ubuntu 24.04，**4 vCPU / 4GB**，loopback；客户端与服务端同机；
> 消息 64B，日志关闭（`-DBENCH_SILENT` 整块编译掉），`-O2 -ltcmalloc`；
> 吞吐单位 msg/s（每消息一个 4B 长度头 + 64B 正文）。

---

## v16 — 稳定性与生命周期修复（2026-11-12）

**Changed**
- `EventLoop::RunInLoop` 改为线程感知：在 loop 线程直接执行，跨线程走 `AddTask`（eventfd 唤醒）。
  文件：`include/EventLoop.h`、`EventLoop.cpp`。
- `EventLoop::HandleTasks`：锁内只 `swap` 任务队列，**锁外**执行任务，避免任务内部再 `AddTask` 造成自死锁。
- 新增 `EventLoop::Quit()`：置 `end_ = true` 并写 eventfd 唤醒 `epoll_wait`；`~EventLoop()` 调用 `Quit()`。
- `TcpServer::~TcpServer()`：先对所有 loop 调用 `Quit()`，再 `join` 线程（修复进程无法退出）。
- 删除调试残留 `func(){ std::cout << "111" ... }` 及每连接的 `loop->AddTask(func)`。
- 帧长非法（`MSGSIZE == 0 || MSGSIZE > 4096`）时直接 `DeleteConnection()`，不再丢弃 4 字节头后继续解析错位流。

**验证**
| 场景 | 结果 |
|---|---|
| 128 连接 t2/b16/5s ×4 | 14.26–14.49M，服务端 189–198% CPU |
| 1024 连接 t4/b64/5s | 62–63M（与 v15 交替对照，±5%） |
| 5000 空闲连接 | 8,640 B/连接，6 线程 |
| 5000 连接 5 分钟稳态（两客户端各 2500） | 30.37M msg/s，RSS 50,960KB 全程零增长，服务存活 |
| churn（512 稳态 + 100 次/秒，120s） | 11,553 次成功、0 错误，服务存活 |
| ASan（1024 连接带流量） | 无越界/UAF 报告 |
| `quit` | 进程正常退出 |

---

## v15 — eventfd 唤醒 + 任务队列（2026-11-12）

**Added**
- `EventLoop` 引入 eventfd 通道（`ev_channel_`）、任务队列（`tasks_` + `mtx_`）、
  `AddTask()` / `HandleTasks()` / `EnableEventFd()`。
- 连接建立从 accept 线程改为投递到目标 sub loop 线程执行，消除跨线程修改 `Connections_` 的竞争。

**验证**
- 128c：14.54–15.17M；1024c：64.10–66.63M（单次）；5000 空闲 8,607 B/连接；churn 120s 通过；ASan 无报告。
- 与 v14 交替对照：无显著性能差异（机器方差 ±15%）。**eventfd 是正确性修复，不是性能优化。**

**问题（已在 v16 修复）**
- `RunInLoop` 仍为 `f()`；`HandleTasks` 持锁执行任务；调试打印 `111`；析构不唤醒 eventfd 导致 `quit` 挂起。

---

## v14 — 移除线程池与内存池（2026-11-12）

**Changed / Removed**
- 移除 `ThreadPool`，恢复 `std::vector<std::thread> threads_` 启动各 loop。
- 移除 `Channel`/`Connection`/`Buffer` 的池化 `operator new/delete`，统一使用 tcmalloc。
- `ServerBuffer.h` 重命名为 `Buffer.h` / `Buffer.cpp`。

**验证**
- 128c：13.95–14.40M；1024c：52.24–52.41M；5000 空闲 8,582 B/连接；5000 连接带流量 28.53M；
  churn 120s 通过；ASan 无报告。

---

## v13 — 对照实验：去掉线程池（未合并）

仅用于量化线程池收益：128c 13.46–14.30M；1024c 52.04–54.88M。
与 v11（带线程池）无显著差异 → **线程池对稳态吞吐无贡献**，且池越大越慢（pool 30 比 pool 6 慢 3.7%）。

---

## v12 — 实验：仅 Buffer 使用内存池（未合并）

128c ×6 稳定；1024c ×5 中 4 次 segfault。
根因：Buffer 在连接建立线程分配、在 loop 线程析构，跨线程 `allocate/deallocate` 仍非线程安全。
结论：不可行，只降低崩溃概率。

---

## v11 — 移除静态内存池（2026-11-11）

**Fixed**
- 删除 `Channel`/`Connection`/`Buffer` 的池化 `operator new/delete`，改为 tcmalloc。
- 配合 v10 的 eventfd 任务队列，`128 连接 × 4 次` 全部稳定通过；ASan 无报告。

**性能（4 vCPU）**
- 128c：13.46–14.61M；1024c t4/b64 **56.78M**（服务端 262% CPU）；
- 池大小扫描：pool 6 → 30，吞吐 14.36M → 13.83M（**越小越好**）。

---

## v10 — eventfd + QueueInLoop 实验（未合并，仍崩）

- 为 `EventLoop` 增加 eventfd、`pendingFunctors_`、`loopThread_`，`RunInLoop` 跨线程转 `QueueInLoop`。
- 单独修复该项后仍随机崩溃（pool=14 4/4），证明静态内存池是独立的第二根因。

---

## v9 — 线程池实验（已撤回，2026-11-11）

**Added**
- `ThreadPool`（`queue + mutex + condvar`），`TcpServer` 用 `tp_(loops_num + 10)`，
  loop 启动与连接建立均投递到线程池。

**问题**
- 128 连接下 pool=14/16/24/32 **4/4 segfault（rc=139）**；pool=6 稳定。
- ASan：`SEGV in EventLoop::AddChannel` + `epoll_ctl: File exists`。
- TSan（`-fsanitize=thread` + `setarch -R`）抓出 **84 条 data race**，代表：
  `MemoryPool::allocate() include/MemoryPool.h:26 ← Connection::operator new ← TcpServer.cpp:32 ← EventLoop::RunInLoop`，
  以及 `EventLoop::AddConnection → unordered_map::emplace ← TcpServer.cpp:45` 与 loop 线程 `erase` 竞争。
- 根因：①`RunInLoop(f){f();}` 同步执行，连接建立在池线程；②静态 `MemoryPool` 无锁共享。
- 教训：线程池用于「常驻阻塞任务」（loop）是错误用法，占用 worker 且放大竞争。

---

## v8 — 内存池块大小修正（2026-11-10）

- `MemoryPool` 块大小 64B → 128B（`sizeof(Channel)=88` 曾越界 24B，ASan 报
  `WRITE of size 24 ... 0 bytes after 262144-byte region`）。
- 4096 / 5000 空闲连接存活（约 8.7KB/连接）；QPS 扫描 128/512/1024/2048 连接 → 41.7 / 46.3 / **52.07** / 37.5M。
- churn（512 稳态 + 100 次/秒）约 200s 段错误 → 指向跨线程问题（见 v9）。

---

## v7 — 内存池 vs tcmalloc 对比（2026-11-10）

- 相同代码仅差池化 `operator new`：128c +1.5%、1024c +3.3% → **池收益在噪声内**；
  而 tcmalloc 相对 glibc 为 **+17–26%**。
- 结论：保留 tcmalloc，删除自研池。

---

## v6 — 自研内存池 + tcmalloc（2026-11-10）

- `MemoryPool`：free-list，64 blocks × 4096B 页，`operator new` 忽略 size。
- v6tc：1L 17.75M、4L t4/b64 **53.5M**；空闲连接内存 17,152 B/连接（4KB 块装 48B 对象，浪费 8KB/连接）。
- 遗留：静态池跨线程无锁、页只增不减。

---

## v5 — `size_t` 误接 `ssize_t` 修复后复测（2026-11-09）

- `Connection.cpp` 的 `size_t n = read_buff_->readn(...)` 改为 `ssize_t n`（EAGAIN 的 -1 曾被当作 SIZE_MAX → 忙等死循环）。
- 日志关闭下：1L 14.05M、4L 14.79M、4L t4/b64 43.42M；队头阻塞探针 20/20；ASan 无报告。

---

## v4 — EAGAIN 忙等定位（2026-11-09）

- 现象：4L 仅 448,723 msg/s（209% CPU），队头阻塞 0/20。
- 根因：`size_t` 接收 `ssize_t` 返回值，EAGAIN 退化为 SIZE_MAX，`if (n > 0) continue;` 进入忙等。
- 修复一行后：4L 15.30M，队头阻塞 20/20。

---

## v3 — StreamBuffer 重构（2026-11-08）

**Added**
- 流式接收缓冲区（`reader_`/`writer_` 双下标、`makeSpace` 紧凑、按需扩容 ≤64KB）。
- 先切完整帧再从 socket 读；半包保留；`MSGSIZE` 上限校验（4096）。
- 非阻塞监听/连接 fd + `accept4` + `SO_REUSEADDR`；accept 循环 `EAGAIN` 退出。
- 延迟删除连接（`connectionsBin`，事件分发后统一 erase），修复回调内析构自身 UAF。
- 日志 `#ifndef BENCH_SILENT` 整块编译掉（此前用空流替换仍会求值 `GetTime::GetCurrentTime()`）。

**性能**
- 128c：1L 17.8M、4L 15.1M；1024c t4/b64 **49.5M**；1L 单核 26.2M（100% CPU）。
- 32KB 大帧正常；队头阻塞 20/20；ASan 全绿。

---

## v2 — Buffer 抽象（2026-11-08）

**Added**
- 新增 `Buffer` 抽象、长度前缀解析、构造参数化（IP/port/loops）、`localtime_r` + `snprintf` 时间戳、`-O2 -std=c++17`。

**问题（v3 修复）**
- `Connection.cpp:44` 用 1024B 栈数组接收 `read_buff_->left()` 字节 → **栈溢出**（>1024B 帧即崩，远程 DoS）。
- 无长度校验；半包头被丢弃导致帧失步；UAF；阻塞 recv 导致队头阻塞。

---

## v1 — 初始版本（2026-11-06）

- 参考 muduo 的 one-loop-per-thread 主从 Reactor；每消息 `std::cout` 日志 + `std::endl`；
  连接回调内析构自身；阻塞 socket；无长度校验；`-O0` 构建、`loops_num` 硬编码 15。
- 基线测试：未修复版在 400k msg/s 以下可用，600k 起随机停摆，800k+ 必崩
  （`malloc(): invalid size` / `heap-buffer-overflow`）。

---

## 性能总览（4 vCPU / 64B / loopback / 日志关）

| 版本 | 128 连接 | 1024 连接 t4/b64 | 单核（1 loop） | 备注 |
|---|---:|---:|---:|---|
| v1 原始 | — | 崩溃 | — | `-O0`，每消息日志 |
| v2 用户版 | 4L 2.17M* | — | 1.15M* | 1024B 栈缓冲 |
| v3 StreamBuffer | 15.1M | 49.5M | 26.2M | 修复 UAF/HOL/溢出 |
| v6 + 内存池 | 15.3M | 53.5M | 17.8M | 池收益有限 |
| v11 去池 + eventfd 任务队列 | 14.4M | 56.8M | 27.0M | 稳定 |
| v14 最终版（去线程池/池） | 14.2M | 52.3M | 16.7M | 稳定 |
| **v16 当前** | **14.3M** | **62–63M** | — | 稳定 + 可优雅退出 |

\* v2 为日志关闭、与同期基线对比的相对值。

---

## 已知限制（不影响当前功能）

- 服务端只收不回：`SendMsg`/`Buffer::writen` 未处理短写与 `EAGAIN`，做 echo/RPC 前需补输出缓冲 + `EPOLLOUT`。
- `Channel::HandleEvent` 使用注册事件掩码而非 `revents`，`EPOLLERR/HUP` 依赖 `recv() == 0/-1` 兜底。
- 无空闲连接超时/心跳；`Acceptor` bind 失败仅 `perror`（无 `SO_REUSEADDR`）。
- （已清理）`MemoryPool.h` / `ThreadPool.h` 已从仓库删除，`Makefile` 的 `-Iinclude` 已修正。
- `Makefile` 的 `HEAD` 仍只列了 `include/GetTime.h`（改其它头文件不会触发重编）。
- 性能数字均在「服务端与压测客户端共享 4 vCPU」的单机环境测得，多机环境下服务端极限会更高。