#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>
#include "modbus_device.hpp"

using namespace std;
using namespace testing;
using nlohmann::json;

// Mocks the Modbus interface.
class Mock2Modbus : public Modbus {
 public:
  Mock2Modbus() : Modbus(std::cout) {}
  ~Mock2Modbus() {}
  MOCK_METHOD1(initialize, void(const nlohmann::json&));
  MOCK_METHOD5(command, void(Msg&, Msg&, uint32_t, modbus_time, modbus_time));
};

// Matches Msg with an expected value.
MATCHER_P(encodeMsgContentEqual, msg_exp, "") {
  Encoder::encode(arg);
  return arg == msg_exp;
}

// Sets an argument of Msg type with the provided literal Msg assumed
// to be already encoded.
ACTION_TEMPLATE(
    SetMsgDecode,
    HAS_1_TEMPLATE_PARAMS(unsigned, uIndex),
    AND_1_VALUE_PARAMS(msg)) {
  std::get<uIndex>(args) = msg;
  Encoder::decode(std::get<uIndex>(args));
}

// Our Test class, sets up the register map and a common device store.
class ModbusDeviceTest : public ::testing::Test {
 protected:
  Mock2Modbus modbus_device;
  RegisterMap regmap;
  std::string regmap_s = R"({
    "name": "orv3_psu",
    "address_range": [110, 140],
    "probe_register": 104,
    "default_baudrate": 19200,
    "preferred_baudrate": 19200,
    "registers": [
      {
        "begin": 0,
        "length": 2,
        "keep": 2,
        "format": "string",
        "name": "MFG_MODEL"
      }
    ]
  })";
  void SetUp() override {
    regmap = nlohmann::json::parse(regmap_s);
  }
  Mock2Modbus& get_modbus() {
    return modbus_device;
  }
  RegisterMap& get_regmap() {
    return regmap;
  }
};

// Basic initialization with sane init values as expected
// from the register map and input parameters.
TEST_F(ModbusDeviceTest, BasicSetup) {
  ModbusDevice dev(get_modbus(), 0x32, get_regmap());
  ASSERT_TRUE(dev.is_active());
  ModbusDeviceStatus status = dev.get_status();
  ASSERT_EQ(status.addr, 0x32);
  ASSERT_EQ(status.baudrate, 19200);
  ASSERT_EQ(status.crc_failures, 0);
  ASSERT_EQ(status.timeouts, 0);
  ASSERT_EQ(status.misc_failures, 0);
  ASSERT_EQ(status.num_consecutive_failures, 0);
}

// Basic command interface is a blind pass through.
TEST_F(ModbusDeviceTest, BasicCommand) {
  EXPECT_CALL(
      get_modbus(),
      command(
          Eq(0x3202_M),
          _,
          19200,
          modbus_time::zero(),
          modbus_time::zero()))
      .Times(1)
      .WillOnce(SetArgReferee<1>(0x32020304_M));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap());

  Msg req, resp;
  req.raw = {0x32, 2};
  req.len = 2;

  dev.command(req, resp);
  ASSERT_EQ(resp, 0x32020304_M);
}

TEST_F(ModbusDeviceTest, CommandTimeout) {
  EXPECT_CALL(get_modbus(), command(_, _, _, _, _))
      .Times(1)
      .WillOnce(Throw(TimeoutException()));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap());

  Msg req, resp;
  EXPECT_THROW(dev.command(req, resp), TimeoutException);
  ModbusDeviceStatus status = dev.get_status();
  ASSERT_EQ(status.timeouts, 1);
}

TEST_F(ModbusDeviceTest, CommandCRC) {
  EXPECT_CALL(get_modbus(), command(_, _, _, _, _))
      .Times(1)
      .WillOnce(Throw(CRCError(1, 2)));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap());

  Msg req, resp;
  EXPECT_THROW(dev.command(req, resp), CRCError);
  ModbusDeviceStatus status = dev.get_status();
  ASSERT_EQ(status.crc_failures, 1);
}

