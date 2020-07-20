#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sstream>
#include "selstream.hpp"

using namespace std;
using namespace testing;

class MockSELFormat : public SELFormat {
 public:
  MockSELFormat(uint8_t fru_id) : SELFormat(fru_id) {}

  MOCK_METHOD1(get_fru_name, string(uint8_t));
  MOCK_METHOD0(get_current_time, string());
};

class MockSELStream : public SELStream {
 public:
  MockSELStream(OutputFormat fmt) : SELStream(fmt) {}
  MOCK_METHOD1(make_sel, std::unique_ptr<SELFormat>(uint8_t));
};

TEST(SELStream, BasicReadWrite) {
  stringstream inp;

  inp << " 2020 May 18 10:18:40 bmc-oob. user.crit fbtp-9b6bf3961d-dirty: healthd: BMC Reboot detected - caused by reboot command\n";
  inp << " 2020 Apr  6 15:00:40 bmc-oob. user.crit fbtp-v2020.09.1: sensord: ASSERT: Upper Non Critical threshold - raised - FRU: 1, num: 0xC0 curr_val: 8988.00 RPM, thresh_val: 8500.00 RPM, snr: MB_FAN0_TACH\n";
  inp << " 2020 May 18 10:18:38 bmc-oob. user.crit fbtp-9b6bf3961d-dirty: ncsid: FRU: 2 NIC AEN Supported: 0x7, AEN Enable Mask=0x7\n";
  inp << "2020 May 21 17:29:55 log-util: User cleared FRU: 2 logs\n";
  stringstream exp;
  exp << "0    all      2020-05-18 10:18:40    healthd          BMC Reboot detected - caused by reboot command\n";
  exp << "1    mb       2020-04-06 15:00:40    sensord          ASSERT: Upper Non Critical threshold - raised - FRU: 1, num: 0xC0 curr_val: 8988.00 RPM, thresh_val: 8500.00 RPM, snr: MB_FAN0_TACH\n";
  exp << "2    nic      2020-05-18 10:18:38    ncsid            FRU: 2 NIC AEN Supported: 0x7, AEN Enable Mask=0x7\n";
  exp << "2020 May 21 17:29:55 log-util: User cleared FRU: 2 logs\n";
  MockSELStream stream(FORMAT_PRINT);

  auto sel = std::make_unique<MockSELFormat>(SELFormat::FRU_ALL);
  EXPECT_CALL(*sel, get_fru_name(AnyOf(1, 2)))
      .Times(3)
      .WillOnce(Return(string("mb")))
      .WillOnce(Return(string("nic")))
      .WillOnce(Return(string("nic")));
  EXPECT_CALL(stream, make_sel(SELFormat::FRU_ALL))
      .Times(1)
      .WillOnce(Return(ByMove(std::move(sel))));
  stringstream outp;
  stream.start(inp, outp, {SELFormat::FRU_ALL});
  stream.flush(outp);
  EXPECT_EQ(outp.str(), exp.str());
}

TEST(SELStream, BasicFilter) {
  stringstream inp1, inp2;

  inp1
      << " 2020 May 18 10:18:40 bmc-oob. user.crit fbtp-9b6bf3961d-dirty: healthd: BMC Reboot detected - caused by reboot command\n";
  inp1
      << " 2020 Apr  6 15:00:40 bmc-oob. user.crit fbtp-v2020.09.1: sensord: ASSERT: Upper Non Critical threshold - raised - FRU: 1, num: 0xC0 curr_val: 8988.00 RPM, thresh_val: 8500.00 RPM, snr: MB_FAN0_TACH\n";
  inp2
      << " 2020 May 18 10:18:38 bmc-oob. user.crit fbtp-9b6bf3961d-dirty: ncsid: FRU: 2 NIC AEN Supported: 0x7, AEN Enable Mask=0x7\n";
  inp2 << "2020 May 21 17:29:55 log-util: User cleared FRU: 2 logs\n";
  stringstream exp;
  exp << "2    nic      2020-05-18 10:18:38    ncsid            FRU: 2 NIC AEN Supported: 0x7, AEN Enable Mask=0x7\n";
  exp << "2020 May 21 17:29:55 log-util: User cleared FRU: 2 logs\n";
  MockSELStream stream(FORMAT_PRINT);

  auto sel1 = std::make_unique<MockSELFormat>(SELFormat::FRU_ALL);
  auto sel2 = std::make_unique<MockSELFormat>(SELFormat::FRU_ALL);
  EXPECT_CALL(*sel1, get_fru_name(1)).Times(1).WillOnce(Return(string("mb")));
  EXPECT_CALL(*sel2, get_fru_name(2)).WillRepeatedly(Return(string("nic")));
  EXPECT_CALL(stream, make_sel(SELFormat::FRU_ALL))
      .Times(2)
      .WillOnce(Return(ByMove(std::move(sel1))))
      .WillOnce(Return(ByMove(std::move(sel2))));
  stringstream outp;
  stream.start(inp1, outp, {2});
  stream.start(inp2, outp, {2});
  stream.flush(outp);
  EXPECT_EQ(outp.str(), exp.str());
}

