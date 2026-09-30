// Copyright 2021-present Facebook. All Rights Reserved.
#include "ModbusDevice.h"
#include <thread>
#include "TestUtils.h"

using namespace std;
using namespace testing;
using nlohmann::json;
using namespace rackmon;

// Mocks the Modbus interface.
class Mock2Modbus : public Modbus {
 public:
  explicit Mock2Modbus(std::optional<uint8_t> p) : Modbus() {
    port_ = p;
  }
  ~Mock2Modbus() = default;
  MOCK_METHOD1(initialize, void(const nlohmann::json&));
  MOCK_METHOD5(command, void(Msg&, Msg&, uint32_t, ModbusTime, Parity));
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
  Mock2Modbus modbus_device{std::nullopt};
  RegisterMap regmap;
  std::string regmap_s = R"({
    "name": "orv3_psu",
    "address_range": [[110, 140]],
    "probe": [{"register": 104}],
    "baudrate": 19200,
    "registers": [
      {
        "begin": 0,
        "length": 2,
        "keep": 2,
        "format": "STRING",
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
  EXPECT_TRUE(dev.isActive());
  ModbusDeviceInfo status = dev.getInfo();
  EXPECT_EQ(status.deviceAddress, 0x32);
  EXPECT_EQ(status.baudrate, 19200);
  EXPECT_EQ(status.crcErrors, 0);
  EXPECT_EQ(status.timeouts, 0);
  EXPECT_EQ(status.miscErrors, 0);
  EXPECT_EQ(status.numConsecutiveFailures, 0);
}

TEST_F(ModbusDeviceTest, BasicSetupWithPort) {
  Mock2Modbus modbus(123);
  EXPECT_EQ(modbus.getPort(), 123);
  ModbusDevice dev(modbus, 0x32, get_regmap());
  EXPECT_TRUE(dev.isActive());
  ModbusDeviceInfo status = dev.getInfo();
  EXPECT_EQ(status.deviceAddress, 0x32);
  EXPECT_EQ(status.port, 123);
  EXPECT_EQ(status.baudrate, 19200);
  EXPECT_EQ(status.crcErrors, 0);
  EXPECT_EQ(status.timeouts, 0);
  EXPECT_EQ(status.miscErrors, 0);
  EXPECT_EQ(status.numConsecutiveFailures, 0);
}

// Basic command interface is a blind pass through.
TEST_F(ModbusDeviceTest, BasicCommand) {
  EXPECT_CALL(
      get_modbus(), command(Eq(0x3202_M), _, 19200, ModbusTime::zero(), _))
      .Times(1)
      .WillOnce(SetArgReferee<1>(0x32020304_M));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap());

  Msg req, resp;
  req.raw = {0x32, 2};
  req.len = 2;

  dev.command(req, resp);
  EXPECT_EQ(resp, 0x32020304_M);
}

TEST_F(ModbusDeviceTest, CommandTimeout) {
  EXPECT_CALL(get_modbus(), command(_, _, _, _, _))
      .Times(3)
      .WillRepeatedly(Throw(TimeoutException()));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap(), 3);

  Msg req, resp;
  EXPECT_THROW(dev.command(req, resp), TimeoutException);
  ModbusDeviceInfo status = dev.getInfo();
  EXPECT_EQ(status.timeouts, 3);
}

TEST_F(ModbusDeviceTest, CommandCRC) {
  EXPECT_CALL(get_modbus(), command(_, _, _, _, _))
      .Times(5)
      .WillRepeatedly(Throw(CRCError(1, 2)));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap());

  Msg req, resp;
  EXPECT_THROW(dev.command(req, resp), CRCError);
  ModbusDeviceInfo status = dev.getInfo();
  EXPECT_EQ(status.crcErrors, 5);
}

TEST_F(ModbusDeviceTest, CommandMisc) {
  EXPECT_CALL(get_modbus(), command(_, _, _, _, _))
      .Times(1)
      .WillOnce(Throw(std::runtime_error("")));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap(), 1);

  Msg req, resp;
  EXPECT_THROW(dev.command(req, resp), std::runtime_error);
  ModbusDeviceInfo status = dev.getInfo();
  EXPECT_EQ(status.miscErrors, 1);
}

TEST_F(ModbusDeviceTest, CommandFlaky) {
  EXPECT_CALL(get_modbus(), command(_, _, _, _, _))
      .Times(2)
      .WillOnce(Invoke([](Msg& req, Msg&, uint32_t, ModbusTime, Parity) {
        EXPECT_EQ(req, 0x3202_M);
        Encoder::encode(req);
        throw TimeoutException();
      }))
      .WillOnce(Invoke([](Msg& req, Msg& resp, uint32_t, ModbusTime, Parity) {
        EXPECT_EQ(req, 0x3202_M);
        Encoder::encode(req);
        resp = 0x32020304_EM;
        Encoder::decode(resp);
      }));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap(), 3);

  Msg req, resp;
  req.raw = {0x32, 2};
  req.len = 2;
  dev.command(req, resp);
  EXPECT_EQ(resp, 0x32020304_M);
  ModbusDeviceInfo status = dev.getInfo();
  EXPECT_EQ(status.timeouts, 1);
}

