#define PROFILING
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <thread>
#include "../profile.hpp"

using namespace std::literals;

TEST(ProfileTest, Basic) {
  std::stringstream ss;
  { openbmc::Profile p("BASIC", ss); }
  std::string exp = "PROFILE BASIC : 0 ms\n";
  ASSERT_EQ(ss.str(), exp);
}

TEST(ProfileTest, Ext) {
  std::stringstream ss;
  {
    openbmc::Profile p("BASIC", ss);
    std::this_thread::sleep_for(1s);
  }
  std::string exp = "PROFILE BASIC : 1000 ms\n";
  ASSERT_EQ(ss.str(), exp);
}

TEST(ProfileTest, BasicMacro) {
  std::stringstream ss;
  { PROFILE_SCOPE(BASIC, ss); }
  std::string exp = "PROFILE BASIC : 0 ms\n";
  ASSERT_EQ(ss.str(), exp);
}

TEST(ProfileTest, ExtMacro) {
  std::stringstream ss;
  {
    PROFILE_SCOPE(BASIC, ss);
    std::this_thread::sleep_for(1s);
  }
  std::string exp = "PROFILE BASIC : 1000 ms\n";
  ASSERT_EQ(ss.str(), exp);
}
