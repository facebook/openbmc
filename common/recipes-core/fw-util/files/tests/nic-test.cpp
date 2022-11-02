#include "nic.h"
#include "system_mock.h"

using namespace std;
using namespace testing;

class NicMockComponent : public NicComponent {
  public:
    NicMockComponent(std::string fru, std::string comp) :
      NicComponent(fru, comp) {}
    NicMockComponent(std::string fru, std::string comp, std::string ver_key_store) :
      NicComponent(fru, comp, ver_key_store) {}
    MOCK_METHOD2(get_key, int(const std::string& key, std::string& val));
    MOCK_METHOD0(sys, System&());
};


TEST(NicVersionTest, Mellanox)
{
  const std::array<uint8_t, 36> _payload = {
    0xf1, 0xf1, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x6d, 0x6c, 0x78, 0x30, 0x2e, 0x31, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x0e, 0x15, 0x0b, 0xc0,
    0x10, 0x15, 0x15, 0xb3, 0x02, 0x77, 0x15, 0xb3,
    0x00, 0x00, 0x81, 0x19};
  const std::string payload(std::begin(_payload), std::end(_payload));
  stringstream out, err;
  SystemMock smock(out, err);

  NicMockComponent nic("nic", "nic0");
  EXPECT_CALL(nic, sys())
    .WillRepeatedly(ReturnRef(smock));
  EXPECT_CALL(nic, get_key(string("nic_fw_ver"), _))
    .Times(1)
    .WillRepeatedly(DoAll(SetArgReferee<1>(payload), Return(0)));

  json j;
  EXPECT_EQ(nic.get_version(j), 0);
  EXPECT_EQ(j["VERSION"], "14.21.3008");
  EXPECT_EQ(j["PRETTY_COMPONENT"], "Mellanox NIC firmware");
}

TEST(NicVersionTest, Broadcomm)
{
  const std::array<uint8_t, 36> _payload = {
    0xf1, 0xf1, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x32, 0x30, 0x2e, 0x36, 0x2e, 0x34, 0x2e, 0x31,
    0x32, 0x00, 0x00, 0x00, 0x01, 0x05, 0x63, 0x00,
    0x16, 0xc9, 0x14, 0xe4, 0x02, 0x07, 0x14, 0xe4,
    0x00, 0x00, 0x11, 0x3d};
  const std::string payload(std::begin(_payload), std::end(_payload));
  stringstream out, err;
  SystemMock smock(out, err);

  NicMockComponent nic("nic", "nic0");
  EXPECT_CALL(nic, sys())
    .WillRepeatedly(ReturnRef(smock));
  EXPECT_CALL(nic, get_key(string("nic_fw_ver"), _))
    .Times(1)
    .WillRepeatedly(DoAll(SetArgReferee<1>(payload), Return(0)));

  json j;
  EXPECT_EQ(nic.get_version(j), 0);
  EXPECT_EQ(j["VERSION"], "20.6.4.12");
  EXPECT_EQ(j["PRETTY_COMPONENT"], "Broadcom NIC firmware");
}


TEST(NicVersionTest, Unknown)
{
  const std::array<uint8_t, 36> _payload = {
    0xf1, 0xf1, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x6d, 0x6c, 0x78, 0x30, 0x2e, 0x31, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x0e, 0x15, 0x0b, 0xc0,
    0x10, 0x15, 0x15, 0xb3, 0x02, 0x77, 0x15, 0xb3,
    0x00, 0x00, 0x81, 0x20};
  const std::string payload(std::begin(_payload), std::end(_payload));
  stringstream out, err;
  SystemMock smock(out, err);

  NicMockComponent nic("nic", "nic0");
  EXPECT_CALL(nic, sys())
    .WillRepeatedly(ReturnRef(smock));
  EXPECT_CALL(nic, get_key(string("nic_fw_ver"), _))
    .Times(1)
    .WillRepeatedly(DoAll(SetArgReferee<1>(payload), Return(0)));

  json j;
  EXPECT_EQ(nic.get_version(j), 0);
  EXPECT_EQ(j["VERSION"], "NA (Unknown Manufacture ID: 0x20810000)");
  EXPECT_EQ(j["PRETTY_COMPONENT"], "NIC firmware");
}

TEST(NicUpgrade, CallTest)
{
  stringstream out, err;
  SystemMock smock(out, err);
  EXPECT_CALL(smock, runcmd(string("/usr/local/bin/ncsi-util -p \"MY_IMAGE\"")))
    .Times(2)
    .WillOnce(Return(-1))
    .WillOnce(Return(0));
  NicMockComponent nic("nic", "nic0");
  EXPECT_CALL(nic, sys())
    .WillRepeatedly(ReturnRef(smock));

  EXPECT_EQ(nic.update("MY_IMAGE"), -1);
  EXPECT_EQ(nic.update("MY_IMAGE"), 0);
}
