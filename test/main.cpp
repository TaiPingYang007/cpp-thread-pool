#include "ThreadPool.h"

#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace
{
// 统一打印单个测试用例的结果。
void print_result(const std::string &name, bool passed)
{
  std::cout << (passed ? "[PASS] " : "[FAIL] ") << name << '\n';
}

// 验证 thread_count = 0 时会被构造函数拒绝。
bool test_invalid_thread_count()
{
  try
  {
    ThreadPool::Config config;
    config.thread_count = 0;
    ThreadPool pool(config);
    (void)pool;
    return false;
  }
  catch (const std::invalid_argument &)
  {
    return true;
  }
}

// 验证 max_queue_size = 0 时会被构造函数拒绝。
bool test_invalid_queue_size()
{
  try
  {
    ThreadPool::Config config;
    config.max_queue_size = 0;
    ThreadPool pool(config);
    (void)pool;
    return false;
  }
  catch (const std::invalid_argument &)
  {
    return true;
  }
}

// 验证当任务队列已满时，线程池会拒绝继续接收新任务。
bool test_queue_limit_rejects_overflow()
{
  ThreadPool::Config config;
  config.thread_count = 1;
  config.max_queue_size = 1;

  ThreadPool pool(config);

  std::promise<void> release_first_task;
  std::shared_future<void> gate = release_first_task.get_future().share();
  std::atomic<bool> first_task_started{false};

  // 第一个任务先占住唯一的 worker，不让它太快结束。
  auto first = pool.enqueue([&]() {
    first_task_started = true;
    gate.wait();
    return 1;
  });

  // 等到第一个任务确认已经被 worker 取走，再塞第二个任务进队列。
  while (!first_task_started.load())
  {
    std::this_thread::yield();
  }

  // 这时队列中已经有一个排队任务了，再提交第三个任务应该触发拒绝策略。
  auto second = pool.enqueue([]() {
    return 2;
  });

  bool rejected = false;
  try
  {
    auto third = pool.enqueue([]() {
      return 3;
    });
    (void)third;
  }
  catch (const std::runtime_error &)
  {
    rejected = true;
  }

  release_first_task.set_value();
  first.get();
  second.get();
  pool.shutdown();
  return rejected;
}

// 验证 shutdown 会等待旧任务完成，并拒绝 shutdown 之后的新任务。
bool test_shutdown_drains_existing_tasks_and_rejects_new_tasks()
{
  ThreadPool::Config config;
  config.thread_count = 2;
  config.max_queue_size = 8;

  ThreadPool pool(config);
  std::atomic<int> finished{0};
  std::vector<std::future<void>> futures;

  // 先提交一批短任务，观察 shutdown 是否会等它们全部执行完。
  for (int i = 0; i < 4; ++i)
  {
    futures.emplace_back(pool.enqueue([&finished]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      ++finished;
    }));
  }

  pool.shutdown();

  // shutdown 返回后，所有历史任务都应该已经可安全收尾。
  for (auto &future : futures)
  {
    future.wait();
    if (future.valid())
    {
      future.get();
    }
  }

  // shutdown 之后继续 enqueue，应该立即抛异常。
  bool rejected = false;
  try
  {
    auto after_shutdown = pool.enqueue([]() {
      return 42;
    });
    (void)after_shutdown;
  }
  catch (const std::runtime_error &)
  {
    rejected = true;
  }

  return finished.load() == 4 && rejected;
}

// 验证 shutdown 可重复调用，不会因为二次关闭而崩溃。
bool test_shutdown_is_idempotent()
{
  ThreadPool::Config config;
  config.thread_count = 2;
  ThreadPool pool(config);

  auto future = pool.enqueue([]() {
    return 7;
  });

  if (future.get() != 7)
  {
    return false;
  }

  pool.shutdown();
  pool.shutdown();
  return true;
}

// 验证任务内部调用 shutdown 时，线程池会进入关闭状态。
// 这个测试对应的是“worker 线程自己发起关闭”的特殊场景。
bool test_shutdown_called_from_worker()
{
  ThreadPool::Config config;
  config.thread_count = 1;
  config.max_queue_size = 4;

  ThreadPool pool(config);

  // 只有一个 worker，这个任务一定会在唯一的消费者线程中执行。
  auto future = pool.enqueue([&pool]() -> int {
    // 在任务内部主动关闭线程池。
    // 按当前实现，这里不会抛异常，而是会把线程池切换到 stop 状态。
    pool.shutdown();
    return 123;
  });

  // 如果任务能正常返回，说明“worker 内部关闭线程池”这条路径至少没有直接卡死。
  bool current_task_finished = (future.get() == 123);

  // 关闭之后继续提交任务，应该被拒绝。
  bool rejected = false;
  try
  {
    auto after_shutdown = pool.enqueue([]() {
      return 9;
    });
    (void)after_shutdown;
  }
  catch (const std::runtime_error &)
  {
    rejected = true;
  }

  return current_task_finished && rejected;
}
} // namespace

int main()
{
  std::cout << "--- ThreadPool 自测开始 ---\n";

  // 用统一表驱动的方式组织测试，后面继续加用例会更方便。
  struct TestCase
  {
    std::string name;
    bool (*fn)();
  };

  const std::vector<TestCase> tests = {
      {"非法线程数会抛异常", test_invalid_thread_count},
      {"非法队列容量会抛异常", test_invalid_queue_size},
      {"队列满时拒绝新任务", test_queue_limit_rejects_overflow},
      {"shutdown 会等旧任务完成并拒绝新任务", test_shutdown_drains_existing_tasks_and_rejects_new_tasks},
      {"shutdown 可重复调用", test_shutdown_is_idempotent},
      {"任务内部调用 shutdown 会让线程池进入关闭状态", test_shutdown_called_from_worker},
  };

  int passed = 0;
  for (const auto &test : tests)
  {
    bool ok = false;
    try
    {
      ok = test.fn();
    }
    catch (const std::exception &e)
    {
      std::cout << "[FAIL] " << test.name << "，异常信息：" << e.what() << '\n';
      continue;
    }

    print_result(test.name, ok);
    if (ok)
    {
      ++passed;
    }
  }

  std::cout << "--- 自测结束：通过 " << passed << " / " << tests.size() << " ---\n";
  return passed == static_cast<int>(tests.size()) ? 0 : 1;
}