TEST_F(ModbusDeviceTest, CommandMisc) {
  EXPECT_CALL(get_modbus(), command(_, _, _, _, _))
      .Times(1)
      .WillOnce(Throw(std::runtime_error("")));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap());

  Msg req, resp;
  EXPECT_THROW(dev.command(req, resp), std::runtime_error);
  ModbusDeviceStatus status = dev.get_status();
  ASSERT_EQ(status.misc_failures, 1);
}

TEST_F(ModbusDeviceTest, MakeDormant) {
  EXPECT_CALL(get_modbus(), command(_, _, _, _, _))
      .Times(10)
      .WillRepeatedly(Throw(TimeoutException()));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap());

  for (int i = 0; i < 10; i++) {
    ModbusDeviceStatus status = dev.get_status();
    ASSERT_EQ(status.get_mode(), ModbusDeviceMode::ACTIVE);
    Msg req, resp;
    EXPECT_THROW(dev.command(req, resp), TimeoutException);
  }

  ModbusDeviceStatus status = dev.get_status();
  ASSERT_EQ(status.timeouts, 10);
  ASSERT_EQ(status.get_mode(), ModbusDeviceMode::DORMANT);
}

TEST_F(ModbusDeviceTest, ReadHoldingRegs) {
  EXPECT_CALL(
      get_modbus(),
      command(
          // addr(1) = 0x32,
          // func(1) = 0x03,
          // reg_off(2) = 0x0064,
          // reg_cnt(2) = 0x0002
          encodeMsgContentEqual(0x320300640002_EM),
          _,
          19200,
          modbus_time::zero(),
          modbus_time::zero()))
      .Times(1)
      // addr(1) = 9x32
      // func(1) = 03
      // bytes(1) = 04
      // data(4) = 11223344
      .WillOnce(SetMsgDecode<1>(0x32030411223344_EM));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap());

  std::vector<uint16_t> regs(2), exp_regs{0x1122, 0x3344};
  dev.ReadHoldingRegisters(0x64, regs);
  ASSERT_EQ(regs, exp_regs);
}

TEST_F(ModbusDeviceTest, WriteSingleReg) {
  EXPECT_CALL(
      get_modbus(),
      command(
          // addr(1) = 0x32,
          // func(1) = 0x6,
          // reg_off(2) = 0x0064,
          // reg_val(2) = 0x1122
          encodeMsgContentEqual(0x320600641122_EM),
          _,
          19200,
          modbus_time::zero(),
          modbus_time::zero()))
      .Times(1)
      // addr(1) = 0x32,
      // func(1) = 0x06,
      // reg_off(2) = 0x0064,
      // reg_val(2) = 0x1122
      .WillOnce(SetMsgDecode<1>(0x320600641122_EM));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap());

  std::vector<uint16_t> regs(2), exp_regs{0x1122, 0x3344};
  dev.WriteSingleRegister(0x64, 0x1122);
}

TEST_F(ModbusDeviceTest, WriteMultipleReg) {
  EXPECT_CALL(
      get_modbus(),
      command(
          // addr(1) = 0x32,
          // func(1) = 0x10,
          // reg_off(2) = 0x0064,
          // reg_cnt(2) = 0x0002,
          // bytes(1) = 0x04,
          // regs(2*2) = 0x1122 3344
          encodeMsgContentEqual(0x3210006400020411223344_EM),
          _,
          19200,
          modbus_time::zero(),
          modbus_time::zero()))
      .Times(1)
      // addr(1) = 0x32,
      // func(1) = 0x10,
      // reg_off(2) = 0x0064,
      // reg_cnt(2) = 0x0002
      .WillOnce(SetMsgDecode<1>(0x321000640002_EM));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap());

  std::vector<uint16_t> regs{0x1122, 0x3344};
  dev.WriteMultipleRegisters(0x64, regs);
}

