#include "bmc.h"
#include <string>
#include <fcntl.h>
#include <cstdio>
#include <fstream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace std;
using namespace testing;

string file_contents(string name)
{
  ifstream in(name);
  string ret;
  in >> ret;
  in.close();
  return ret;
}

class SystemMock : public System {
  public:
  SystemMock() : System() {}
  SystemMock(std::ostream &out, std::ostream &err): System(out, err) {}

  MOCK_METHOD1(runcmd, int(const string &cmd));
  MOCK_METHOD0(vboot_hardware_enforce, bool());
  MOCK_METHOD2(get_mtd_name, bool(const string name, string &dev));
  MOCK_METHOD0(name, string());
  MOCK_METHOD0(version, string());
  MOCK_METHOD0(partition_conf, string&());
  MOCK_METHOD1(get_fru_id, uint8_t(string &name));
  MOCK_METHOD2(set_update_ongoing, void(uint8_t fruid, int timeo));
  MOCK_METHOD1(lock_file, string(string name));

  int copy_file(string cmd) {
    size_t pos = cmd.find("flashcp -v ");
    if (pos == string::npos) {
      return -1;
    }
    string params = cmd.substr(pos + strlen("flashcp -v "));
    pos = params.find(" ");
    string filesrc = params.substr(0, pos);
    string filedst = params.substr(pos + 1);
    ofstream dst(filedst);
    dst << file_contents(filesrc);
    dst.close();
    return 0;
  }
};

// TEST1: Verify that if the BMC component is created without a version flash
//        device, then it returns the system version.
// TEST2: Verify that if the BMC component is created without a flash MTD device,
//        it does not allow upgrading and returns a not-supported error.
TEST(BmcComponentTest, DefaultSystemVersion) {
  stringstream out, err;
  SystemMock mock(out, err);
  EXPECT_CALL(mock, version()).Times(1).WillOnce(Return(string("fbtp-v10.0")));

  BmcComponent *b = new BmcComponent("bmc_test", "bmc_test", mock, "");
  EXPECT_NE(nullptr, b);
  EXPECT_EQ(0, b->print_version());
  EXPECT_EQ(out.str(), "BMC_TEST Version: fbtp-v10.0\n");
  EXPECT_EQ(err.str(), "");
  EXPECT_EQ(FW_STATUS_NOT_SUPPORTED, b->update("unused"));
  delete b;
}

class TmpFile {
  public:
  string name;
  TmpFile(string contents) {
    name = std::tmpnam(nullptr);
    {
      ofstream mtd(name);
      mtd << contents;
      mtd.close();
    }
  }
  string read() {
    return file_contents(name);
  }
  ~TmpFile() {
    remove(name.c_str());
  }
};

// TEST1: Ensure that when the vers MTD is provided to BMC during
//        creation, then it reads the BMC version from it.
TEST(BmcComponentTest, MTDVersion) {
  stringstream out, err;
  SystemMock mock(out, err);
  TmpFile mtd("U-Boot 2016.07 fbtp-v11.0");

  EXPECT_CALL(mock, get_mtd_name(string("flash123"), _))
    .Times(2)
    .WillOnce(Return(false))
    .WillOnce(DoAll(SetArgReferee<1>(mtd.name), Return(true)));

  BmcComponent *b = new BmcComponent("bmc_test", "bmc_test", mock, "", "flash123");
  EXPECT_EQ(0, b->print_version());
  EXPECT_EQ(out.str(), "BMC_TEST Version: NA\n");
  EXPECT_EQ(err.str(), "");
  out.str("");
  err.str("");
  EXPECT_EQ(0, b->print_version());
  EXPECT_EQ(out.str(), "BMC_TEST Version: fbtp-v11.0\n");
  EXPECT_EQ(err.str(), "");
  delete b;
}

class BmcComponentMock : public BmcComponent {
  public:
    BmcComponentMock(std::string fru, std::string comp, System &sys, std::string mtd, std::string vers = "", size_t w_offset = 0, size_t skip_offset = 0)
      : BmcComponent(fru, comp, sys, mtd, vers, w_offset, skip_offset) {}
  MOCK_METHOD1(is_valid, bool(string &image));
  MOCK_METHOD1(update, int(string &image));
  MOCK_METHOD0(print_version, int());

  int real_update(string &image) {
    return BmcComponent::update(image);
  }
};