TEST_F(ModbusDeviceTest, TimeoutInExclusiveMode) {
  EXPECT_CALL(get_modbus(), command(_, _, _, _, _))
      .Times(1)
      .WillOnce(Throw(TimeoutException()));
  ModbusDevice dev(get_modbus(), 0x32, get_regmap(), 3);
  dev.setExclusiveMode(true);
  Msg req, resp;
  req.raw = {0x32, 2};
  req.len = 2;
  EXPECT_THROW(dev.command(req, resp), TimeoutException);
  ModbusDeviceInfo status = dev.getInfo();
  EXPECT_EQ(status.timeouts, 1);
}

TEST_F(ModbusDeviceTest, MakeDormant) {
  EXPECT_CALL(get_modbus(), command(_, _, _, _, _))
      .Times(10)
      .WillRepeatedly(Throw(TimeoutException()));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap(), 1);

  for (int i = 0; i < 10; i++) {
    ModbusDeviceInfo status = dev.getInfo();
    EXPECT_EQ(status.mode, ModbusDeviceMode::ACTIVE);
    Msg req, resp;
    EXPECT_THROW(dev.command(req, resp), TimeoutException);
  }

  ModbusDeviceInfo status = dev.getInfo();
  EXPECT_EQ(status.timeouts, 10);
  EXPECT_EQ(status.mode, ModbusDeviceMode::DORMANT);
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
          ModbusTime::zero(),
          _))
      .Times(1)
      // addr(1) = 9x32
      // func(1) = 03
      // bytes(1) = 04
      // data(4) = 11223344
      .WillOnce(SetMsgDecode<1>(0x32030411223344_EM));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap());

  std::vector<uint16_t> regs(2), exp_regs{0x1122, 0x3344};
  dev.readHoldingRegisters(0x64, regs);
  EXPECT_EQ(regs, exp_regs);
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
          ModbusTime::zero(),
          _))
      .Times(1)
      // addr(1) = 0x32,
      // func(1) = 0x06,
      // reg_off(2) = 0x0064,
      // reg_val(2) = 0x1122
      .WillOnce(SetMsgDecode<1>(0x320600641122_EM));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap());

  std::vector<uint16_t> regs(2), exp_regs{0x1122, 0x3344};
  dev.writeSingleRegister(0x64, 0x1122);
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
          ModbusTime::zero(),
          _))
      .Times(1)
      // addr(1) = 0x32,
      // func(1) = 0x10,
      // reg_off(2) = 0x0064,
      // reg_cnt(2) = 0x0002
      .WillOnce(SetMsgDecode<1>(0x321000640002_EM));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap());

  std::vector<uint16_t> regs{0x1122, 0x3344};
  dev.writeMultipleRegisters(0x64, regs);
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
          ModbusTime::zero(),
          _))
      .Times(1)
      .WillOnce(SetMsgDecode<1>(0x32140C05060DFE0020050633CD0040_EM));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap());

  std::vector<FileRecord> records(2);
  records[0].data.resize(2);
  records[0].fileNum = 4;
  records[0].recordNum = 1;
  records[1].data.resize(2);
  records[1].fileNum = 3;
  records[1].recordNum = 9;
  dev.readFileRecord(records);
  EXPECT_EQ(records[0].data[0], 0x0DFE);
  EXPECT_EQ(records[0].data[1], 0x20);
  EXPECT_EQ(records[1].data[0], 0x33CD);
  EXPECT_EQ(records[1].data[1], 0x0040);
}

TEST_F(ModbusDeviceTest, DeviceStatus) {
  ModbusDevice dev(get_modbus(), 0x32, get_regmap());
  ModbusDeviceInfo status = dev.getInfo();
  nlohmann::json j = status;
  EXPECT_EQ(status.deviceAddress, 0x32);
  EXPECT_EQ(status.baudrate, 19200);
  EXPECT_EQ(status.crcErrors, 0);
  EXPECT_EQ(status.miscErrors, 0);
  EXPECT_EQ(status.timeouts, 0);
  EXPECT_EQ(status.mode, ModbusDeviceMode::ACTIVE);
  EXPECT_EQ(status.parity, Parity::EVEN);
  EXPECT_EQ(j["devAddress"], 0x32);
  EXPECT_EQ(j["crcErrors"], 0);
  EXPECT_EQ(j["miscErrors"], 0);
  EXPECT_EQ(j["timeouts"], 0);
  EXPECT_EQ(j["mode"], "ACTIVE");
  EXPECT_EQ(j["baudrate"], 19200);
  // Spelt out, not the enum's underlying integer: modbus-update.py hands
  // this straight to the direct backend, which only knows the names.
  EXPECT_EQ(j["parity"], "EVEN");
}