TEST_F(ModbusDeviceTest, ReadFileRecord) {
  // Request and response are copied from
  // Page 33, (Adds addr to the head)
  // https://modbus.org/docs/Modbus_Application_Protocol_V1_1b.pdf
  EXPECT_CALL(
      get_modbus(),
      command(
          encodeMsgContentEqual(0x32140E0600040001000206000300090002_EM),
          _,
          19200,
          modbus_time::zero(),
          modbus_time::zero()))
      .Times(1)
      .WillOnce(SetMsgDecode<1>(0x32140C05060DFE0020050633CD0040_EM));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap());

  std::vector<FileRecord> records(2);
  records[0].data.resize(2);
  records[0].file_num = 4;
  records[0].record_num = 1;
  records[1].data.resize(2);
  records[1].file_num = 3;
  records[1].record_num = 9;
  dev.ReadFileRecord(records);
  ASSERT_EQ(records[0].data[0], 0x0DFE);
  ASSERT_EQ(records[0].data[1], 0x20);
  ASSERT_EQ(records[1].data[0], 0x33CD);
  ASSERT_EQ(records[1].data[1], 0x0040);
}

TEST_F(ModbusDeviceTest, DeviceStatus) {
  ModbusDevice dev(get_modbus(), 0x32, get_regmap());
  ModbusDeviceStatus status = dev.get_status();
  nlohmann::json j = status;
  ASSERT_EQ(status.addr, 0x32);
  ASSERT_EQ(status.baudrate, 19200);
  ASSERT_EQ(status.crc_failures, 0);
  ASSERT_EQ(status.misc_failures, 0);
  ASSERT_EQ(status.timeouts, 0);
  ASSERT_EQ(status.get_mode(), ModbusDeviceMode::ACTIVE);
  ASSERT_EQ(j["addr"], 0x32);
  ASSERT_EQ(j["crc_fails"], 0);
  ASSERT_EQ(j["misc_fails"], 0);
  ASSERT_EQ(j["timeouts"], 0);
  ASSERT_EQ(j["mode"], "active");
  ASSERT_EQ(j["baudrate"], 19200);
}

