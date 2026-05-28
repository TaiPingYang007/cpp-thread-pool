#include "ThreadPool.h"

#include <chrono>
#include <future>
#include <iostream>
#include <string>
#include <vector>

// 基本用法：创建线程池、提交任务、拿结果、shutdown
void demo_basic()
{
    std::cout << "=== demo_basic ===" << std::endl;

    ThreadPool::Config config;
    config.thread_count = 4;
    config.max_queue_size = 100;

    ThreadPool pool(config);

    auto f1 = pool.enqueue([] {
        return 42;
    });

    auto f2 = pool.enqueue([](int a, int b) {
        return a + b;
    }, 10, 20);

    std::cout << "f1.get() = " << f1.get() << std::endl;
    std::cout << "f2.get() = " << f2.get() << std::endl;

    pool.shutdown();
    std::cout << std::endl;
}

// 多任务并发：提交一批任务，用 future 收集结果
void demo_multiple_tasks()
{
    std::cout << "=== demo_multiple_tasks ===" << std::endl;

    ThreadPool::Config config;
    config.thread_count = 4;

    ThreadPool pool(config);

    std::vector<std::future<int>> futures;
    futures.reserve(10);

    for (int i = 0; i < 10; ++i)
    {
        futures.emplace_back(pool.enqueue([i] {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            return i * i;
        }));
    }

    std::cout << "Results: ";
    for (int i = 0; i < 10; ++i)
    {
        std::cout << futures[i].get();
        if (i < 9)
        {
            std::cout << ", ";
        }
    }
    std::cout << std::endl;

    pool.shutdown();
    std::cout << std::endl;
}

// 异常处理：任务内部抛异常，future.get() 捕获
void demo_exception()
{
    std::cout << "=== demo_exception ===" << std::endl;

    ThreadPool::Config config;
    config.thread_count = 2;

    ThreadPool pool(config);

    auto f1 = pool.enqueue([] {
        return 100;
    });

    auto f2 = pool.enqueue([]() -> int {
        throw std::runtime_error("something went wrong");
    });

    std::cout << "f1.get() = " << f1.get() << std::endl;

    try
    {
        f2.get();
    }
    catch (const std::runtime_error &e)
    {
        std::cout << "f2.get() caught: " << e.what() << std::endl;
    }

    pool.shutdown();
    std::cout << std::endl;
}

// 队列满：提交到队列满时 enqueue 抛异常
void demo_queue_full()
{
    std::cout << "=== demo_queue_full ===" << std::endl;

    ThreadPool::Config config;
    config.thread_count = 1;
    config.max_queue_size = 1;

    ThreadPool pool(config);

    std::promise<void> gate;
    std::shared_future<void> ready = gate.get_future().share();

    std::atomic<bool> first_started{false};

    auto f1 = pool.enqueue([&] {
        first_started = true;
        ready.wait();
        return 1;
    });

    while (!first_started.load())
    {
        std::this_thread::yield();
    }

    auto f2 = pool.enqueue([] { return 2; });

    bool caught = false;
    try
    {
        auto f3 = pool.enqueue([] { return 3; });
    }
    catch (const std::runtime_error &e)
    {
        caught = true;
        std::cout << "enqueue rejected: " << e.what() << std::endl;
    }

    gate.set_value();

    std::cout << "f1.get() = " << f1.get() << std::endl;
    std::cout << "f2.get() = " << f2.get() << std::endl;
    std::cout << "queue full rejected: " << (caught ? "yes" : "no") << std::endl;

    pool.shutdown();
    std::cout << std::endl;
}

int main()
{
    demo_basic();
    demo_multiple_tasks();
    demo_exception();
    demo_queue_full();

    std::cout << "All demos completed." << std::endl;
    return 0;
}
