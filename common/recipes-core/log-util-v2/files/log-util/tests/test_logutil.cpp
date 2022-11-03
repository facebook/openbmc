#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <fstream>
#include <sstream>
#include "log-util.hpp"

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

class MockLogUtil : public LogUtil {
 public:
  MockLogUtil() : LogUtil() {}
  MOCK_METHOD1(make_stream, std::unique_ptr<SELStream>(OutputFormat));
  MOCK_METHOD0(make_rsyslogd, std::unique_ptr<rsyslogd>());
  MOCK_METHOD0(logfile_list, const std::vector<std::string>&());
};

class MockRsyslog : public rsyslogd {
 public:
  MockRsyslog() : rsyslogd() {}
  MOCK_METHOD0(reload, void());
};

class LogPrintTest : public ::testing::Test {
 protected:
  const std::vector<std::string> logfiles = {"./logfile.0", "./logfile"};
  std::unique_ptr<MockLogUtil> logutil = nullptr;
  uint8_t my_fru_id = SELFormat::FRU_ALL;
  LogPrintTest() {}

  void make_logutil(
      uint8_t my_fru_id = SELFormat::FRU_ALL,
      OutputFormat fmt = FORMAT_PRINT) {
    logutil = make_unique<MockLogUtil>();
    auto sel1 = std::make_unique<MockSELFormat>(my_fru_id);
    EXPECT_CALL(*sel1, get_fru_name(1)).Times(1).WillOnce(Return(string("mb")));
    auto sel2 = std::make_unique<MockSELFormat>(my_fru_id);
    EXPECT_CALL(*sel2, get_fru_name(2))
        .Times(2)
        .WillRepeatedly(Return(string("nic")));
    auto stream = std::make_unique<MockSELStream>(fmt);
    EXPECT_CALL(*stream, make_sel(my_fru_id))
        .Times(2)
        .WillOnce(Return(ByMove(std::move(sel1))))
        .WillOnce(Return(ByMove(std::move(sel2))));
    EXPECT_CALL(*logutil, make_stream(fmt))
        .Times(1)
        .WillOnce(Return(ByMove(std::move(stream))));
    EXPECT_CALL(*logutil, logfile_list())
        .Times(1)
        .WillRepeatedly(ReturnRef(logfiles));
  }

  void SetUp() {
    ofstream ofs;

    ofs.open("./logfile.0");
    ofs << " 2020 May 18 10:18:40 bmc-oob. user.crit fbtp-9b6bf3961d-dirty: healthd: BMC Reboot detected - caused by reboot command\n";
    ofs << " 2020 Apr  6 15:00:40 bmc-oob. user.crit fbtp-v2020.09.1: sensord: ASSERT: Upper Non Critical threshold - raised - FRU: 1, num: 0xC0 curr_val: 8988.00 RPM, thresh_val: 8500.00 RPM, snr: MB_FAN0_TACH\n";
    ofs.close();
    ofs.open("./logfile");
    ofs << " 2020 May 18 10:18:38 bmc-oob. user.crit fbtp-9b6bf3961d-dirty: ncsid: FRU: 2 NIC AEN Supported: 0x7, AEN Enable Mask=0x7\n";
    ofs << "2020 May 21 17:29:55 log-util: User cleared FRU: 2 logs\n";
    ofs.close();
  }
  void TearDown() {
    remove("./logfile");
    remove("./logfile.0");
    logutil = nullptr;
  }
};

TEST_F(LogPrintTest, BasicPrintAll) {
  stringstream outp;
  make_logutil();
  logutil->print({SELFormat::FRU_ALL}, "", "", false, outp);

  stringstream exp;
  exp << "0    all      2020-05-18 10:18:40    healthd          BMC Reboot detected - caused by reboot command\n";
  exp << "1    mb       2020-04-06 15:00:40    sensord          ASSERT: Upper Non Critical threshold - raised - FRU: 1, num: 0xC0 curr_val: 8988.00 RPM, thresh_val: 8500.00 RPM, snr: MB_FAN0_TACH\n";
  exp << "2    nic      2020-05-18 10:18:38    ncsid            FRU: 2 NIC AEN Supported: 0x7, AEN Enable Mask=0x7\n";
  exp << "2020 May 21 17:29:55 log-util: User cleared FRU: 2 logs\n";

  EXPECT_EQ(outp.str(), exp.str());
}

TEST_F(LogPrintTest, BasicPrintSome) {
  stringstream outp;
  make_logutil();
  logutil->print({2}, "", "", false, outp);

  stringstream exp;
  exp << "2    nic      2020-05-18 10:18:38    ncsid            FRU: 2 NIC AEN Supported: 0x7, AEN Enable Mask=0x7\n";
  exp << "2020 May 21 17:29:55 log-util: User cleared FRU: 2 logs\n";

  EXPECT_EQ(outp.str(), exp.str());
}

