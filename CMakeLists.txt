# 指定 CMake 的最低版本
cmake_minimum_required(VERSION 3.16)

# 声明项目名称和使用的语言
project(thread_pool LANGUAGES CXX)

# 指定项目使用 C++17
set(CMAKE_CXX_STANDARD 11)

# 要求编译器必须支持 C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 关闭编译器扩展，统一使用标准 C++
set(CMAKE_CXX_EXTENSIONS OFF)

# 将可执行文件统一输出到构建目录，避免污染源码目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 开启 CTest，方便用一条命令跑自测
include(CTest)

# 查找标准线程库
find_package(Threads REQUIRED)

# 编译线程池源码和测试文件 main.cpp，生成 test_pool
add_executable(test_pool
  test/main.cpp
  src/ThreadPool.cpp
)

# 为目标添加头文件搜索路径
target_include_directories(test_pool PRIVATE ${CMAKE_SOURCE_DIR}/include)

# 为目标链接线程库
target_link_libraries(test_pool PRIVATE Threads::Threads)

# 将当前自测程序注册到 CTest
add_test(NAME thread_pool_self_test COMMAND $<TARGET_FILE:test_pool>)
