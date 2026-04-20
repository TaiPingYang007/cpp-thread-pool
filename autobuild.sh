#!/usr/bin/env bash

# 遇到错误、未定义变量或管道失败时立即退出
set -euo pipefail

# 项目根目录
ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"

# 构建目录
BUILD_DIR="$ROOT_DIR/build"

echo "[1/2] 正在生成构建文件..."
cmake -S "$ROOT_DIR" -B "$BUILD_DIR"

echo "[2/2] 正在编译 test_pool..."
cmake --build "$BUILD_DIR"

echo "编译完成，可执行文件位置：$ROOT_DIR/bin/test_pool"
echo "正在运行自测..."
"$ROOT_DIR/bin/test_pool"