TEST_F(LogPrintTest, BasicPrintSys) {
  stringstream outp;
  make_logutil(SELFormat::FRU_SYS);
  logutil->print({SELFormat::FRU_SYS}, "", "", false, outp);

  stringstream exp;
  exp << "0    sys      2020-05-18 10:18:40    healthd          BMC Reboot detected - caused by reboot command\n";

  EXPECT_EQ(outp.str(), exp.str());
}

TEST_F(LogPrintTest, BasicPrintTimestamp) {
  stringstream outp;
  make_logutil();
  logutil->print({SELFormat::FRU_ALL}, "2020-05-18 00:00:00", "2020-05-19 00:00:00", false, outp);

  stringstream exp;
  exp << "0    all      2020-05-18 10:18:40    healthd          BMC Reboot detected - caused by reboot command\n";
  exp << "2    nic      2020-05-18 10:18:38    ncsid            FRU: 2 NIC AEN Supported: 0x7, AEN Enable Mask=0x7\n";

  EXPECT_EQ(outp.str(), exp.str());

}

class LogClearTest : public ::testing::Test {
 protected:
  const std::vector<std::string> logfiles = {"./logfile.0", "./logfile"};
  std::unique_ptr<MockLogUtil> logutil = nullptr;
  LogClearTest() {}

  void make_logutil(uint8_t my_fru_id, uint8_t cleared_fru) {
    logutil = make_unique<MockLogUtil>();
    auto sel1 = std::make_unique<MockSELFormat>(my_fru_id);
    EXPECT_CALL(*sel1, get_fru_name(1)).Times(1).WillOnce(Return(string("mb")));
    auto sel2 = std::make_unique<MockSELFormat>(my_fru_id);
    EXPECT_CALL(*sel2, get_fru_name(2))
        .Times(2)
        .WillRepeatedly(Return(string("nic")));
    auto sel3 = std::make_unique<MockSELFormat>(my_fru_id);
    if (cleared_fru != SELFormat::FRU_ALL &&
        cleared_fru != SELFormat::FRU_SYS) {
      EXPECT_CALL(*sel3, get_fru_name(cleared_fru))
          .Times(1)
          .WillRepeatedly(Return(string("nic")));
    }
    EXPECT_CALL(*sel3, get_current_time())
        .Times(1)
        .WillOnce(Return(string("2020 Jun 21 17:29:55")));
    auto stream = std::make_unique<MockSELStream>(FORMAT_RAW);
    auto rslog = std::make_unique<MockRsyslog>();
    EXPECT_CALL(*rslog, reload()).Times(1);
    EXPECT_CALL(
        *stream, make_sel(AnyOf(SELFormat::FRU_ALL, SELFormat::FRU_SYS)))
        .Times(3)
        .WillOnce(Return(ByMove(std::move(sel1))))
        .WillOnce(Return(ByMove(std::move(sel2))))
        .WillOnce(Return(ByMove(std::move(sel3))));
    EXPECT_CALL(*logutil, make_stream(FORMAT_RAW))
        .Times(1)
        .WillOnce(Return(ByMove(std::move(stream))));
    EXPECT_CALL(*logutil, logfile_list())
        .Times(1)
        .WillRepeatedly(ReturnRef(logfiles));
    EXPECT_CALL(*logutil, make_rsyslogd())
        .Times(1)
        .WillRepeatedly(Return(ByMove(std::move(rslog))));
  }

  void SetUp() {
    ofstream ofs;

    ofs.open("./logfile.0");
    ofs << " 2020 May 18 10:18:40 bmc-oob. user.crit fbtp-9b6bf3961d-dirty: healthd: BMC Reboot detected - caused by reboot command\n";
    ofs << " 2020 Apr  6 15:00:40 bmc-oob. user.crit fbtp-v2020.09.1: sensord: ASSERT: Upper Non Critical threshold - raised - FRU: 1, num: 0xC0 curr_val: 8988.00 RPM, thresh_val: 8500.00 RPM, snr: MB_FAN0_TACH\n";
    ofs.close();
    ofs.open("./logfile");
    ofs << " 2020 May 18 10:18:38 bmc-oob. user.crit fbtp-9b6bf3961d-dirty: ncsid: FRU: 2 NIC AEN Supported: 0x7, AEN Enable Mask=0x7\n";
    ofs << "2020 May 21 17:29:55 log-util: User cleared FRU: 2 logs\n";
    ofs.close();
  }
  void TearDown() {
    remove("./logfile");
    remove("./logfile.0");
    logutil = nullptr;
  }
};

