# Thread Pool

一个基于 C++17 实现的固定大小线程池项目，重点演示任务提交、异步结果回传、有界队列和优雅关闭这几类后端并发基础能力。

## Highlights

- 使用固定数量的 worker 线程复用执行任务，避免频繁创建和销毁线程。
- 使用 `std::packaged_task` 和 `std::future` 返回异步任务结果。
- 使用有界任务队列做过载保护，避免任务无限堆积。
- 提供显式 `shutdown()`，析构函数兜底回收线程资源。
- 自带一组自测，覆盖配置边界、队列上限和关闭语义。

## Requirements

- C++17
- CMake 3.16+
- 支持标准线程库的编译器

## Quick Start

### Local Build

```bash
cmake -S . -B build
cmake --build build
./build/bin/test_pool
```

### Run With CTest

```bash
ctest --test-dir build --output-on-failure
```

### One-Click Build

```bash
./autobuild.sh
```

这个脚本会自动：

- 生成 `build/` 构建目录
- 编译 `test_pool`
- 运行全部自测

## Docker

```bash
docker build -t thread_pool_env:v1 .
docker run --rm thread_pool_env:v1
```

当前 `Dockerfile` 会在镜像构建阶段完成以下事情：

- 基于 `ubuntu:22.04` 准备构建环境
- 安装 `build-essential` 和 `cmake`
- 复制项目源码
- 编译并执行 `ctest`
- 在最终运行镜像里只保留可执行文件 `test_pool`

这意味着别人拿到你的项目后，只要本机有 Docker，通常就可以用同样的命令得到一致的构建和运行环境。

## Self-Test Coverage

当前自测主要覆盖：

- 非法线程数配置
- 非法队列容量配置
- 队列满时拒绝新任务
- `shutdown()` 等待历史任务完成
- `shutdown()` 重复调用
- worker 线程内部调用 `shutdown()`

## Project Layout

```text
.
├── CMakeLists.txt
├── Dockerfile
├── README.md
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

- `include/ThreadPool.h`: 线程池类声明和模板成员函数 `enqueue`
- `src/ThreadPool.cpp`: 构造函数、worker 循环、`shutdown()` 和析构逻辑
- `test/main.cpp`: 自测入口
- `autobuild.sh`: 一键构建并运行测试
- `Dockerfile`: 容器化构建和运行入口

## Notes

- 本地构建产物默认放在 `build/` 目录，不提交到仓库。
- `.dockerignore` 会忽略 `.git`、`build/`、`bin/` 等不需要进入 Docker 构建上下文的内容。
- 如果仓库路径发生变化，`autobuild.sh` 会自动清理失效的旧 CMake 缓存。