TEST_F(ModbusDeviceTest, MonitorDataValue) {
  EXPECT_CALL(
      get_modbus(),
      command(
          // addr(1) = 0x32,
          // func(1) = 0x03,
          // reg_off(2) = 0x0000,
          // reg_cnt(2) = 0x0002
          encodeMsgContentEqual(0x320300000002_EM),
          _,
          19200,
          modbus_time::zero(),
          modbus_time::zero()))
      .Times(3)
      // addr(1) = 0x32,
      // func(1) = 0x03,
      // bytes(1) = 0x04,
      // data(4) = 61626364, 62636465, 63646566
      .WillOnce(SetMsgDecode<1>(0x32030461626364_EM))
      .WillOnce(SetMsgDecode<1>(0x32030462636465_EM))
      .WillOnce(SetMsgDecode<1>(0x32030463646566_EM));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap());

  dev.monitor();
  ModbusDeviceValueData data = dev.get_value_data();
  ASSERT_EQ(data.addr, 0x32);
  ASSERT_EQ(data.baudrate, 19200);
  ASSERT_EQ(data.crc_failures, 0);
  ASSERT_EQ(data.timeouts, 0);
  ASSERT_EQ(data.misc_failures, 0);
  ASSERT_NEAR(data.last_active, std::time(0), 10);
  ASSERT_EQ(data.num_consecutive_failures, 0);
  ASSERT_EQ(data.get_mode(), ModbusDeviceMode::ACTIVE);
  ASSERT_EQ(data.register_list.size(), 1);
  ASSERT_EQ(data.register_list[0].reg_addr, 0);
  ASSERT_EQ(data.register_list[0].name, "MFG_MODEL");
  ASSERT_EQ(data.register_list[0].history.size(), 1);
  ASSERT_NEAR(data.register_list[0].history[0].timestamp, std::time(0), 10);
  ASSERT_EQ(data.register_list[0].history[0].type, RegisterValueType::STRING);
  ASSERT_EQ(data.register_list[0].history[0].value.strValue, "abcd");

  dev.monitor();
  ModbusDeviceValueData data2 = dev.get_value_data();
  ASSERT_EQ(data2.addr, 0x32);
  ASSERT_EQ(data2.baudrate, 19200);
  ASSERT_EQ(data2.crc_failures, 0);
  ASSERT_EQ(data2.timeouts, 0);
  ASSERT_EQ(data2.misc_failures, 0);
  ASSERT_NEAR(data2.last_active, std::time(0), 10);
  ASSERT_EQ(data2.num_consecutive_failures, 0);
  ASSERT_EQ(data2.get_mode(), ModbusDeviceMode::ACTIVE);
  ASSERT_EQ(data2.register_list.size(), 1);
  ASSERT_EQ(data2.register_list[0].reg_addr, 0);
  ASSERT_EQ(data2.register_list[0].name, "MFG_MODEL");
  ASSERT_EQ(data2.register_list[0].history.size(), 2);
  ASSERT_EQ(data2.register_list[0].history[0].type, RegisterValueType::STRING);
  ASSERT_EQ(data2.register_list[0].history[0].value.strValue, "abcd");
  ASSERT_EQ(data2.register_list[0].history[1].type, RegisterValueType::STRING);
  ASSERT_EQ(data2.register_list[0].history[1].value.strValue, "bcde");
  ASSERT_NEAR(data2.register_list[0].history[0].timestamp, std::time(0), 10);
  ASSERT_NEAR(data2.register_list[0].history[1].timestamp, std::time(0), 10);
  ASSERT_GE(
      data2.register_list[0].history[1].timestamp,
      data2.register_list[0].history[0].timestamp);

  dev.monitor();
  ModbusDeviceValueData data3 = dev.get_value_data();
  ASSERT_EQ(data3.register_list[0].history.size(), 2);
  // TODO We probably need a circular iterator on the history.
  // Till then, we will probably get out of order stuff.
  ASSERT_EQ(data3.register_list[0].history[1].value.strValue, "bcde");
  ASSERT_EQ(data3.register_list[0].history[0].value.strValue, "cdef");
  nlohmann::json j = data3;
  ASSERT_EQ(j["addr"], 0x32);
  ASSERT_EQ(j["crc_fails"], 0);
  ASSERT_EQ(j["timeouts"], 0);
  ASSERT_EQ(j["misc_fails"], 0);
  ASSERT_EQ(j["mode"], "active");
  ASSERT_NEAR(j["now"], std::time(0), 10);
  ASSERT_TRUE(j["ranges"].is_array() && j["ranges"].size() == 1);
  ASSERT_EQ(j["ranges"][0]["begin"], 0);
  ASSERT_EQ(j["ranges"][0]["name"], "MFG_MODEL");
  ASSERT_TRUE(
      j["ranges"][0]["readings"].is_array() &&
      j["ranges"][0]["readings"].size() == 2);
  ASSERT_NEAR(j["ranges"][0]["readings"][0]["time"], std::time(0), 10);
  ASSERT_EQ(j["ranges"][0]["readings"][0]["value"], "cdef");
  ASSERT_EQ(j["ranges"][0]["readings"][0]["type"], "string");
  ASSERT_NEAR(j["ranges"][0]["readings"][1]["time"], std::time(0), 10);
  ASSERT_EQ(j["ranges"][0]["readings"][1]["value"], "bcde");
  ASSERT_EQ(j["ranges"][0]["readings"][1]["type"], "string");
}

