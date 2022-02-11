/*
 * Copyright 2014-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <string>
#include <system_error>
#include <stdexcept>
#include <memory>
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <gio/gio.h>
#include <nlohmann/json.hpp>
#include <object-tree/Attribute.h>
#include "../SensorDevice.h"
#include "../SensorObject.h"
#include "../SensorApi.h"
#include "../SensorSysfsApi.h"
using namespace openbmc::qin;

TEST(SensorDeviceObjectTest, Constructor) {
  std::string fsPath = "./test.txt";
  std::unique_ptr<SensorSysfsApi> uSysfsApi(new SensorSysfsApi(fsPath));
  EXPECT_STREQ(uSysfsApi.get()->getFsPath().c_str(), fsPath.c_str());
  SensorDevice parent("Card", std::move(uSysfsApi));
  EXPECT_STREQ(parent.getName().c_str(), "Card");

  uSysfsApi = std::unique_ptr<SensorSysfsApi>(new SensorSysfsApi(fsPath));
  SensorDevice sDevice("sensor1", std::move(uSysfsApi), &parent);
  EXPECT_TRUE(sDevice.getParent() == &parent);
  EXPECT_TRUE(parent.getChildObject("sensor1") == &sDevice);

  SensorObject sobj("sensor2", &parent);
  EXPECT_TRUE(sobj.getParent() == &parent);
  EXPECT_TRUE(parent.getChildObject("sensor2") == &sobj);
}

class ReadWriteTest : public ::testing::Test {
  protected:
    virtual void SetUp() {
      std::string fsPath = "/sys/class/hwmon/hwmon1";
      std::unique_ptr<SensorSysfsApi> uSysfsApi(new SensorSysfsApi(fsPath));
      sDevice_ = new SensorDevice("sensor1", std::move(uSysfsApi));
      sObject_ = new SensorObject("temp", sDevice_);
      sObject_->addAttribute("1_input"); // modes is by default RO
      SensorAttribute* attrRW = sObject_->addAttribute("1_max");
      attrRW->setModes(Attribute::RW);
    }

    virtual void TearDown() {
      delete sObject_;
      delete sDevice_;
    }

    SensorDevice* sDevice_;
    SensorObject* sObject_;
};

TEST_F(ReadWriteTest, AttributeRead) {
  std::string value;
  ASSERT_NO_THROW(value = sObject_->readAttrValue("1_input"));
  EXPECT_STREQ(value.c_str(), sObject_->getAttribute("1_input")
                                      ->getValue().c_str());

  // set addr in SensorAttribute
  SensorAttribute* attr = sObject_->getAttribute("1_input");
  ASSERT_TRUE(attr != nullptr);
  EXPECT_FALSE(attr->isAccessible());
  attr->setAddr("temp1_input");
  EXPECT_TRUE(attr->isAccessible());
  EXPECT_STREQ(attr->getAddr().c_str(), "temp1_input");
}

TEST_F(ReadWriteTest, AttributeWrite) {
  // cannot write with modes RO
  EXPECT_THROW(sObject_->writeAttrValue("8000", "1_input"), std::system_error);
  std::string origVal;
  const std::string newVal = "81000";
  ASSERT_NO_THROW(origVal = sObject_->readAttrValue("1_max"));
  ASSERT_NO_THROW(sObject_->writeAttrValue(newVal, "1_max"));
  EXPECT_STREQ(sObject_->readAttrValue("1_max").c_str(), newVal.c_str());
  ASSERT_NO_THROW(sObject_->writeAttrValue(origVal, "1_max"));
  EXPECT_STREQ(sObject_->readAttrValue("1_max").c_str(), origVal.c_str());
}

TEST_F(ReadWriteTest, Dump) {
  nlohmann::json deviceDump = sDevice_->dumpToJson();
  nlohmann::json objectDump = sObject_->dumpToJson();
  std::string deviceType;
  std::string objectType;

  deviceType = deviceDump["objectType"];
  EXPECT_STREQ(deviceType.c_str(), "SensorDevice");
  objectType = objectDump["objectType"];
  EXPECT_STREQ(objectType.c_str(), "SensorObject");

  EXPECT_TRUE(objectDump.find("access") == objectDump.end());
  ASSERT_TRUE(deviceDump.find("access") != deviceDump.end());
  const std::string &api = deviceDump["access"]["api"];
  EXPECT_STREQ(api.c_str(), "sysfs");
}

int main (int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  ::google::InitGoogleLogging(argv[0]);

  return RUN_ALL_TESTS();
}
