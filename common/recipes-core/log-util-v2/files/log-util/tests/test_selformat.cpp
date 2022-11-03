#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "selformat.hpp"

// NOTE:
// Constant strings being compared against the test
// output were copied from a run of log-util-v1.
// This is specifically done to ensure that we are string
// compatible with log-util-v1 and user wont know the
// difference between V1 output and V2 output.
// DO NOT CHANGE THE CONSTANT STRINGS to "pass" your code.

using namespace std;
using namespace testing;

TEST(SELFormat, leftAlignTest) {
  EXPECT_EQ(SELFormat::left_align("HELLO", 5), "HELLO");
  EXPECT_EQ(SELFormat::left_align("HELLO", 4), "HELLO");
  EXPECT_EQ(SELFormat::left_align("HELLO", 6), "HELLO ");
  EXPECT_EQ(SELFormat::left_align("HELLO", 7), "HELLO  ");
}

TEST(SELFormat, headerTests) {
  // The constant string we are testing against is copied
  // from the output of log-util v1. Do not change it to pass
  // your test! We need this to keep log-util string compatible
  // with v1.
  EXPECT_EQ(
      SELFormat::get_header(),
      "FRU# FRU_NAME TIME_STAMP             APP_NAME         MESSAGE");
}

class MockSELFormat : public SELFormat {
 public:
  MockSELFormat(uint8_t fru_id) : SELFormat(fru_id) {}

  MOCK_METHOD1(get_fru_name, string(uint8_t));
  MOCK_METHOD0(get_current_time, string());
};

TEST(SELFormat, BasicSysFormatTestsAll) {
  MockSELFormat sel(SELFormat::FRU_ALL);
  MockSELFormat sel2(SELFormat::FRU_SYS);
  EXPECT_EQ(sel.is_bare(), true);
  EXPECT_EQ(sel.fru_id(), SELFormat::FRU_ALL);
  EXPECT_EQ(sel2.fru_id(), SELFormat::FRU_SYS);

  string raw(
      " 2020 May 18 10:18:40 bmc-oob. user.crit fbtp-9b6bf3961d-dirty: healthd: BMC Reboot detected - caused by reboot command");
  sel.set_raw(std::move(raw));
  string raw2(
      " 2020 May 18 10:18:40 bmc-oob. user.crit fbtp-9b6bf3961d-dirty: healthd: BMC Reboot detected - caused by reboot command");
  sel2.set_raw(std::move(raw));

  EXPECT_EQ(sel.is_self(), false);
  EXPECT_EQ(sel.is_bare(), false);
  EXPECT_EQ(sel.fru_id(), SELFormat::FRU_ALL);
  EXPECT_EQ(sel2.fru_id(), SELFormat::FRU_ALL);
  EXPECT_EQ(sel.fru_name(), "all");
  EXPECT_EQ(sel2.fru_name(), "sys");
  EXPECT_EQ(sel.time_stamp(), "2020-05-18 10:18:40");
  EXPECT_EQ(sel.hostname(), "bmc-oob.");
  EXPECT_EQ(sel.version(), "fbtp-9b6bf3961d-dirty");
  EXPECT_EQ(sel.app(), "healthd");
  EXPECT_EQ(sel.msg(), "BMC Reboot detected - caused by reboot command");
  string exp(
      "0    all      2020-05-18 10:18:40    healthd          BMC Reboot detected - caused by reboot command");
  EXPECT_EQ(sel.str(), exp);
  string exp2(
      "0    sys      2020-05-18 10:18:40    healthd          BMC Reboot detected - caused by reboot command");
  EXPECT_EQ(sel2.str(), exp2);

  nlohmann::json obj1;
  sel.json(obj1);
  EXPECT_EQ(obj1["APP_NAME"], "healthd");
  EXPECT_EQ(obj1["TIME_STAMP"], "2020-05-18 10:18:40");
  EXPECT_EQ(obj1["FRU#"], "0");
  EXPECT_EQ(obj1["FRU_NAME"], "all");
  EXPECT_EQ(obj1["MESSAGE"], "BMC Reboot detected - caused by reboot command");

  nlohmann::json obj2;
  sel2.json(obj2);
  EXPECT_EQ(obj2["APP_NAME"], "healthd");
  EXPECT_EQ(obj2["TIME_STAMP"], "2020-05-18 10:18:40");
  EXPECT_EQ(obj2["FRU#"], "0");
  EXPECT_EQ(obj2["FRU_NAME"], "sys");
  EXPECT_EQ(obj2["MESSAGE"], "BMC Reboot detected - caused by reboot command");
}