TEST_F(ModbusDeviceTest, MonitorRawData) {
  EXPECT_CALL(
      get_modbus(),
      command(
          // addr(1) = 0x32,
          // func(1) = 0x03,
          // reg_off(2) = 0x0000,
          // reg_cnt(2) = 0x0002
          encodeMsgContentEqual(0x320300000002_EM),
          _,
          19200,
          modbus_time::zero(),
          modbus_time::zero()))
      .Times(3)
      // addr(1) = 0x32,
      // func(1) = 0x03,
      // bytes(1) = 0x04,
      // data(4) = 61626364, 62636465, 63646566
      .WillOnce(SetMsgDecode<1>(0x32030461626364_EM))
      .WillOnce(SetMsgDecode<1>(0x32030462636465_EM))
      .WillOnce(SetMsgDecode<1>(0x32030463646566_EM));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap());

  dev.monitor();
  nlohmann::json data = dev.get_raw_data();
  ASSERT_EQ(data["addr"], 0x32);
  ASSERT_EQ(data["crc_fails"], 0);
  ASSERT_EQ(data["timeouts"], 0);
  ASSERT_EQ(data["misc_fails"], 0);
  ASSERT_EQ(data["mode"], "active");
  ASSERT_NEAR(data["now"], std::time(0), 10);
  ASSERT_TRUE(data["ranges"].is_array() && data["ranges"].size() == 1);
  ASSERT_EQ(data["ranges"][0]["begin"], 0);
  ASSERT_TRUE(
      data["ranges"][0]["readings"].is_array() &&
      data["ranges"][0]["readings"].size() == 2);
  ASSERT_NEAR(data["ranges"][0]["readings"][0]["time"], std::time(0), 10);
  ASSERT_EQ(data["ranges"][0]["readings"][0]["data"], "61626364");
  ASSERT_EQ(data["ranges"][0]["readings"][1]["time"], 0);
  ASSERT_EQ(data["ranges"][0]["readings"][1]["data"], "00000000");

  dev.monitor();
  nlohmann::json data2 = dev.get_raw_data();
  ASSERT_EQ(data2["addr"], 0x32);
  ASSERT_EQ(data2["crc_fails"], 0);
  ASSERT_EQ(data2["timeouts"], 0);
  ASSERT_EQ(data2["misc_fails"], 0);
  ASSERT_EQ(data2["mode"], "active");
  ASSERT_NEAR(data2["now"], std::time(0), 10);
  ASSERT_TRUE(data2["ranges"].is_array() && data2["ranges"].size() == 1);
  ASSERT_EQ(data2["ranges"][0]["begin"], 0);
  ASSERT_TRUE(
      data2["ranges"][0]["readings"].is_array() &&
      data2["ranges"][0]["readings"].size() == 2);
  ASSERT_NEAR(data2["ranges"][0]["readings"][0]["time"], std::time(0), 10);
  ASSERT_EQ(data2["ranges"][0]["readings"][0]["data"], "61626364");
  ASSERT_NEAR(data2["ranges"][0]["readings"][1]["time"], std::time(0), 10);
  ASSERT_EQ(data2["ranges"][0]["readings"][1]["data"], "62636465");

  dev.monitor();
  nlohmann::json data3 = dev.get_raw_data();
  ASSERT_TRUE(
      data3["ranges"][0]["readings"].is_array() &&
      data3["ranges"][0]["readings"].size() == 2);
  ASSERT_NEAR(data3["ranges"][0]["readings"][0]["time"], std::time(0), 10);
  ASSERT_EQ(data3["ranges"][0]["readings"][0]["data"], "63646566");
  ASSERT_NEAR(data3["ranges"][0]["readings"][1]["time"], std::time(0), 10);
  ASSERT_EQ(data3["ranges"][0]["readings"][1]["data"], "62636465");
}

// TODO Test Formatted data. Potentially, we could have
// CLI display value data itself and we can completely
// remove formatDataw.
TEST_F(ModbusDeviceTest, MonitorFmtData) {
  EXPECT_CALL(
      get_modbus(),
      command(
          // addr(1) = 0x32,
          // func(1) = 0x03,
          // reg_off(2) = 0x0000,
          // reg_cnt(2) = 0x0002
          encodeMsgContentEqual(0x320300000002_EM),
          _,
          19200,
          modbus_time::zero(),
          modbus_time::zero()))
      .Times(3)
      // addr(1) = 0x32,
      // func(1) = 0x03,
      // bytes(1) = 0x04,
      // data(4) = 61626364, 62636465, 63646566
      .WillOnce(SetMsgDecode<1>(0x32030461626364_EM))
      .WillOnce(SetMsgDecode<1>(0x32030462636465_EM))
      .WillOnce(SetMsgDecode<1>(0x32030463646566_EM));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap());

  dev.monitor();
  dev.monitor();
  dev.monitor();
  nlohmann::json data = dev.get_fmt_data();
  ASSERT_EQ(data["addr"], 0x32);
  ASSERT_EQ(data["crc_fails"], 0);
  ASSERT_EQ(data["timeouts"], 0);
  ASSERT_EQ(data["misc_fails"], 0);
  ASSERT_EQ(data["mode"], "active");
  ASSERT_EQ(data["type"], "orv3_psu");
  ASSERT_NEAR(data["now"], std::time(0), 10);
  ASSERT_TRUE(data["ranges"].is_array() && data["ranges"].size() == 1);
  std::string exp1_out =
      R"(  <0x0000> MFG_MODEL                        : cdef bcde)";
  std::string actual = data["ranges"][0];
  ASSERT_EQ(actual, exp1_out);
}

