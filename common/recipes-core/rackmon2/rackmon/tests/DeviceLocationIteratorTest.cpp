// Copyright 2021-present Facebook. All Rights Reserved.
#include "TestUtils.h"

#if (defined(__llvm__) && (__clang_major__ < 9)) || \
    (!defined(__llvm__) && (__GNUC__ < 8))
#include <experimental/filesystem>
namespace std {
namespace filesystem = experimental::filesystem;
}
#else
#include <filesystem>
#endif
#include "DeviceLocationIterator.h"
#include "Register.h"

using namespace std;
using namespace testing;
using namespace rackmon;
using nlohmann::json;

TEST(DeviceLocationIteratorTest, Basic) {
  std::string json1 = R"(
  {
    "name": "orv2_psu",
    "address_range": [[160, 162], [10, 10]],
    "probe": [{"register": 104}],
    "baudrate": 19200,
    "registers": [
      {
        "begin": 0,
        "length": 8,
        "format": "STRING",
        "name": "MFG_MODEL",
        "interval": 40
      }
    ]
  })";

  RegisterMapDatabase db;
  db.load(nlohmann::json::parse(json1));

  json exp = R"({
    "device_path": "/tmp/blah",
    "baudrate": 19200,
    "port": 123
  })"_json;

  std::shared_ptr<Modbus> interface = std::make_shared<Modbus>();
  interface->initialize(exp);
  EXPECT_EQ(interface.get()->getPort().value(), 123);

  {
    DeviceLocationIterator iterator(db, interface);
    EXPECT_EQ((*iterator).addr, 160);
    EXPECT_EQ((*iterator).interface.getPort(), optional<uint8_t>(123));
    ++iterator;

    EXPECT_EQ((*iterator).addr, 161);
    EXPECT_EQ((*iterator).interface.getPort(), optional<uint8_t>(123));
    ++iterator;

    EXPECT_EQ((*iterator).addr, 162);
    EXPECT_EQ((*iterator).interface.getPort(), optional<uint8_t>(123));
    ++iterator;

    EXPECT_EQ((*iterator).addr, 10);
    EXPECT_EQ((*iterator).interface.getPort(), optional<uint8_t>(123));
    ++iterator;

    EXPECT_THROW(*iterator, out_of_range);
  }

  json exp_no_port = R"({
        "device_path": "/tmp/blah",
        "baudrate": 19200
      })"_json;
  interface = std::make_shared<Modbus>();
  interface->initialize(exp_no_port);
  EXPECT_FALSE(interface.get()->getPort().has_value());

  {
    DeviceLocationIterator iterator(db, interface);

    EXPECT_EQ((*iterator).addr, 160);
    EXPECT_EQ((*iterator).interface.getPort(), std::nullopt);
    ++iterator;

    EXPECT_EQ((*iterator).addr, 161);
    EXPECT_EQ((*iterator).interface.getPort(), std::nullopt);
    ++iterator;

    EXPECT_EQ((*iterator).addr, 162);
    EXPECT_EQ((*iterator).interface.getPort(), std::nullopt);
    ++iterator;

    EXPECT_EQ((*iterator).addr, 10);
    EXPECT_EQ((*iterator).interface.getPort(), std::nullopt);
    ++iterator;

    EXPECT_THROW(*iterator, out_of_range);
  }

  std::string json2 = R"(
        {
          "name": "orv2_psu",
          "address_range": [[1, 2], [3, 4]],
          "probe": [{"register": 104}],
          "baudrate": 19200,
          "registers": [
            {
              "begin": 0,
              "length": 8,
              "format": "STRING",
              "name": "MFG_MODEL",
              "interval": 40
            }
          ]
        })";
  db.load(nlohmann::json::parse(json2));

  {
    DeviceLocationIterator iterator(db, interface);

    EXPECT_EQ((*iterator).addr, 160);
    EXPECT_EQ((*iterator).interface.getPort(), std::nullopt);
    ++iterator;

    EXPECT_EQ((*iterator).addr, 161);
    EXPECT_EQ((*iterator).interface.getPort(), std::nullopt);
    ++iterator;

    EXPECT_EQ((*iterator).addr, 162);
    EXPECT_EQ((*iterator).interface.getPort(), std::nullopt);
    ++iterator;

    EXPECT_EQ((*iterator).addr, 10);
    EXPECT_EQ((*iterator).interface.getPort(), std::nullopt);
    ++iterator;

    EXPECT_EQ((*iterator).addr, 1);
    EXPECT_EQ((*iterator).interface.getPort(), std::nullopt);
    ++iterator;

    EXPECT_EQ((*iterator).addr, 2);
    EXPECT_EQ((*iterator).interface.getPort(), std::nullopt);
    ++iterator;

    EXPECT_EQ((*iterator).addr, 3);
    EXPECT_EQ((*iterator).interface.getPort(), std::nullopt);
    ++iterator;

    EXPECT_EQ((*iterator).addr, 4);
    EXPECT_EQ((*iterator).interface.getPort(), std::nullopt);
    ++iterator;

    EXPECT_THROW(*iterator, out_of_range);
    EXPECT_EQ(iterator, iterator.end());
    EXPECT_THROW(++iterator, out_of_range);
  }
}
