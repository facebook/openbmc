#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <thread>

template <class T>
class PollThread {
  using poll_interval = std::chrono::seconds;
  std::mutex m{};
  std::condition_variable cv{};
  std::thread tid{};
  std::atomic<bool> started = false;

  std::function<void(T*)> func{};
  T* obj = nullptr;
  poll_interval sleep_time{5};

  void worker() {
    std::unique_lock lk(m);
    while (started.load()) {
      func(obj);
      cv.wait_for(lk, sleep_time, [this]() { return !started.load(); });
    }
  }
  void notify_stop() {
    std::unique_lock lk(m);
    started = false;
    cv.notify_all();
  }

 public:
  PollThread(std::function<void(T*)> fp, T* o, const poll_interval& pi)
      : func(fp), obj(o), sleep_time(pi) {}
  ~PollThread() {
    stop();
  }
  void start() {
    if (!started.load()) {
      started = true;
      tid = std::thread(&PollThread::worker, this);
    }
  }
  void stop() {
    if (started.load()) {
      notify_stop();
      tid.join();
    }
  }
};