TEST_F(LogClearTest, BasicClearAll) {
  make_logutil(SELFormat::FRU_ALL, SELFormat::FRU_ALL);
  logutil->clear({SELFormat::FRU_ALL}, "", "");
  ifstream ifs;
  ifs.open("./logfile.0");
  std::string str;
  str.assign(
      (std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  ifs.close();
  EXPECT_EQ(str, "");
  ifs.open("./logfile");
  str.assign(
      (std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  ifs.close();
  EXPECT_EQ(str, "2020 Jun 21 17:29:55 log-util: User cleared all logs\n");
}

TEST_F(LogClearTest, BasicClearSys) {
  make_logutil(SELFormat::FRU_SYS, SELFormat::FRU_SYS);
  logutil->clear({SELFormat::FRU_SYS}, "", "");
  ifstream ifs;
  ifs.open("./logfile.0");
  std::string str;
  str.assign(
      (std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  ifs.close();
  stringstream exp;
  exp << " 2020 Apr  6 15:00:40 bmc-oob. user.crit fbtp-v2020.09.1: sensord: ASSERT: Upper Non Critical threshold - raised - FRU: 1, num: 0xC0 curr_val: 8988.00 RPM, thresh_val: 8500.00 RPM, snr: MB_FAN0_TACH\n";
  EXPECT_EQ(str, exp.str());
  ifs.open("./logfile");
  str.assign(
      (std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  ifs.close();
  exp.str(std::string());
  exp << " 2020 May 18 10:18:38 bmc-oob. user.crit fbtp-9b6bf3961d-dirty: ncsid: FRU: 2 NIC AEN Supported: 0x7, AEN Enable Mask=0x7\n";
  exp << "2020 May 21 17:29:55 log-util: User cleared FRU: 2 logs\n";
  exp << "2020 Jun 21 17:29:55 log-util: User cleared sys logs\n";
  EXPECT_EQ(str, exp.str());
}

TEST_F(LogClearTest, BasicClearFru) {
  make_logutil(SELFormat::FRU_ALL, 2);
  logutil->clear({2}, "", "");
  ifstream ifs;
  ifs.open("./logfile.0");
  std::string str;
  str.assign(
      (std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  ifs.close();
  stringstream exp;
  exp << " 2020 May 18 10:18:40 bmc-oob. user.crit fbtp-9b6bf3961d-dirty: healthd: BMC Reboot detected - caused by reboot command\n";
  exp << " 2020 Apr  6 15:00:40 bmc-oob. user.crit fbtp-v2020.09.1: sensord: ASSERT: Upper Non Critical threshold - raised - FRU: 1, num: 0xC0 curr_val: 8988.00 RPM, thresh_val: 8500.00 RPM, snr: MB_FAN0_TACH\n";
  EXPECT_EQ(str, exp.str());
  ifs.open("./logfile");
  str.assign(
      (std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  ifs.close();
  exp.str(std::string());
  exp << "2020 Jun 21 17:29:55 log-util: User cleared FRU: 2 logs\n";
  EXPECT_EQ(str, exp.str());
}

TEST_F(LogClearTest, BasicClearTimestamp) {
  make_logutil(SELFormat::FRU_ALL, SELFormat::FRU_ALL);
  logutil->clear({SELFormat::FRU_ALL}, "2020-05-18 10:18:39", "2020-05-19 00:00:00");
  ifstream ifs;
  ifs.open("./logfile.0");
  std::string str;
  str.assign(
      (std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  ifs.close();
  stringstream exp;
  exp << " 2020 Apr  6 15:00:40 bmc-oob. user.crit fbtp-v2020.09.1: sensord: ASSERT: Upper Non Critical threshold - raised - FRU: 1, num: 0xC0 curr_val: 8988.00 RPM, thresh_val: 8500.00 RPM, snr: MB_FAN0_TACH\n";
  EXPECT_EQ(str, exp.str());
  ifs.open("./logfile");
  str.assign(
      (std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  ifs.close();
  exp.str(std::string());
  exp << " 2020 May 18 10:18:38 bmc-oob. user.crit fbtp-9b6bf3961d-dirty: ncsid: FRU: 2 NIC AEN Supported: 0x7, AEN Enable Mask=0x7\n";
  exp << "2020 May 21 17:29:55 log-util: User cleared FRU: 2 logs\n";
  exp << "2020 Jun 21 17:29:55 log-util: User cleared all logs from 2020-05-18 10:18:39 to 2020-05-19 00:00:00\n";
  EXPECT_EQ(str, exp.str());
}
