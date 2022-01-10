#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <fstream>
#include "rackmon.hpp"

using namespace std;
using namespace testing;
using nlohmann::json;

class FakeModbus : public Modbus {
  uint8_t exp_addr;
  uint8_t min_addr;
  uint8_t max_addr;
  uint32_t baud;
  bool probed = false;
  Encoder encoder{};

 public:
  FakeModbus(uint8_t e, uint8_t mina, uint8_t maxa, uint32_t b)
      : Modbus(std::cout),
        exp_addr(e),
        min_addr(mina),
        max_addr(maxa),
        baud(b) {}
  void command(
      Msg& req,
      Msg& resp,
      uint32_t b,
      ModbusTime /* unused */,
      ModbusTime /* unused */) {
    encoder.encode(req);
    ASSERT_GE(req.addr, min_addr);
    ASSERT_LE(req.addr, max_addr);
    ASSERT_EQ(b, baud);
    // We are mocking a system with only one available
    // address, exp_addr. So, throw an exception for others.
    if (req.addr != exp_addr)
      throw TimeoutException();
    // There is really no reason at this point for rackmon
    // to be sending any message other than read-holding-regs
    // TODO When adding support for baudrate negotitation etc
    // we might need to make this more flexible.
    ASSERT_EQ(req.raw[1], 0x3);
    ASSERT_EQ(req.len, 8);
    ASSERT_EQ(req.raw[2], 0);
    ASSERT_EQ(req.raw[4], 0);
    if (probed) {
      Msg expMsg = 0x000300000008_M;
      expMsg.addr = exp_addr;
      Encoder::finalize(expMsg);
      ASSERT_EQ(req, expMsg);
      resp = 0x0003106162636465666768696a6b6c6d6e6f70_M;
    } else {
      Msg expMsg = 0x000300680001_M;
      expMsg.addr = exp_addr;
      Encoder::finalize(expMsg);
      ASSERT_EQ(req, expMsg);
      // Allow to be probed only once.
      probed = true;
      resp = 0x0003020000_M;
    }
    resp.addr = exp_addr;
    Encoder::finalize(resp);
    Encoder::decode(resp);
  }
};

class Mock3Modbus : public Modbus {
 public:
  Mock3Modbus(uint8_t e, uint8_t mina, uint8_t maxa, uint32_t b)
      : Modbus(std::cout), fake_(e, mina, maxa, b) {
    ON_CALL(*this, command(_, _, _, _, _))
        .WillByDefault(Invoke([this](
                                  Msg& req,
                                  Msg& resp,
                                  uint32_t b,
                                  ModbusTime timeout,
                                  ModbusTime sleep_time) {
          return fake_.command(req, resp, b, timeout, sleep_time);
        }));
  }
  MOCK_METHOD1(initialize, void(const nlohmann::json& j));
  MOCK_METHOD5(command, void(Msg&, Msg&, uint32_t, ModbusTime, ModbusTime));

  FakeModbus fake_;
};

class MockRackmon : public Rackmon {
 public:
  MockRackmon() : Rackmon() {}
  MOCK_METHOD0(make_interface, std::unique_ptr<Modbus>());
};

class RackmonTest : public ::testing::Test {
 public:
  const std::string r_test_dir = "./test_rackmon.d";
  const std::string r_conf = "./test_rackmon.conf";
  const std::string r_test_map = r_test_dir + "/test1.json";

 public:
  void SetUp() override {
    mkdir(r_test_dir.c_str(), 0755);
    std::string json1 = R"({
        "name": "orv2_psu",
        "address_range": [160, 162],
        "probe_register": 104,
        "default_baudrate": 19200,
        "preferred_baudrate": 19200,
        "registers": [
          {
            "begin": 0,
            "length": 8,
            "format": "string",
            "name": "MFG_MODEL"
          }
        ]
      })";
    std::ofstream ofs1(r_test_map);
    ofs1 << json1;
    ofs1.close();
    std::string rconf_s = R"({
      "interfaces": [
        {
          "device_path": "/tmp/blah",
          "baudrate": 19200
        }
      ]
    })";
    std::ofstream ofs2(r_conf);
    ofs2 << rconf_s;
    ofs2.close();
  }
  void TearDown() override {
    remove(r_test_map.c_str());
    remove(r_test_dir.c_str());
    remove(r_conf.c_str());
  }

 public:
  std::unique_ptr<Modbus> make_modbus(uint8_t exp_addr, int num_cmd_calls) {
    json exp = R"({
      "device_path": "/tmp/blah",
      "baudrate": 19200
    })"_json;
    std::unique_ptr<Mock3Modbus> ptr =
        std::make_unique<Mock3Modbus>(exp_addr, 160, 162, 19200);
    EXPECT_CALL(*ptr, initialize(exp)).Times(1);
    if (num_cmd_calls > 0) {
      EXPECT_CALL(*ptr, command(_, _, _, _, _)).Times(AtLeast(num_cmd_calls));
    }
    std::unique_ptr<Modbus> ptr2 = std::move(ptr);
    return ptr2;
  }
};

