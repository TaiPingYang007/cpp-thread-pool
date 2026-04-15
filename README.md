# Thread Pool

一个基于 C++17 实现的固定大小线程池组件，面向后端服务场景设计，可作为后续集群聊天服务器的底层并发模块。

当前版本重点解决三件事：

- 复用工作线程，降低频繁创建与销毁线程的系统开销
- 解耦任务提交与任务执行，统一异步调度入口
- 用自测覆盖关键边界，证明线程池在配置校验、关闭语义和队列控制上的正确性

## Overview

| 项目项 | 说明 |
| --- | --- |
| 语言标准 | `C++17` |
| 构建工具 | `CMake 3.16+` |
| 核心机制 | `std::thread`、`std::mutex`、`std::condition_variable` |
| 异步返回 | `std::packaged_task`、`std::future` |
| 可执行文件 | `bin/test_pool` |
| 默认入口 | 自测程序 |
| 适用场景 | 后端组件、并发基础设施、面试项目 |

## Highlights

- 使用 `ThreadPool::Config` 管理线程数和队列容量，接口清晰，便于后续扩展。
- 采用固定大小 worker 模型，降低线程创建与销毁成本。
- 使用有界任务队列实现过载保护，避免任务无限堆积。
- `enqueue()` 支持任意可调用对象，统一封装成异步任务。
- 通过 `future` 返回任务结果，调用方可同步等待或延后获取。
- 使用条件变量阻塞唤醒 worker，避免空轮询浪费 CPU。
- 构造函数具备异常安全清理能力，避免部分线程创建成功后程序直接失控。
- 提供显式 `shutdown()`，析构函数兜底回收资源，保持 RAII 风格。

## Why This Project Matters

这个项目的价值不在代码量，而在它覆盖了 C++ 后端面试中非常高频的基础能力：

- 生产者-消费者模型
- 条件变量与阻塞唤醒
- 模板与完美转发
- 类型擦除
- 异步结果回传
- 线程生命周期管理
- 关闭语义与异常安全

如果要在面试中展开，我最建议重点讲这 5 个点：

1. 为什么 `condition_variable::wait` 必须配合 `unique_lock`
2. 为什么 `task()` 必须放在锁外执行
3. 为什么队列元素统一存成 `std::function<void()>`
4. 为什么要用 `packaged_task + future`
5. 为什么构造函数和关闭流程都要考虑异常安全

## Validation

当前仓库默认运行的是一组自测，而不是打印型 demo。自测入口在 [test/main.cpp](/home/taipingyang/learn/cpp_project/01_thread_pool/test/main.cpp)。

| 自测用例 | 验证目标 |
| --- | --- |
| 非法线程数会抛异常 | `thread_count == 0` 的配置边界 |
| 非法队列容量会抛异常 | `max_queue_size == 0` 的配置边界 |
| 队列满时拒绝新任务 | 有界队列是否真正生效 |
| `shutdown` 会等旧任务完成并拒绝新任务 | 优雅关闭语义 |
| `shutdown` 可重复调用 | 基础幂等性 |
| 任务内部调用 `shutdown` 会让线程池进入关闭状态 | 特殊关闭路径验证 |

这套自测目前更偏向 correctness 验证，而不是性能压测。  
如果后续要展示吞吐、延迟和不同线程数下的性能曲线，更建议单独增加 `benchmark` 目标，而不是和自测混在一起。

## Quick Start

### Build

```bash
cmake -S . -B build
cmake --build build
```

### Run Self-Test

```bash
./bin/test_pool
```

### Run With CTest

```bash
ctest --test-dir build --output-on-failure
```

### One-Click Build

```bash
bash ./autobuild.sh
```

## Project Layout

```text
.
├── CMakeLists.txt
├── README.md
├── .gitignore
├── autobuild.sh
├── docs
│   └── knowledge_summary.md
├── include
│   └── ThreadPool.h
├── src
│   └── ThreadPool.cpp
└── test
    └── main.cpp
```

| 路径 | 说明 |
| --- | --- |
| `include/ThreadPool.h` | 线程池类声明、配置结构体、模板成员函数 `enqueue` |
| `src/ThreadPool.cpp` | 构造函数、worker 循环、`shutdown()`、析构函数 |
| `test/main.cpp` | 自测入口，验证边界条件、关闭语义与队列控制 |
| `docs/knowledge_summary.md` | 项目知识总结、复盘材料、面试速记稿 |
| `autobuild.sh` | 一键配置、编译并执行自测 |

## Current Status

当前版本已经具备：

- 配置合法性校验
- 有界任务队列
- 构造函数异常安全
- 主动关闭与析构兜底
- 一套可重复执行的自测入口

## Roadmap

后续可继续增强：

- 独立 `benchmark` 目标
- 动态扩缩容
- 任务优先级
- 更细粒度的关闭状态管理
- CI / clang-format / sanitizer

## Interview Summary

这是一个基于 C++17 实现的固定线程数线程池。我使用 `mutex + condition_variable` 管理任务队列，用 `packaged_task + future` 返回异步结果，通过有界队列做过载保护，并在构造阶段和关闭阶段分别处理线程创建失败与资源回收问题。当前仓库默认提供 6 个自测用例，用来验证边界条件、关闭语义和队列控制行为。
# cpp-thread-pool