class MockModbusDevice : public ModbusDevice {
 public:
  MockModbusDevice(Modbus& m, uint8_t addr, const RegisterMap& rmap)
      : ModbusDevice(m, addr, rmap) {}
  MOCK_METHOD4(command, void(Msg&, Msg&, modbus_time, modbus_time));
};

TEST(ModbusSpecialHandler, BasicHandlingStringValuePeriodic) {
  Modbus mock_modbus{std::cout};
  RegisterMap mock_rmap = R"({
    "name": "orv3_psu",
    "address_range": [110, 140],
    "probe_register": 104,
    "default_baudrate": 19200,
    "preferred_baudrate": 19200,
    "registers": [
      {
        "begin": 0,
        "length": 2,
        "name": "MFG_MODEL"
      }
    ]
  })"_json;
  MockModbusDevice dev(mock_modbus, 0x32, mock_rmap);

  EXPECT_CALL(
      dev,
      command(
          // addr(1) = 0x32,
          // func(1) = 0x10,
          // reg_off(2) = 0x000a (10),
          // reg_cnt(2) = 0x0002
          // bytes(1) = 0x04,
          // data(2*2) = 0x3031 0x3233
          encodeMsgContentEqual(0x3210000a00020430313233_EM),
          _,
          _,
          _))
      .Times(Between(2,3));
  ModbusSpecialHandler special;
  SpecialHandlerInfo& info = special;
  info = R"({
    "reg": 10,
    "len": 2,
    "period": 1,
    "action": "write",
    "info": {
      "interpret": "string",
      "value": "0123"
    }
  })"_json;

  special.handle(dev);
  std::this_thread::sleep_for(250ms);
  special.handle(
      dev); // Since the period is 1, this should technically do nothing.
  std::this_thread::sleep_for(250ms);
  special.handle(
      dev); // This should do nothing as well, we are less than 1 sec.
  std::this_thread::sleep_for(1500ms);
  special.handle(dev); // This should call! we are 1.25s out from first handle.
}

TEST(ModbusSpecialHandler, BasicHandlingIntegerOneShot) {
  Modbus mock_modbus{std::cout};
  RegisterMap mock_rmap = R"({
    "name": "orv3_psu",
    "address_range": [110, 140],
    "probe_register": 104,
    "default_baudrate": 19200,
    "preferred_baudrate": 19200,
    "registers": [
      {
        "begin": 0,
        "length": 2,
        "name": "MFG_MODEL"
      }
    ]
  })"_json;
  MockModbusDevice dev(mock_modbus, 0x32, mock_rmap);

  EXPECT_CALL(
      dev,
      command(
          // addr(1) = 0x32,
          // func(1) = 0x10,
          // reg_off(2) = 0x000a (10),
          // reg_cnt(2) = 0x0002
          // bytes(1) = 0x04,
          // data(2*2) = 0x00bc 0x614e (hex for int 12345678)
          encodeMsgContentEqual(0x3210000a00020400bc614e_EM),
          _,
          _,
          _))
      .Times(1);
  ModbusSpecialHandler special;
  SpecialHandlerInfo& info = special;
  info = R"({
    "reg": 10,
    "len": 2,
    "period": -1,
    "action": "write",
    "info": {
      "interpret": "integer",
      "shell": "echo 12345678"
    }
  })"_json;
  // 12345678 == 0x00bc614e

  special.handle(dev);
  std::this_thread::sleep_for(250ms);
  special.handle(dev); // Do the same as above, but the call should happen only
                       // once since period = -1
  std::this_thread::sleep_for(250ms);
  special.handle(dev);
  std::this_thread::sleep_for(1s);
  special.handle(dev);
}