TEST_F(ModbusDeviceTest, MonitorInvalidRegOnce) {
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
          ModbusTime::zero(),
          _))
      .Times(1)
      // addr(1) = 0x32,
      // func(1) = 0x83,
      // data(1) = 0x02
      .WillOnce(SetMsgDecode<1>(0x328302_EM));

  ModbusDevice dev(get_modbus(), 0x32, get_regmap(), 1);
  // This should see the illegal address error
  dev.reloadAllRegisters();
  // This should be a no-op.
  dev.reloadAllRegisters();
}

class ModbusDeviceMockTime : public ModbusDevice {
  time_t currTime_ = 0;

 public:
  ModbusDeviceMockTime(
      Modbus& interface,
      uint8_t deviceAddress,
      const RegisterMap& registerMap,
      time_t baseTime,
      int numCommandRetries = 5)
      : ModbusDevice(interface, deviceAddress, registerMap, numCommandRetries),
        currTime_(baseTime) {}
  void incTime(time_t byTime) {
    currTime_ += byTime;
  }
  time_t getCurrentTime() override {
    return currTime_;
  }
};

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
          ModbusTime::zero(),
          _))
      .Times(3)
      // addr(1) = 0x32,
      // func(1) = 0x03,
      // bytes(1) = 0x04,
      // data(4) = 61626364, 62636465, 63646566
      .WillOnce(SetMsgDecode<1>(0x32030461626364_EM))
      .WillOnce(SetMsgDecode<1>(0x32030462636465_EM))
      .WillOnce(SetMsgDecode<1>(0x32030463646566_EM));

  time_t baseTime = std::time(nullptr);
  constexpr time_t monInterval = RegisterDescriptor::kDefaultInterval;
  ModbusDeviceMockTime dev(get_modbus(), 0x32, get_regmap(), baseTime);

  ModbusDeviceValueData emptyLatestData = dev.getValueData({}, true);
  EXPECT_EQ(emptyLatestData.deviceAddress, 0x32);
  EXPECT_EQ(emptyLatestData.registerList.size(), 1);
  EXPECT_EQ(emptyLatestData.registerList[0].regAddr, 0);
  EXPECT_EQ(emptyLatestData.registerList[0].name, "MFG_MODEL");
  EXPECT_EQ(emptyLatestData.registerList[0].history.size(), 0);

  dev.reloadAllRegisters();
  ModbusDeviceValueData data = dev.getValueData();
  EXPECT_EQ(data.deviceAddress, 0x32);
  EXPECT_EQ(data.baudrate, 19200);
  EXPECT_EQ(data.crcErrors, 0);
  EXPECT_EQ(data.timeouts, 0);
  EXPECT_EQ(data.miscErrors, 0);
  EXPECT_EQ(data.lastActive, baseTime);
  EXPECT_EQ(data.numConsecutiveFailures, 0);
  EXPECT_EQ(data.mode, ModbusDeviceMode::ACTIVE);
  EXPECT_EQ(data.registerList.size(), 1);
  EXPECT_EQ(data.registerList[0].regAddr, 0);
  EXPECT_EQ(data.registerList[0].name, "MFG_MODEL");
  EXPECT_EQ(data.registerList[0].history.size(), 1);
  EXPECT_EQ(data.registerList[0].history[0].timestamp, baseTime);
  EXPECT_EQ(data.registerList[0].history[0].type, RegisterValueType::STRING);
  EXPECT_EQ(
      std::get<std::string>(data.registerList[0].history[0].value), "abcd");

  ModbusRegisterFilter filter1, filter2, filter3, filter4;
  filter1.addrFilter = {0x0};
  filter2.addrFilter = {0x10};
  filter3.nameFilter = {"MFG_MODEL"};
  filter4.nameFilter = {"HELLOWORLD"};
  ModbusDeviceValueData filterData1 = dev.getValueData(filter1);
  EXPECT_EQ(filterData1.deviceAddress, 0x32);
  EXPECT_EQ(filterData1.registerList.size(), 1);
  EXPECT_EQ(filterData1.registerList[0].regAddr, 0);
  EXPECT_EQ(filterData1.registerList[0].history.size(), 1);

  ModbusDeviceValueData filterData2 = dev.getValueData(filter2);
  EXPECT_EQ(filterData2.deviceAddress, 0x32);
  EXPECT_EQ(filterData2.registerList.size(), 0);

  ModbusDeviceValueData filterData3 = dev.getValueData(filter3);
  EXPECT_EQ(filterData3.deviceAddress, 0x32);
  EXPECT_EQ(filterData3.registerList.size(), 1);
  EXPECT_EQ(filterData3.registerList[0].regAddr, 0);
  EXPECT_EQ(filterData3.registerList[0].history.size(), 1);

  ModbusDeviceValueData filterData4 = dev.getValueData(filter4);
  EXPECT_EQ(filterData4.deviceAddress, 0x32);
  EXPECT_EQ(filterData4.registerList.size(), 0);

  dev.incTime(monInterval);
  dev.reloadAllRegisters();
  ModbusDeviceValueData data2 = dev.getValueData();
  EXPECT_EQ(data2.deviceAddress, 0x32);
  EXPECT_EQ(data2.baudrate, 19200);
  EXPECT_EQ(data2.crcErrors, 0);
  EXPECT_EQ(data2.timeouts, 0);
  EXPECT_EQ(data2.miscErrors, 0);
  EXPECT_EQ(data2.lastActive, baseTime + monInterval);
  EXPECT_EQ(data2.numConsecutiveFailures, 0);
  EXPECT_EQ(data2.mode, ModbusDeviceMode::ACTIVE);
  EXPECT_EQ(data2.registerList.size(), 1);
  EXPECT_EQ(data2.registerList[0].regAddr, 0);
  EXPECT_EQ(data2.registerList[0].name, "MFG_MODEL");
  EXPECT_EQ(data2.registerList[0].history.size(), 2);
  EXPECT_EQ(data2.registerList[0].history[0].type, RegisterValueType::STRING);
  EXPECT_EQ(
      std::get<std::string>(data2.registerList[0].history[0].value), "abcd");
  EXPECT_EQ(data2.registerList[0].history[1].type, RegisterValueType::STRING);
  EXPECT_EQ(
      std::get<std::string>(data2.registerList[0].history[1].value), "bcde");
  EXPECT_EQ(data2.registerList[0].history[0].timestamp, baseTime);
  EXPECT_EQ(data2.registerList[0].history[1].timestamp, baseTime + monInterval);
  EXPECT_GE(
      data2.registerList[0].history[1].timestamp,
      data2.registerList[0].history[0].timestamp);

  dev.incTime(monInterval);
  dev.reloadAllRegisters();
  ModbusDeviceValueData data3 = dev.getValueData();
  EXPECT_EQ(data3.registerList[0].history.size(), 2);
  // TODO We probably need a circular iterator on the history.
  // Till then, we will probably get out of order stuff.
  EXPECT_EQ(
      std::get<std::string>(data3.registerList[0].history[1].value), "bcde");
  EXPECT_EQ(
      std::get<std::string>(data3.registerList[0].history[0].value), "cdef");
  nlohmann::json j = data3;
  EXPECT_EQ(j["devInfo"]["devAddress"], 0x32);
  EXPECT_EQ(j["devInfo"]["crcErrors"], 0);
  EXPECT_EQ(j["devInfo"]["timeouts"], 0);
  EXPECT_EQ(j["devInfo"]["miscErrors"], 0);
  EXPECT_EQ(j["devInfo"]["mode"], "ACTIVE");
  EXPECT_TRUE(j["regList"].is_array() && j["regList"].size() == 1);
  EXPECT_EQ(j["regList"][0]["regAddress"], 0);
  EXPECT_EQ(j["regList"][0]["name"], "MFG_MODEL");
  EXPECT_TRUE(
      j["regList"][0]["history"].is_array() &&
      j["regList"][0]["history"].size() == 2);
  EXPECT_EQ(
      j["regList"][0]["history"][0]["timestamp"], baseTime + (monInterval * 2));
  EXPECT_EQ(j["regList"][0]["history"][0]["value"]["strValue"], "cdef");
  EXPECT_EQ(j["regList"][0]["history"][0]["type"], "STRING");
  EXPECT_EQ(
      j["regList"][0]["history"][1]["timestamp"], baseTime + (monInterval * 1));
  EXPECT_EQ(j["regList"][0]["history"][1]["value"]["strValue"], "bcde");
  EXPECT_EQ(j["regList"][0]["history"][1]["type"], "STRING");

  ModbusDeviceValueData data4 = dev.getValueData({}, true);
  EXPECT_EQ(data4.registerList[0].history.size(), 1);
  EXPECT_EQ(
      std::get<std::string>(data4.registerList[0].history[0].value), "cdef");
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
          ModbusTime::zero(),
          _))
      .Times(3)
      // addr(1) = 0x32,
      // func(1) = 0x03,
      // bytes(1) = 0x04,
      // data(4) = 61626364, 62636465, 63646566
      .WillOnce(SetMsgDecode<1>(0x32030461626364_EM))
      .WillOnce(SetMsgDecode<1>(0x32030462636465_EM))
      .WillOnce(SetMsgDecode<1>(0x32030463646566_EM));

  constexpr time_t monInterval = RegisterDescriptor::kDefaultInterval;
  time_t baseTime = std::time(nullptr);
  ModbusDeviceMockTime dev(get_modbus(), 0x32, get_regmap(), baseTime);

  dev.reloadAllRegisters();
  nlohmann::json data = dev.getRawData();
  EXPECT_EQ(data["addr"], 0x32);
  EXPECT_EQ(data["crc_fails"], 0);
  EXPECT_EQ(data["timeouts"], 0);
  EXPECT_EQ(data["misc_fails"], 0);
  EXPECT_EQ(data["mode"], "ACTIVE");
  EXPECT_NEAR(data["now"], baseTime, 10);
  EXPECT_TRUE(data["ranges"].is_array() && data["ranges"].size() == 1);
  EXPECT_EQ(data["ranges"][0]["begin"], 0);
  EXPECT_TRUE(
      data["ranges"][0]["readings"].is_array() &&
      data["ranges"][0]["readings"].size() == 1);
  EXPECT_NEAR(data["ranges"][0]["readings"][0]["time"], baseTime, 10);
  EXPECT_EQ(data["ranges"][0]["readings"][0]["data"], "61626364");

  dev.incTime(monInterval);
  dev.reloadAllRegisters();
  nlohmann::json data2 = dev.getRawData();
  EXPECT_EQ(data2["addr"], 0x32);
  EXPECT_EQ(data2["crc_fails"], 0);
  EXPECT_EQ(data2["timeouts"], 0);
  EXPECT_EQ(data2["misc_fails"], 0);
  EXPECT_EQ(data2["mode"], "ACTIVE");
  EXPECT_NEAR(data2["now"], baseTime, 10);
  EXPECT_TRUE(data2["ranges"].is_array() && data2["ranges"].size() == 1);
  EXPECT_EQ(data2["ranges"][0]["begin"], 0);
  EXPECT_TRUE(
      data2["ranges"][0]["readings"].is_array() &&
      data2["ranges"][0]["readings"].size() == 2);
  EXPECT_NEAR(data2["ranges"][0]["readings"][0]["time"], baseTime, 10);
  EXPECT_EQ(data2["ranges"][0]["readings"][0]["data"], "61626364");
  EXPECT_NEAR(
      data2["ranges"][0]["readings"][1]["time"], baseTime + monInterval, 10);
  EXPECT_EQ(data2["ranges"][0]["readings"][1]["data"], "62636465");

  // Dont change time, just a single reload should not reload since
  // time has not changed.
  dev.reloadAllRegisters();
  data2 = dev.getRawData();
  EXPECT_TRUE(data2["ranges"].is_array() && data2["ranges"].size() == 1);
  EXPECT_EQ(data2["ranges"][0]["begin"], 0);
  EXPECT_TRUE(
      data2["ranges"][0]["readings"].is_array() &&
      data2["ranges"][0]["readings"].size() == 2);
  EXPECT_NEAR(data2["ranges"][0]["readings"][0]["time"], baseTime, 10);
  EXPECT_EQ(data2["ranges"][0]["readings"][0]["data"], "61626364");
  EXPECT_NEAR(
      data2["ranges"][0]["readings"][1]["time"], baseTime + monInterval, 10);
  EXPECT_EQ(data2["ranges"][0]["readings"][1]["data"], "62636465");

  // Enter and exit exclusive mode. This should
  // force the next reload to happen even if our time has not incremented.
  dev.setExclusiveMode(true);
  dev.setExclusiveMode(false);
  dev.reloadAllRegisters();
  nlohmann::json data3 = dev.getRawData();
  EXPECT_TRUE(
      data3["ranges"][0]["readings"].is_array() &&
      data3["ranges"][0]["readings"].size() == 2);
  EXPECT_NEAR(
      data3["ranges"][0]["readings"][0]["time"],
      baseTime + (monInterval * 1),
      10);
  EXPECT_EQ(data3["ranges"][0]["readings"][0]["data"], "63646566");
  EXPECT_NEAR(
      data3["ranges"][0]["readings"][1]["time"],
      baseTime + (monInterval * 1),
      10);
  EXPECT_EQ(data3["ranges"][0]["readings"][1]["data"], "62636465");
}