TEST(SELStream, BasicJSON) {
  stringstream inp;

  inp << " 2020 May 18 10:18:40 bmc-oob. user.crit fbtp-9b6bf3961d-dirty: healthd: BMC Reboot detected - caused by reboot command\n";
  inp << " 2020 Apr  6 15:00:40 bmc-oob. user.crit fbtp-v2020.09.1: sensord: ASSERT: Upper Non Critical threshold - raised - FRU: 1, num: 0xC0 curr_val: 8988.00 RPM, thresh_val: 8500.00 RPM, snr: MB_FAN0_TACH\n";
  inp << " 2020 May 18 10:18:38 bmc-oob. user.crit fbtp-9b6bf3961d-dirty: ncsid: FRU: 2 NIC AEN Supported: 0x7, AEN Enable Mask=0x7\n";
  inp << "2020 May 21 17:29:55 log-util: User cleared FRU: 2 logs\n";
  MockSELStream stream(FORMAT_JSON);

  auto sel = std::make_unique<MockSELFormat>(SELFormat::FRU_ALL);
  EXPECT_CALL(*sel, get_fru_name(AnyOf(1, 2)))
      .Times(3)
      .WillOnce(Return(string("mb")))
      .WillOnce(Return(string("nic")))
      .WillOnce(Return(string("nic")));
  EXPECT_CALL(stream, make_sel(SELFormat::FRU_ALL))
      .Times(1)
      .WillOnce(Return(ByMove(std::move(sel))));
  stringstream outp;
  stream.start(inp, outp, {SELFormat::FRU_ALL});
  stream.flush(outp);

  nlohmann::json got = nlohmann::json::parse(outp.str());
  EXPECT_NE(got.find("Logs"), got.end());
  EXPECT_EQ(got["Logs"].size(), 3);
  EXPECT_EQ(
      got["Logs"][0]["MESSAGE"],
      "BMC Reboot detected - caused by reboot command");
  EXPECT_EQ(got["Logs"][1]["APP_NAME"], "sensord");
  EXPECT_EQ(
      string(got["Logs"][2]["MESSAGE"]),
      "FRU: 2 NIC AEN Supported: 0x7, AEN Enable Mask=0x7");
  EXPECT_EQ(got["Logs"][0]["FRU#"], "0");
  EXPECT_EQ(got["Logs"][1]["FRU_NAME"], "mb");
  EXPECT_EQ(
      got["Logs"][1]["MESSAGE"],
      "ASSERT: Upper Non Critical threshold - raised - FRU: 1, num: 0xC0 curr_val: 8988.00 RPM, thresh_val: 8500.00 RPM, snr: MB_FAN0_TACH");
}

