#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>
#include <sstream>
#include "name.hpp"
#include <nlohmann/json.hpp>
#include <openbmc/pal.h>

using namespace std;
using namespace testing;
using json = nlohmann::json;

class MockFruNames : public FruNames {
  public:
    MockFruNames() : FruNames() {}
    MOCK_METHOD1(is_present, bool(uint8_t));
    MOCK_METHOD2(get_capability, int(uint8_t, unsigned int*));
    MOCK_METHOD2(get_fru_name, int(uint8_t, std::string&));
    MOCK_METHOD0(get_fru_count, int());

    // mostly test get_list(fru_type, os)
};

TEST(single_fru, server) {
  MockFruNames mock;
  EXPECT_CALL(mock, is_present(1)).WillRepeatedly(Return(true));
  EXPECT_CALL(mock, get_capability(1, _)).WillRepeatedly(DoAll(SetArgPointee<1>(FRU_CAPABILITY_SERVER), Return(0)));
  EXPECT_CALL(mock, get_fru_name(1, _)).WillRepeatedly(DoAll(SetArgReferee<1>("server"), Return(0)));
  EXPECT_CALL(mock, get_fru_count()).WillRepeatedly(Return(1));
  stringstream ss;
  EXPECT_EQ(mock.get_list("server_fru", ss), 0);
  json j;
  j["version"] = "1.0.0";
  j["server_fru"] = {"server"};
  EXPECT_EQ(ss.str(), j.dump(4) + "\n");

  ss.str("");
  EXPECT_EQ(mock.get_list("all", ss), 0);
  std::vector<std::string> empty;
  j["nic_fru"] = empty;
  j["bmc_fru"] = empty;
  EXPECT_EQ(ss.str(), j.dump(4) + "\n");
}

TEST(single_fru, nic) {
  MockFruNames mock;
  EXPECT_CALL(mock, is_present(1)).WillRepeatedly(Return(true));
  EXPECT_CALL(mock, get_capability(1, _)).WillRepeatedly(DoAll(SetArgPointee<1>(FRU_CAPABILITY_NETWORK_CARD), Return(0)));
  EXPECT_CALL(mock, get_fru_name(1, _)).WillRepeatedly(DoAll(SetArgReferee<1>("nicx"), Return(0)));
  EXPECT_CALL(mock, get_fru_count()).WillRepeatedly(Return(1));
  stringstream ss;
  EXPECT_EQ(mock.get_list("nic_fru", ss), 0);
  json j;
  j["version"] = "1.0.0";
  j["nic_fru"] = {"nicx"};
  EXPECT_EQ(ss.str(), j.dump(4) + "\n");

  ss.str("");
  EXPECT_EQ(mock.get_list("all", ss), 0);
  std::vector<std::string> empty;
  j["server_fru"] = empty;
  j["bmc_fru"] = empty;
  EXPECT_EQ(ss.str(), j.dump(4) + "\n");
}

TEST(single_fru, bmc) {
  MockFruNames mock;
  EXPECT_CALL(mock, is_present(1)).WillRepeatedly(Return(true));
  EXPECT_CALL(mock, get_capability(1, _)).WillRepeatedly(DoAll(SetArgPointee<1>(FRU_CAPABILITY_MANAGEMENT_CONTROLLER), Return(0)));
  EXPECT_CALL(mock, get_fru_name(1, _)).WillRepeatedly(DoAll(SetArgReferee<1>("spb"), Return(0)));
  EXPECT_CALL(mock, get_fru_count()).WillRepeatedly(Return(1));
  stringstream ss;
  EXPECT_EQ(mock.get_list("bmc_fru", ss), 0);
  json j;
  j["version"] = "1.0.0";
  j["bmc_fru"] = {"spb"};
  EXPECT_EQ(ss.str(), j.dump(4) + "\n");

  ss.str("");
  EXPECT_EQ(mock.get_list("all", ss), 0);
  std::vector<std::string> empty;
  j["server_fru"] = empty;
  j["nic_fru"] = empty;
  EXPECT_EQ(ss.str(), j.dump(4) + "\n");
}

TEST(single_fru, multi) {
  MockFruNames mock;
  EXPECT_CALL(mock, is_present(1)).WillRepeatedly(Return(true));
  EXPECT_CALL(mock, is_present(2)).WillRepeatedly(Return(true));
  EXPECT_CALL(mock, is_present(3)).WillRepeatedly(Return(true));
  EXPECT_CALL(mock, get_capability(1, _)).WillRepeatedly(DoAll(SetArgPointee<1>(FRU_CAPABILITY_SERVER), Return(0)));
  EXPECT_CALL(mock, get_capability(2, _)).WillRepeatedly(DoAll(SetArgPointee<1>(FRU_CAPABILITY_MANAGEMENT_CONTROLLER), Return(0)));
  EXPECT_CALL(mock, get_capability(3, _)).WillRepeatedly(DoAll(SetArgPointee<1>(FRU_CAPABILITY_NETWORK_CARD), Return(0)));
  EXPECT_CALL(mock, get_fru_name(1, _)).WillRepeatedly(DoAll(SetArgReferee<1>("mb"), Return(0)));
  EXPECT_CALL(mock, get_fru_name(2, _)).WillRepeatedly(DoAll(SetArgReferee<1>("bmc"), Return(0)));
  EXPECT_CALL(mock, get_fru_name(3, _)).WillRepeatedly(DoAll(SetArgReferee<1>("nic0"), Return(0)));
  EXPECT_CALL(mock, get_fru_count()).WillRepeatedly(Return(3));
  stringstream ss;
  EXPECT_EQ(mock.get_list("all", ss), 0);
  json j;
  j["version"] = "1.0.0";
  j["bmc_fru"] = {"bmc"};
  j["server_fru"] = {"mb"};
  j["nic_fru"] = {"nic0"};
  EXPECT_EQ(ss.str(), j.dump(4) + "\n");
}