class MockModbusDevice : public ModbusDevice {
 public:
  MockModbusDevice(Modbus& m, uint8_t addr, const RegisterMap& rmap)
      : ModbusDevice(m, addr, rmap) {}
  MOCK_METHOD3(command, void(Msg&, Msg&, ModbusTime));
};

static nlohmann::json getPlanRegmap() {
  std::string regmap_s = R"({
    "name": "orv3_psu",
    "address_range": [[5, 5]],
    "probe": [{"register": 0}],
    "baudrate": 19200,
    "registers": [
      {
        "begin": 0,
        "length": 2,
        "keep": 2,
        "format": "LONG",
        "name": "THING1"
      },
      {
        "begin": 2,
        "length": 2,
        "keep": 2,
        "format": "LONG",
        "name": "THING2"
      }
    ]
  })";
  return nlohmann::json::parse(regmap_s);
}

TEST_F(ModbusDeviceTest, ReloadPlan) {
  RegisterMap regmap = getPlanRegmap();
  InSequence seq;
  // First reload registers, We read first reg returns 0x12345678
  EXPECT_CALL(
      get_modbus(),
      command(
          // addr(1) = 0x5,
          // func(1) = 0x03,
          // reg_off(2) = 0x0000,
          // reg_cnt(2) = 0x0002
          encodeMsgContentEqual(0x050300000002_EM),
          _,
          19200,
          ModbusTime::zero(),
          _))
      .Times(1)
      .WillOnce(SetMsgDecode<1>(0x05030412345678_EM))
      .RetiresOnSaturation();
  // First (same) reload registers we read the second reg.
  // Returns 0x89abcdef
  EXPECT_CALL(
      get_modbus(),
      command(
          // addr(1) = 0x5,
          // func(1) = 0x03,
          // reg_off(2) = 0x0002,
          // reg_cnt(2) = 0x0002
          encodeMsgContentEqual(0x050300020002_EM),
          _,
          19200,
          ModbusTime::zero(),
          _))
      .Times(1)
      .WillOnce(SetMsgDecode<1>(0x05030489abcdef_EM))
      .RetiresOnSaturation();
  // Now the first two are part of the plan. The second
  // time we call reloadAllRegisters, we expect to do a
  // batched read.
  EXPECT_CALL(
      get_modbus(),
      command(
          // addr(1) = 0x5,
          // func(1) = 0x03,
          // reg_off(2) = 0x0000,
          // reg_cnt(2) = 0x0004
          encodeMsgContentEqual(0x050300000004_EM),
          _,
          19200,
          ModbusTime::zero(),
          _))
      .Times(1)
      .WillOnce(SetMsgDecode<1>(0x050308fedcba9876543210_EM))
      .RetiresOnSaturation();

  time_t baseTime = std::time(nullptr);
  constexpr time_t monInterval = RegisterDescriptor::kDefaultInterval;
  ModbusDeviceMockTime dev(get_modbus(), 0x5, regmap, baseTime);

  // We expect it to reload the registers one by one.
  // This should cover the first two expect-calls.
  dev.reloadAllRegisters();
  {
    auto data = dev.getValueData();
    EXPECT_EQ(data.deviceAddress, 0x5);
    EXPECT_EQ(data.registerList.size(), 2);

    EXPECT_EQ(data.registerList[0].regAddr, 0);
    EXPECT_EQ(data.registerList[0].history.size(), 1);
    EXPECT_EQ(data.registerList[0].history[0].timestamp, baseTime);
    EXPECT_EQ(data.registerList[0].history[0].type, RegisterValueType::LONG);
    EXPECT_EQ(
        std::get<int64_t>(data.registerList[0].history[0].value), 0x12345678);

    EXPECT_EQ(data.registerList[1].regAddr, 2);
    EXPECT_EQ(data.registerList[1].history.size(), 1);
    EXPECT_EQ(data.registerList[1].history[0].timestamp, baseTime);
    EXPECT_EQ(data.registerList[1].history[0].type, RegisterValueType::LONG);
    EXPECT_EQ(
        std::get<int64_t>(data.registerList[1].history[0].value), 0x89abcdef);
  }
  dev.incTime(monInterval);
  // Second reload should exercise the span-read and the final (3d)
  // expect should be satisfied.
  dev.reloadAllRegisters();
  {
    auto data = dev.getValueData();
    EXPECT_EQ(data.deviceAddress, 0x5);
    EXPECT_EQ(data.registerList.size(), 2);

    EXPECT_EQ(data.registerList[0].regAddr, 0);
    EXPECT_EQ(data.registerList[0].history.size(), 2);
    EXPECT_EQ(data.registerList[0].history[0].timestamp, baseTime);
    EXPECT_EQ(data.registerList[0].history[0].type, RegisterValueType::LONG);
    EXPECT_EQ(
        std::get<int64_t>(data.registerList[0].history[0].value), 0x12345678);
    EXPECT_EQ(
        data.registerList[0].history[1].timestamp, baseTime + monInterval);
    EXPECT_EQ(data.registerList[0].history[1].type, RegisterValueType::LONG);
    EXPECT_EQ(
        std::get<int64_t>(data.registerList[0].history[1].value), 0xfedcba98);

    EXPECT_EQ(data.registerList[1].regAddr, 2);
    EXPECT_EQ(data.registerList[1].history.size(), 2);
    EXPECT_EQ(data.registerList[1].history[0].timestamp, baseTime);
    EXPECT_EQ(data.registerList[1].history[0].type, RegisterValueType::LONG);
    EXPECT_EQ(
        std::get<int64_t>(data.registerList[1].history[0].value), 0x89abcdef);
    EXPECT_EQ(
        data.registerList[1].history[1].timestamp, baseTime + monInterval);
    EXPECT_EQ(data.registerList[1].history[1].type, RegisterValueType::LONG);
    EXPECT_EQ(
        std::get<int64_t>(data.registerList[1].history[1].value), 0x76543210);
  }
  {
    ModbusRegisterFilter filter{};
    filter.addrFilter = {0};
    auto data = dev.getValueData(filter);
    EXPECT_EQ(data.deviceAddress, 0x5);
    EXPECT_EQ(data.registerList.size(), 1);
    EXPECT_EQ(data.registerList[0].regAddr, 0);
    EXPECT_EQ(data.registerList[0].history.size(), 2);
    EXPECT_EQ(data.registerList[0].history[0].timestamp, baseTime);
    EXPECT_EQ(data.registerList[0].history[0].type, RegisterValueType::LONG);
    EXPECT_EQ(
        std::get<int64_t>(data.registerList[0].history[0].value), 0x12345678);
    EXPECT_EQ(
        data.registerList[0].history[1].timestamp, baseTime + monInterval);
    EXPECT_EQ(data.registerList[0].history[1].type, RegisterValueType::LONG);
    EXPECT_EQ(
        std::get<int64_t>(data.registerList[0].history[1].value), 0xfedcba98);
  }
  {
    ModbusRegisterFilter filter{};
    filter.nameFilter = {"THING1"};
    auto data = dev.getValueData(filter, true);
    EXPECT_EQ(data.deviceAddress, 0x5);
    EXPECT_EQ(data.registerList.size(), 1);
    EXPECT_EQ(data.registerList[0].regAddr, 0);
    EXPECT_EQ(data.registerList[0].history.size(), 1);
    EXPECT_EQ(
        data.registerList[0].history[0].timestamp, baseTime + monInterval);
    EXPECT_EQ(data.registerList[0].history[0].type, RegisterValueType::LONG);
    EXPECT_EQ(
        std::get<int64_t>(data.registerList[0].history[0].value), 0xfedcba98);
  }
}