TEST(SELStream, ClearAll) {
  MockSELStream stream(FORMAT_RAW);

  auto sel = std::make_unique<MockSELFormat>(SELFormat::FRU_ALL);
  EXPECT_CALL(*sel, get_fru_name(AnyOf(1, 2)))
      .Times(3)
      .WillOnce(Return(string("mb")))
      .WillOnce(Return(string("nic")))
      .WillOnce(Return(string("nic")));
  auto sel2 = std::make_unique<MockSELFormat>(SELFormat::FRU_ALL);
  EXPECT_CALL(*sel2, get_current_time())
      .Times(1)
      .WillOnce(Return(string("2020 Jun 21 17:29:55")));
  EXPECT_CALL(stream, make_sel(SELFormat::FRU_ALL))
      .Times(2)
      .WillOnce(Return(ByMove(std::move(sel))))
      .WillOnce(Return(ByMove(std::move(sel2))));

  stringstream inp;

  inp << " 2020 May 18 10:18:40 bmc-oob. user.crit fbtp-9b6bf3961d-dirty: healthd: BMC Reboot detected - caused by reboot command\n";
  inp << " 2020 Apr  6 15:00:40 bmc-oob. user.crit fbtp-v2020.09.1: sensord: ASSERT: Upper Non Critical threshold - raised - FRU: 1, num: 0xC0 curr_val: 8988.00 RPM, thresh_val: 8500.00 RPM, snr: MB_FAN0_TACH\n";
  inp << " 2020 May 18 10:18:38 bmc-oob. user.crit fbtp-9b6bf3961d-dirty: ncsid: FRU: 2 NIC AEN Supported: 0x7, AEN Enable Mask=0x7\n";
  inp << "2020 May 21 17:29:55 log-util: User cleared FRU: 2 logs\n";

  stringstream outp;
  stream.start(inp, outp, {SELFormat::FRU_ALL});
  stream.log_cleared(outp, {SELFormat::FRU_ALL});
  stream.flush(outp);
  stringstream exp;
  exp << "2020 Jun 21 17:29:55 log-util: User cleared all logs\n";
  ASSERT_EQ(outp.str(), exp.str());
}

TEST(SELStream, ClearFru) {
  auto sel = std::make_unique<MockSELFormat>(SELFormat::FRU_ALL);
  EXPECT_CALL(*sel, get_fru_name(AnyOf(1, 2)))
      .Times(3)
      .WillOnce(Return(string("mb")))
      .WillOnce(Return(string("nic")))
      .WillOnce(Return(string("nic")));
  auto sel2 = std::make_unique<MockSELFormat>(SELFormat::FRU_ALL);
  EXPECT_CALL(*sel2, get_fru_name(2)).Times(1).WillOnce(Return(string("nic")));
  EXPECT_CALL(*sel2, get_current_time())
      .Times(1)
      .WillOnce(Return(string("2020 Jun 21 17:29:55")));

  MockSELStream stream(FORMAT_RAW);
  EXPECT_CALL(stream, make_sel(AnyOf(SELFormat::FRU_ALL, 2)))
      .Times(2)
      .WillOnce(Return(ByMove(std::move(sel))))
      .WillOnce(Return(ByMove(std::move(sel2))));

  stringstream inp;
  inp << " 2020 May 18 10:18:40 bmc-oob. user.crit fbtp-9b6bf3961d-dirty: healthd: BMC Reboot detected - caused by reboot command\n";
  inp << " 2020 Apr  6 15:00:40 bmc-oob. user.crit fbtp-v2020.09.1: sensord: ASSERT: Upper Non Critical threshold - raised - FRU: 1, num: 0xC0 curr_val: 8988.00 RPM, thresh_val: 8500.00 RPM, snr: MB_FAN0_TACH\n";
  inp << " 2020 May 18 10:18:38 bmc-oob. user.crit fbtp-9b6bf3961d-dirty: ncsid: FRU: 2 NIC AEN Supported: 0x7, AEN Enable Mask=0x7\n";
  inp << "2020 May 21 17:29:55 log-util: User cleared FRU: 2 logs\n";

  stringstream outp;
  stream.start(inp, outp, {2});
  stream.log_cleared(outp, {2});
  stream.flush(outp);
  stringstream exp;
  exp << " 2020 May 18 10:18:40 bmc-oob. user.crit fbtp-9b6bf3961d-dirty: healthd: BMC Reboot detected - caused by reboot command\n";
  exp << " 2020 Apr  6 15:00:40 bmc-oob. user.crit fbtp-v2020.09.1: sensord: ASSERT: Upper Non Critical threshold - raised - FRU: 1, num: 0xC0 curr_val: 8988.00 RPM, thresh_val: 8500.00 RPM, snr: MB_FAN0_TACH\n";
  exp << "2020 Jun 21 17:29:55 log-util: User cleared FRU: 2 logs\n";
  ASSERT_EQ(outp.str(), exp.str());
}
