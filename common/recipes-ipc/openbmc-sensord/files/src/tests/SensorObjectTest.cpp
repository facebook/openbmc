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
#include "../SensorObject.h"
#include "../SensorApi.h"
#include "../SensorSysfsApi.h"
using namespace openbmc::ipc;

TEST(SensorObjectTest, Constructor) {
  std::string fsPath = "./test.txt";
  std::unique_ptr<SensorSysfsApi> uSysfsApi(new SensorSysfsApi(fsPath));
  EXPECT_STREQ(uSysfsApi.get()->getFsPath().c_str(), fsPath.c_str());
  SensorObject parent("Card", std::move(uSysfsApi));
  EXPECT_STREQ(parent.getName().c_str(), "Card");
  uSysfsApi = std::unique_ptr<SensorSysfsApi>(new SensorSysfsApi(fsPath));
  SensorObject sobj("sensor1", std::move(uSysfsApi), &parent);
  EXPECT_TRUE(sobj.getParent() == &parent);
  EXPECT_TRUE(parent.getChildObject("sensor1") == &sobj);
}

class ReadWriteTest : public ::testing::Test {
  protected:
    virtual void SetUp() {
      std::string fsPath = "/sys/class/hwmon/hwmon1";
      std::unique_ptr<SensorSysfsApi> uSysfsApi(new SensorSysfsApi(fsPath));
      sobj_ = new SensorObject("sensor1", std::move(uSysfsApi));
      sobj_->addAttribute("1_input"); // modes is by default RO
      SensorAttribute* attrRW = sobj_->addAttribute("1_max");
      attrRW->setModes(Attribute::RW);
    }

    virtual void TearDown() {
      delete sobj_;
    }

    SensorObject* sobj_;
};

TEST_F(ReadWriteTest, AttributeRead) {
  std::string value;
  ASSERT_NO_THROW(value = sobj_->readAttrValue("1_input"));
  EXPECT_STREQ(value.c_str(), sobj_->getAttribute("1_input")
                                   ->getValue().c_str());

  // set addr in SensorAttribute
  SensorAttribute* attr = sobj_->getAttribute("1_input");
  ASSERT_TRUE(attr != nullptr);
  EXPECT_FALSE(attr->isAccessible());
  attr->setAddr("temp1_input");
  EXPECT_TRUE(attr->isAccessible());
  EXPECT_STREQ(attr->getAddr().c_str(), "temp1_input");
}

TEST_F(ReadWriteTest, AttributeWrite) {
  // cannot write with modes RO
  EXPECT_THROW(sobj_->writeAttrValue("8000", "1_input"), std::system_error);
  std::string origVal;
  const std::string newVal = "81000";
  ASSERT_NO_THROW(origVal = sobj_->readAttrValue("1_max"));
  ASSERT_NO_THROW(sobj_->writeAttrValue(newVal, "1_max"));
  EXPECT_STREQ(sobj_->readAttrValue("1_max").c_str(), newVal.c_str());
  ASSERT_NO_THROW(sobj_->writeAttrValue(origVal, "1_max"));
  EXPECT_STREQ(sobj_->readAttrValue("1_max").c_str(), origVal.c_str());
}

TEST_F(ReadWriteTest, Dump) {
  Object* obj = sobj_;
  nlohmann::json objectDump = obj->dumpToJson();
  const std::string &objectType = objectDump["objectType"];
  EXPECT_STREQ(objectType.c_str(), "Sensor");

  ASSERT_TRUE(objectDump.find("access") != objectDump.end());
  const std::string &api = objectDump["access"]["api"];
  EXPECT_STREQ(api.c_str(), "sysfs");
}

int main (int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  ::google::InitGoogleLogging(argv[0]);

  return RUN_ALL_TESTS();
}