TEST_F(ModbusDeviceTest, ForceReloadGetValueData) {
  RegisterMap regmap = getPlanRegmap();
  InSequence seq;
  // First reload registers, We read first reg returns 0x12345678
  EXPECT_CALL(
      get_modbus(),
      command(
          // addr(1) = 0x5,
          // func(1) = 0x03,
          // reg_off(2) = 0x0000,
          // reg_cnt(2) = 0x0002
          encodeMsgContentEqual(0x050300000002_EM),
          _,
          19200,
          ModbusTime::zero(),
          _))
      .Times(1)
      .WillOnce(SetMsgDecode<1>(0x05030412345678_EM))
      .RetiresOnSaturation();
  // First (same) reload registers we read the second reg.
  // Returns 0x89abcdef
  EXPECT_CALL(
      get_modbus(),
      command(
          // addr(1) = 0x5,
          // func(1) = 0x03,
          // reg_off(2) = 0x0002,
          // reg_cnt(2) = 0x0002
          encodeMsgContentEqual(0x050300020002_EM),
          _,
          19200,
          ModbusTime::zero(),
          _))
      .Times(1)
      .WillOnce(SetMsgDecode<1>(0x05030489abcdef_EM))
      .RetiresOnSaturation();
  // Now the first two are part of the plan. The second
  // time we call reloadAllRegisters, we expect to do a
  // batched read.
  EXPECT_CALL(
      get_modbus(),
      command(
          // addr(1) = 0x5,
          // func(1) = 0x03,
          // reg_off(2) = 0x0000,
          // reg_cnt(2) = 0x0004
          encodeMsgContentEqual(0x050300000004_EM),
          _,
          19200,
          ModbusTime::zero(),
          _))
      .Times(1)
      .WillOnce(SetMsgDecode<1>(0x050308fedcba9876543210_EM))
      .RetiresOnSaturation();

  time_t baseTime = std::time(nullptr);
  ModbusDeviceMockTime dev(get_modbus(), 0x5, regmap, baseTime);

  // We expect it to reload the registers one by one.
  // This should cover the first two expect-calls.
  dev.reloadAllRegisters();
  {
    auto data = dev.getValueData();
    EXPECT_EQ(data.deviceAddress, 0x5);
    EXPECT_EQ(data.registerList.size(), 2);

    EXPECT_EQ(data.registerList[0].regAddr, 0);
    EXPECT_EQ(data.registerList[0].history.size(), 1);
    EXPECT_EQ(data.registerList[0].history[0].timestamp, baseTime);
    EXPECT_EQ(data.registerList[0].history[0].type, RegisterValueType::LONG);
    EXPECT_EQ(
        std::get<int64_t>(data.registerList[0].history[0].value), 0x12345678);

    EXPECT_EQ(data.registerList[1].regAddr, 2);
    EXPECT_EQ(data.registerList[1].history.size(), 1);
    EXPECT_EQ(data.registerList[1].history[0].timestamp, baseTime);
    EXPECT_EQ(data.registerList[1].history[0].type, RegisterValueType::LONG);
    EXPECT_EQ(
        std::get<int64_t>(data.registerList[1].history[0].value), 0x89abcdef);
  }
  // We are not ready to reload yet, but we will force reload in our get.
  dev.incTime(1);
  {
    ModbusRegisterFilter filter{};
    filter.addrFilter = {0, 2};
    dev.forceReloadRegisters(filter);
    auto data = dev.getValueData(filter, false);
    EXPECT_EQ(data.deviceAddress, 0x5);
    EXPECT_EQ(data.registerList.size(), 2);

    EXPECT_EQ(data.registerList[0].regAddr, 0);
    EXPECT_EQ(data.registerList[0].history.size(), 2);
    EXPECT_EQ(data.registerList[0].history[0].timestamp, baseTime);
    EXPECT_EQ(data.registerList[0].history[0].type, RegisterValueType::LONG);
    EXPECT_EQ(
        std::get<int64_t>(data.registerList[0].history[0].value), 0x12345678);
    EXPECT_EQ(data.registerList[0].history[1].timestamp, baseTime + 1);
    EXPECT_EQ(data.registerList[0].history[1].type, RegisterValueType::LONG);
    EXPECT_EQ(
        std::get<int64_t>(data.registerList[0].history[1].value), 0xfedcba98);

    EXPECT_EQ(data.registerList[1].regAddr, 2);
    EXPECT_EQ(data.registerList[1].history.size(), 2);
    EXPECT_EQ(data.registerList[1].history[0].timestamp, baseTime);
    EXPECT_EQ(data.registerList[1].history[0].type, RegisterValueType::LONG);
    EXPECT_EQ(
        std::get<int64_t>(data.registerList[1].history[0].value), 0x89abcdef);
    EXPECT_EQ(data.registerList[1].history[1].timestamp, baseTime + 1);
    EXPECT_EQ(data.registerList[1].history[1].type, RegisterValueType::LONG);
    EXPECT_EQ(
        std::get<int64_t>(data.registerList[1].history[1].value), 0x76543210);
  }
}

