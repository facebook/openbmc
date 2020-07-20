#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "rsyslogd.hpp"

using namespace std;
using namespace testing;

class MockRsyslogd : public rsyslogd {
 public:
  MockRsyslogd() : rsyslogd() {}
  MOCK_METHOD0(getpid, int());
  MOCK_METHOD1(runcmd, void(const std::string&));
  MOCK_METHOD2(runcmd, void(const std::string&, std::string&));
};

TEST(rsyslogdTest, BasicCallTest) {
  MockRsyslogd r;
  EXPECT_CALL(r, getpid()).Times(1).WillOnce(Return(42));
  EXPECT_CALL(r, runcmd("kill -HUP 42")).Times(1);
  r.reload();
}

class Mock2Rsyslogd : public rsyslogd {
 public:
  Mock2Rsyslogd() : rsyslogd() {}
  MOCK_METHOD1(runcmd, void(const std::string&));
  MOCK_METHOD2(runcmd, void(const std::string&, std::string&));
};

TEST(rsyslogdTest, FullCallTest) {
  Mock2Rsyslogd r;
  // EXPECT_CALL(r, getpid()).WillOnce([&r]() {return r.rsyslogd::getpid();});
  EXPECT_CALL(r, runcmd("pidof rsyslogd", _))
      .Times(1)
      .WillRepeatedly(SetArgReferee<1>("42"));
  EXPECT_CALL(r, runcmd("kill -HUP 42")).Times(1);
  r.reload();
}
