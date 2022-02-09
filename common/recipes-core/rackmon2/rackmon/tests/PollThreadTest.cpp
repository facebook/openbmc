// Copyright 2021-present Facebook. All Rights Reserved.
#include "PollThread.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace std::literals;
using namespace testing;
using namespace rackmon;

struct Tester {
  MOCK_METHOD0(do_stuff, void());
};

TEST(PollThreadTest, basic) {
  Tester test_obj;
  EXPECT_CALL(test_obj, do_stuff()).Times(2);
  PollThread<Tester> thr(&Tester::do_stuff, &test_obj, 1s);
  thr.start();
  std::this_thread::sleep_for(2s);
  thr.stop();
}
