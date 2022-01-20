#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "PollThread.hpp"

using namespace std::literals;
using namespace testing;

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