TEST_F(RackmonTest, BasicLoad) {
  MockRackmon mon;
  EXPECT_CALL(mon, make_interface())
      .Times(1)
      .WillOnce(Return(ByMove(make_modbus(0, 0))));
  mon.load(r_conf, r_test_dir);
}

TEST_F(RackmonTest, BasicScanFoundNone) {
  MockRackmon mon;
  // Mock a modbus with no active devices,
  // we expect rackmon to scan all of them on
  // start up.
  EXPECT_CALL(mon, make_interface())
      .Times(1)
      .WillOnce(Return(ByMove(make_modbus(0, 3))));
  mon.load(r_conf, r_test_dir);
  mon.start();
  std::vector<ModbusDeviceStatus> devs = mon.list_devices();
  ASSERT_EQ(devs.size(), 0);
  mon.stop();
  Msg req, resp;
  req.raw[0] = 100; // Some unknown address, this should throw
  req.raw[1] = 0x3;
  req.len = 2;
  EXPECT_THROW(mon.rawCmd(req, resp, 1s), std::out_of_range);
}

TEST_F(RackmonTest, BasicScanFoundOne) {
  MockRackmon mon;
  // Mock a modbus with no active devices,
  // we expect rackmon to scan all of them on
  // start up.
  EXPECT_CALL(mon, make_interface())
      .Times(1)
      .WillOnce(Return(ByMove(make_modbus(161, 4))));
  mon.load(r_conf, r_test_dir);
  mon.start();
  std::this_thread::sleep_for(1s);
  std::vector<ModbusDeviceStatus> devs = mon.list_devices();
  ASSERT_EQ(devs.size(), 1);
  ASSERT_EQ(devs[0].addr, 161);
  ASSERT_EQ(devs[0].get_mode(), ModbusDeviceMode::ACTIVE);
  mon.stop();

  Msg rreq, rresp;
  rreq.raw[0] = 100; // Some unknown address, this should throw
  rreq.raw[1] = 0x3;
  rreq.len = 2;
  std::vector<uint16_t> read_regs;
  EXPECT_THROW(mon.rawCmd(rreq, rresp, 1s), std::out_of_range);

  // Only difference of implementations between Rackmon and ModbusDevice
  // is rackmon checks validity of address. Check that we are throwing
  // correctly. The actual functionality is tested in modbus_device_test.cpp.
  EXPECT_THROW(
      mon.ReadHoldingRegisters(100, 0x123, read_regs), std::out_of_range);
  EXPECT_THROW(mon.WriteSingleRegister(100, 0x123, 0x1234), std::out_of_range);
  EXPECT_THROW(
      mon.WriteMultipleRegisters(100, 0x123, read_regs), std::out_of_range);
  std::vector<FileRecord> records(1);
  records[0].data.resize(2);
  EXPECT_THROW(mon.ReadFileRecord(100, records), std::out_of_range);

  // Use a known handled response.
  ReadHoldingRegistersReq req(161, 0, 8);
  std::vector<uint16_t> regs(8);
  ReadHoldingRegistersResp resp(161, regs);
  mon.rawCmd(req, resp, 1s);

  ASSERT_EQ(regs[0], 'a' << 8 | 'b');
}

TEST_F(RackmonTest, BasicScanFoundOneMon) {
  MockRackmon mon;
  // Mock a modbus with no active devices,
  // we expect rackmon to scan all of them on
  // start up.
  EXPECT_CALL(mon, make_interface())
      .Times(1)
      .WillOnce(Return(ByMove(make_modbus(161, 4))));
  mon.load(r_conf, r_test_dir);
  mon.start(1s);
  std::this_thread::sleep_for(1s);
  std::vector<ModbusDeviceStatus> devs = mon.list_devices();
  ASSERT_EQ(devs.size(), 1);
  ASSERT_EQ(devs[0].addr, 161);
  ASSERT_EQ(devs[0].get_mode(), ModbusDeviceMode::ACTIVE);
  std::this_thread::sleep_for(1s);
  mon.stop();
  std::vector<ModbusDeviceValueData> data;
  mon.get_value_data(data);
  ASSERT_EQ(data.size(), 1);
  ASSERT_EQ(data[0].type, "orv2_psu");
  ASSERT_EQ(data[0].register_list.size(), 1);
  ASSERT_EQ(data[0].register_list[0].reg_addr, 0);
  ASSERT_EQ(data[0].register_list[0].name, "MFG_MODEL");
  ASSERT_EQ(data[0].register_list[0].history.size(), 1);
  ASSERT_EQ(
      data[0].register_list[0].history[0].type, RegisterValueType::STRING);
  ASSERT_EQ(
      data[0].register_list[0].history[0].value.strValue, "abcdefghijklmnop");
  ASSERT_NEAR(data[0].register_list[0].history[0].timestamp, std::time(0), 10);
}