TEST(SELFormat, BasicFruFormatTests) {
  MockSELFormat sel(SELFormat::FRU_ALL);
  EXPECT_EQ(sel.is_bare(), true);
  EXPECT_EQ(sel.fru_id(), SELFormat::FRU_ALL);
  EXPECT_CALL(sel, get_fru_name(2)).Times(1).WillOnce(Return(string("nic")));

  string raw(
      " 2020 May 18 10:18:37 bmc-oob. user.crit fbtp-9b6bf3961d-dirty: ncsid: FRU: 2 NIC AEN Supported: 0x7, AEN Enable Mask=0x7");
  sel.set_raw(std::move(raw));

  EXPECT_EQ(sel.is_bare(), false);
  EXPECT_EQ(sel.fru_id(), 2);
  EXPECT_EQ(sel.fru_name(), "nic");
  EXPECT_EQ(sel.is_self(), false);
  EXPECT_EQ(sel.time_stamp(), "2020-05-18 10:18:37");
  EXPECT_EQ(sel.hostname(), "bmc-oob.");
  EXPECT_EQ(sel.version(), "fbtp-9b6bf3961d-dirty");
  EXPECT_EQ(sel.app(), "ncsid");
  EXPECT_EQ(sel.msg(), "FRU: 2 NIC AEN Supported: 0x7, AEN Enable Mask=0x7");
  string exp(
      "2    nic      2020-05-18 10:18:37    ncsid            FRU: 2 NIC AEN Supported: 0x7, AEN Enable Mask=0x7");
  EXPECT_EQ(sel.str(), exp);

  nlohmann::json obj;
  sel.json(obj);
  EXPECT_EQ(obj["APP_NAME"], "ncsid");
  EXPECT_EQ(obj["TIME_STAMP"], "2020-05-18 10:18:37");
  EXPECT_EQ(obj["FRU#"], "2");
  EXPECT_EQ(obj["FRU_NAME"], "nic");
  EXPECT_EQ(
      obj["MESSAGE"], "FRU: 2 NIC AEN Supported: 0x7, AEN Enable Mask=0x7");
}

TEST(SELFormat, BasicSelFormatTests) {
  MockSELFormat sel(SELFormat::FRU_ALL);
  string raw("2020 May 21 17:29:55 log-util: User cleared FRU: 2 logs");
  EXPECT_CALL(sel, get_fru_name(2)).Times(1).WillOnce(Return(string("nic")));
  sel.set_raw(std::move(raw));

  EXPECT_EQ(sel.is_self(), true);
  EXPECT_EQ(sel.is_bare(), true);
  EXPECT_EQ(sel.raw(), raw);
  EXPECT_EQ(sel.str(), raw);
}

TEST(SELFormat, BasicClearFormatTests) {
  MockSELFormat sel(SELFormat::FRU_ALL);

  EXPECT_CALL(sel, get_fru_name(2)).Times(1).WillOnce(Return(string("nic")));
  EXPECT_CALL(sel, get_current_time())
      .Times(1)
      .WillOnce(Return(string("2020 May 21 17:29:55")));
  sel.set_clear(2);
  EXPECT_EQ(sel.is_self(), true);
  EXPECT_EQ(sel.is_bare(), true);
  EXPECT_EQ(sel.fru_id(), 2);
  string raw("2020 May 21 17:29:55 log-util: User cleared FRU: 2 logs");
  EXPECT_EQ(sel.raw(), raw);
  EXPECT_EQ(sel.str(), raw);
}

TEST(SELFormat, BasicClearAllFormatTests) {
  MockSELFormat sel(SELFormat::FRU_ALL);

  EXPECT_CALL(sel, get_current_time())
      .Times(1)
      .WillOnce(Return(string("2020 May 21 17:29:55")));
  sel.set_clear(SELFormat::FRU_ALL);
  EXPECT_EQ(sel.is_self(), true);
  EXPECT_EQ(sel.is_bare(), true);
  EXPECT_EQ(sel.fru_id(), SELFormat::FRU_ALL);
  string raw("2020 May 21 17:29:55 log-util: User cleared all logs");
  EXPECT_EQ(sel.raw(), raw);
  EXPECT_EQ(sel.str(), raw);
}

TEST(SELFormat, BasicClearSysFormatTests) {
  MockSELFormat sel(SELFormat::FRU_ALL);

  EXPECT_CALL(sel, get_current_time())
      .Times(1)
      .WillOnce(Return(string("2020 May 21 17:29:55")));
  sel.set_clear(SELFormat::FRU_SYS);
  EXPECT_EQ(sel.is_self(), true);
  EXPECT_EQ(sel.is_bare(), true);
  EXPECT_EQ(sel.fru_id(), SELFormat::FRU_ALL);
  string raw("2020 May 21 17:29:55 log-util: User cleared sys logs");
  EXPECT_EQ(sel.raw(), raw);
  EXPECT_EQ(sel.str(), raw);
}

TEST(SELFormat, TimestampCompare) {
  MockSELFormat sel(SELFormat::FRU_ALL);

  string raw(
          " 2020 May 18 10:18:40 bmc-oob. user.crit fbtp-9b6bf3961d-dirty: healthd: BMC Reboot detected - caused by reboot command");
  sel.set_raw(std::move(raw));

  EXPECT_TRUE(sel.fits_time_range("2020-01-01 00:00:00", "2020-12-31 23:59:59"));
  EXPECT_FALSE(sel.fits_time_range("2021-01-01 00:00:00", "2021-12-31 23:59:59"));
}


