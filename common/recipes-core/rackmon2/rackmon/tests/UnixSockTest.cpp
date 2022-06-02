// Copyright 2021-present Facebook. All Rights Reserved.
#include "UnixSock.h"
#include <thread>
#include <condition_variable>
#include <mutex>
#include <gtest/gtest.h>

using namespace rackmonsvc;

class TestService : public UnixService {
  public:
  TestService() : UnixService("./test.sock") {}
  void handleRequest(const std::vector<char>& req, std::unique_ptr<UnixSock> cli) override {
    std::vector<char> resp(req);
    std::reverse(resp.begin(), resp.end());
    cli->send(resp);
  }
};

class TestClient : public UnixClient {
  public:
  TestClient() : UnixClient("./test.sock") {}
};

TEST(UnixSockTest, BasicLoopback) {
  std::mutex mutex{};
  std::condition_variable cv{};

  std::unique_ptr<TestClient> ptr;
  ASSERT_THROW(ptr = std::make_unique<TestClient>(), std::system_error);
  TestService svc;
  std::thread tid([&svc,&mutex,&cv](){
    svc.initialize(0, nullptr);
    mutex.lock();
    cv.notify_one();
    mutex.unlock();
    svc.doLoop();
    svc.deinitialize();
  });
  {
    std::unique_lock lk(mutex);
    cv.wait(lk);
  }
  ptr = std::make_unique<TestClient>();
  ASSERT_EQ(ptr->request("hello world"), "dlrow olleh");
  ptr = std::make_unique<TestClient>();
  ASSERT_EQ(ptr->request("test"), "tset");
  svc.requestExit();
  tid.join();
}

TEST(UnixSockTest, BasicTERM) {
  std::mutex mutex{};
  std::condition_variable cv{};
  TestService svc;
  std::thread tid([&svc,&mutex,&cv](){
    svc.initialize(0, nullptr);
    mutex.lock();
    cv.notify_one();
    mutex.unlock();
    svc.doLoop();
    svc.deinitialize();
  });
  {
    std::unique_lock lk(mutex);
    cv.wait(lk);
  }
  // raise a signal on self, service should unblock
  raise(SIGTERM);
  svc.requestExit();
  tid.join();
}
