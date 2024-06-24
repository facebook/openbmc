#include <glog/logging.h>
#include <gtest/gtest.h>

int main(int argc, char** argv) {
  ::google::InitGoogleLogging(argv[0]);
  // Disable all logging in tests.
  FLAGS_minloglevel = google::GLOG_FATAL;

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