static nlohmann::json getTimeSyncRegmap() {
  std::string regmap_s = R"({
    "name": "orv3_psu",
    "address_range": [[5, 5]],
    "probe": [{"register": 0}],
    "baudrate": 19200,
    "time_sync": {
      "address": 4,
      "interval": 10
    },
    "max_span_length": 0,
    "registers": [
      {
        "begin": 0,
        "length": 2,
        "keep": 2,
        "format": "LONG",
        "name": "THING1"
      }
    ]
  })";
  return nlohmann::json::parse(regmap_s);
}

TEST_F(ModbusDeviceTest, TimeSync) {
  RegisterMap regmap = getTimeSyncRegmap();
  InSequence seq;
  // First write timestamp.
  EXPECT_CALL(
      get_modbus(),
      command(
          // addr(1) = 0x5,
          // func(1) = 0x10,
          // reg_off(2) = 0x0004,
          // reg_cnt(2) = 0x0002,
          // bytes(1) = 0x0004,
          // upper reg = 0x0000,
          // lower reg = 0x0042
          encodeMsgContentEqual(0x0510000400020400000042_EM),
          _,
          19200,
          ModbusTime::zero(),
          _))
      .Times(1)
      .WillOnce(SetMsgDecode<1>(0x051000040002_EM))
      .RetiresOnSaturation();
  // First reload reads THING1 at offset 0.
  // Returns 0x89abcdef
  EXPECT_CALL(
      get_modbus(),
      command(
          // addr(1) = 0x5,
          // func(1) = 0x03,
          // reg_off(2) = 0x0000,
          // reg_cnt(2) = 0x0002
          encodeMsgContentEqual(0x050300000002_EM),
          _,
          19200,
          ModbusTime::zero(),
          _))
      .Times(1)
      .WillOnce(SetMsgDecode<1>(0x05030489abcdef_EM))
      .RetiresOnSaturation();
  // Second write time, now with the advanced time value 0x52.
  //
  EXPECT_CALL(
      get_modbus(),
      command(
          // addr(1) = 0x5,
          // func(1) = 0x10,
          // reg_off(2) = 0x0004,
          // reg_cnt(2) = 0x0002,
          // bytes(1) = 0x0004,
          // upper reg = 0x0000,
          // lower reg = 0x0052
          encodeMsgContentEqual(0x0510000400020400000052_EM),
          _,
          19200,
          ModbusTime::zero(),
          _))
      .Times(1)
      .WillOnce(SetMsgDecode<1>(0x051000040002_EM))
      .RetiresOnSaturation();

  time_t baseTime = 0x42;
  constexpr time_t timeSyncInterval = 16;
  ModbusDeviceMockTime dev(get_modbus(), 0x5, regmap, baseTime);

  // We expect time to be written with 0x42 and one register to be read.
  dev.reloadAllRegisters();
  dev.incTime(timeSyncInterval);

  // No registers should be read. Only time sync reg written with value 0x52
  dev.reloadAllRegisters();
}