// TEST1: Check if image validation fails, update will fail with the correct error message.
// TEST2: Check if the above test succeeds, but get_mtd_name fails, update will fail with the correct error message.
// TEST3: Check if the above tests succeeds, but runcmd fails update will fail.
// TEST4: Check if the above tests succeeds, bmc is flashed to the correct MTD device (Correct call to flashcp)
TEST(BmcComponentTest, MTDFlash) {
  stringstream out, err;
  SystemMock mock(out, err);
  string dummy_mtd("flash123");
  string dummy_image("blahimage");
  string name("fbtp");
  string version = name + "-4.9";
  TmpFile mtd("U-Boot 2016.07 fbtp-v11.0");
  string exp_flashcp_call("flashcp -v " + dummy_image + " " + mtd.name);

  EXPECT_CALL(mock, version())
    .Times(1)
    .WillRepeatedly(Return(version));

  EXPECT_CALL(mock, get_mtd_name(dummy_mtd, _))
    .Times(3)
    .WillOnce(Return(false))
    .WillOnce(DoAll(SetArgReferee<1>(mtd.name), Return(true)))
    .WillOnce(DoAll(SetArgReferee<1>(mtd.name), Return(true)));

  EXPECT_CALL(mock, runcmd(string(exp_flashcp_call)))
    .Times(2)
    .WillOnce(Return(-1))
    .WillOnce(Return(0));

  BmcComponentMock b("bmc_test", "bmc_test", mock, dummy_mtd);

  EXPECT_CALL(b, update(dummy_image))
    .Times(4)
    .WillRepeatedly(Invoke(&b, &BmcComponentMock::real_update));

  EXPECT_CALL(b, is_valid(dummy_image))
    .Times(4)
    .WillOnce(Return(false))
    .WillOnce(Return(true))
    .WillOnce(Return(true))
    .WillOnce(Return(true));

  // First call is_valid will return false. Check error
  EXPECT_EQ(FW_STATUS_FAILURE, b.update(dummy_image));
  EXPECT_EQ(err.str(), dummy_image + " is not a valid BMC image for " + name + "\n");
  err.str("");

  // Second call is_valid() will return true, but get_mtd_name will return false.
  // check error.
  EXPECT_EQ(FW_STATUS_FAILURE, b.update(dummy_image));
  EXPECT_EQ(err.str(), "Failed to get device for " + dummy_mtd + "\n");
  err.str("");

  EXPECT_EQ(FW_STATUS_FAILURE, b.update(dummy_image));

  // Both succeeds. Check if we are calling flashcp correctly.
  EXPECT_EQ(0, b.update(dummy_image));
}

// Test1: Test offseted flash used for verified boot works as expected.
TEST(BmcComponentTest, MTDOffsetFlash) {
  TmpFile image("1234567890"); // 10 byte image.
  TmpFile mtd_dev("abcdef"); // 6 byte mtd
  string exp_image_name = image.name + "-tmp";

  stringstream out;
  SystemMock mock(out, cerr);
  string dummy_mtd("flash123");
  string exp_flashcp_call("flashcp -v " + exp_image_name + " " + mtd_dev.name);

  EXPECT_CALL(mock, get_mtd_name(dummy_mtd, _))
    .Times(1)
    .WillRepeatedly(DoAll(SetArgReferee<1>(mtd_dev.name), Return(true)));

  EXPECT_CALL(mock, runcmd(string(exp_flashcp_call)))
    .Times(1)
    .WillRepeatedly(Invoke(&mock, &SystemMock::copy_file));

  // We are skipping the first 4 bytes. Copying the next 4 from mtd
  // and replacing our own.
  BmcComponentMock b("bmc_test", "bmc_test", mock, dummy_mtd, "", 4, 8);

  EXPECT_CALL(b, update(image.name))
    .Times(1)
    .WillRepeatedly(Invoke(&b, &BmcComponentMock::real_update));

  EXPECT_CALL(b, is_valid(image.name))
    .Times(1)
    .WillOnce(Return(true));

  EXPECT_EQ(0, b.update(image.name));
  // From 1234567890, we cut and throw away the first 4 bytes. So we have 567890.
  // Then we copy 8-4=4 bytes from mtd (abcd) replacing our first 8-4=4 bytes.
  // Hence we should expect the MTD to contain abcd90.
  EXPECT_EQ("abcd90", mtd_dev.read());
}
