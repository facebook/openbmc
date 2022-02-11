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

#include <stdexcept>
#include <memory>
#include <thread>
#include <iostream>
#include <fstream>
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <gio/gio.h>
#include <nlohmann/json.hpp>
#include <ipc-interface/Ipc.h>
#include <dbus-utils/DBus.h>
#include <dbus-utils/dbus-interface/DBusObjectInterface.h>
#include "../SensorObjectTree.h"
#include "../SensorObject.h"
#include "../SensorApi.h"
#include "../SensorSysfsApi.h"
#include "../SensorJsonParser.h"
using namespace openbmc::qin;

static DBusObjectInterface interface;

void eventLoop(GMainLoop* loop) {
  LOG(INFO) << "Event loop begins";
  g_main_loop_run(loop);
  LOG(INFO) << "Event loop ends";
}

class JsonParserAttrTest : public ::testing::Test {
  protected:
    virtual void SetUp() {
      LOG(INFO) << "Creating a dbus instance and register for connection";
      sDbus_ = std::shared_ptr<DBus>(new DBus("org.openbmc.Chassis"));
      dbus_ = sDbus_.get();
      dbus_->registerConnection();
      loop = g_main_loop_new(nullptr, FALSE);
      t = std::thread(eventLoop, loop);
      dbus_->waitForConnection();

      LOG(INFO) << "Creating a sensor tree instance with objects to be "
        "registered in the dbus";
      sensorTree_ = new SensorObjectTree(sDbus_, "org");
      sensorTree_->addObject("openbmc", "/org");

      attributes_ =
      {
        {
          {"value", "100"},
          {"modes", "RW"}
        }
      };
    }

    virtual void TearDown() {
      LOG(INFO) << "Deleting the dbus and sensor tree instances";
      g_main_loop_quit(loop);
      g_main_loop_unref(loop);
      t.join();
      delete sensorTree_;
      // Notice that the connection should stay before the deletion of
      // sensorTree_ since sensorTree_ will unregister its objects in
      // the desctructor.
      dbus_->unregisterConnection();
    }

    std::shared_ptr<DBus> sDbus_;
    nlohmann::json        attributes_;
    SensorObjectTree*     sensorTree_;
    DBus*                 dbus_;
    GMainLoop*            loop;
    std::thread           t;
};

/**
 * Test parsing json attributes array into /org/openbmc.
 */
TEST_F(JsonParserAttrTest, AttributeParser) {
  Object* object = sensorTree_->getObject("/org/openbmc");
  ASSERT_TRUE(object != nullptr);

  LOG(INFO) << "Throw since the name of an attribute is missing";
  EXPECT_ANY_THROW(
    SensorJsonParser::parseGenericAttribute(attributes_, *object));

  LOG(INFO) << "Add attribute name back to avoid throwing exception";
  attributes_[0]["name"] = "temp_unit_1_input";
  EXPECT_NO_THROW(
    SensorJsonParser::parseGenericAttribute(attributes_, *object));

  LOG(INFO) << "Test if the attribute is added";
  EXPECT_EQ(object->getAttrMap().size(), 1);
  Attribute* attrTemp = nullptr;
  ASSERT_TRUE((attrTemp = object->getAttribute("temp_unit_1_input"))
              != nullptr);
  EXPECT_STREQ(attrTemp->getValue().c_str(), "100");
  EXPECT_EQ(attrTemp->getModes(), Attribute::RW);

  LOG(INFO) << "Add another attribute";
  nlohmann::json tempAttr =
  {
    {"name", "CPU_temp1"},
    {"value", "232"},
    {"modes", "RW"}
  };
  attributes_.push_back(tempAttr);

  LOG(INFO) << "Cannot parse attribute if there is duplicate attribute";
  EXPECT_ANY_THROW(
    SensorJsonParser::parseGenericAttribute(attributes_, *object));

  LOG(INFO) << "Delete attr to successfully add the new one";
  object->deleteAttribute("temp_unit_1_input");
  EXPECT_NO_THROW(
    SensorJsonParser::parseGenericAttribute(attributes_, *object));

  LOG(INFO) << "Check if the tempAttr is added";
  attrTemp = nullptr;
  ASSERT_TRUE((attrTemp = object->getAttribute("CPU_temp1")) != nullptr);
  EXPECT_STREQ(attrTemp->getValue().c_str(), "232");
  EXPECT_EQ(attrTemp->getModes(), Attribute::RW);
}

class JsonParserGenericObjectTest : public JsonParserAttrTest {
  protected:
    virtual void SetUp() {
      JsonParserAttrTest::SetUp();
      attributes_[0]["name"] = "temp_attr";
      jObject_ =
      {
        {"objectName", "Chassis"},
        {"objectType", "Generic"}
      };
      jObject_["attributes"] = attributes_;
    }

    virtual void TearDown() {
      JsonParserAttrTest::TearDown();
    }

    nlohmann::json jObject_;
};

TEST_F(JsonParserGenericObjectTest, GenericObjectTest) {
  LOG(INFO) << "Error if not specifying objectName or objectType";
  EXPECT_ANY_THROW(
    SensorJsonParser::parseObject({},
                                  *sensorTree_,
                                  "/org/openbmc"));

  LOG(INFO) << "ERROR if specified objectType is wrong";
  EXPECT_ANY_THROW(
    SensorJsonParser::parseObject({
                                    {"objectName", "Chassis"},
                                    {"objectType", "System"}
                                  },
                                  *sensorTree_,
                                  "/org/openbmc"));

  LOG(INFO) << "Add object into sensorTree_";
  SensorJsonParser::parseObject(jObject_,
                                *sensorTree_,
                                "/org/openbmc");

  LOG(INFO) << "Check if the object and attributes are successfully added";
  ASSERT_TRUE(sensorTree_->containObject("/org/openbmc/Chassis"));
  Object* object = sensorTree_->getObject("/org/openbmc/Chassis");
  Attribute* attr = nullptr;
  ASSERT_TRUE((attr = object->getAttribute("temp_attr")) != nullptr);
  EXPECT_STREQ(attr->getValue().c_str(), "100");

  LOG(INFO) << "Create child objects";
  jObject_["childObjects"].push_back(
    {
      {"objectName", "Subobject"},
      {"objectType", "Generic"}
    }
  );
  jObject_["childObjects"][0]["attributes"] = attributes_;

  LOG(INFO) << "Cannot add a duplicated object";
  EXPECT_ANY_THROW(
    SensorJsonParser::parseObject(jObject_,
                                  *sensorTree_,
                                  "/org/openbmc"));

  LOG(INFO) << "Add object successfully after removing the duplicate";
  sensorTree_->deleteObjectByName("Chassis", "/org/openbmc");
  EXPECT_NO_THROW(
    SensorJsonParser::parseObject(jObject_,
                                  *sensorTree_,
                                  "/org/openbmc"));

  LOG(INFO) << "Check if the child object including its attributes is "
    "successfully added";
  ASSERT_TRUE(sensorTree_->containObject("/org/openbmc/Chassis/Subobject"));
  object = sensorTree_->getObject("/org/openbmc/Chassis/Subobject");
  ASSERT_TRUE(object->getAttribute("temp_attr") != nullptr);
}

class JsonParserSensorObjectTest : public JsonParserGenericObjectTest {
  protected:
    virtual void SetUp() {
      JsonParserGenericObjectTest::SetUp();
      jObject_["objectName"] = "CPU_sensor";
      jObject_["objectType"] = "SensorDevice";
    }

    virtual void TearDown() {
      JsonParserGenericObjectTest::TearDown();
    }
};

TEST_F(JsonParserSensorObjectTest, SensorObjectTest) {
  LOG(INFO) << "Throw since no access method specified";
  EXPECT_ANY_THROW(
    SensorJsonParser::parseObject(jObject_,
                                  *sensorTree_,
                                  "/org/openbmc"));

  LOG(INFO) << "Specify a wrong access api in the sensor object";
  jObject_["access"] = {
    {"api", "web"},  // can only be sysfs or i2c
    {"path", "/tmp"}
  };
  EXPECT_ANY_THROW(
    SensorJsonParser::parseObject(jObject_,
                                  *sensorTree_,
                                  "/org/openbmc"));

  LOG(INFO) << "Specify the correct access api of sysfs and "
    "add the sensor device into sensorTree_";
  jObject_["access"]["api"] = "sysfs";
  SensorJsonParser::parseObject(jObject_,
                                *sensorTree_,
                                "/org/openbmc");

  ASSERT_TRUE(sensorTree_->containObject("/org/openbmc/CPU_sensor"));
  SensorDevice* sObject =
    sensorTree_->getSensorDevice("/org/openbmc/CPU_sensor");
  ASSERT_TRUE(sObject != nullptr);
  SensorAttribute* sAttr = sObject->getAttribute("temp_attr");
  ASSERT_TRUE(sAttr != nullptr);
  EXPECT_FALSE(sAttr->isAccessible());

  LOG(INFO) << "Add a sensor attribute into the sensor object "
    "with addr specified";
  jObject_["attributes"][0]["addr"] = "test_input";

  LOG(INFO) << "Check if the sensor device is added";

  sensorTree_->deleteObjectByPath("/org/openbmc/CPU_sensor");
  ASSERT_NO_THROW(
    SensorJsonParser::parseObject(jObject_,
                                  *sensorTree_,
                                  "/org/openbmc"));

  LOG(INFO) << "Check if the sensor attribute is successfully added";
  ASSERT_TRUE(sensorTree_->containObject("/org/openbmc/CPU_sensor"));
  sObject = nullptr;
  ASSERT_TRUE((sObject =
    sensorTree_->getSensorDevice("/org/openbmc/CPU_sensor")) != nullptr);
  sAttr = nullptr;
  EXPECT_TRUE((sAttr = sObject->getAttribute("temp_attr"))
    != nullptr);
  EXPECT_STREQ(sAttr->getAddr().c_str(), "test_input");
  EXPECT_TRUE(sAttr->isAccessible());
}

TEST_F(JsonParserSensorObjectTest, JsonFileReadingTest) {
  LOG(INFO) << "Parse the input file \"SensorInfo.json\"";
  SensorJsonParser::parse("SensorInfo.json",
                          *sensorTree_,
                          "/org/openbmc");

  LOG(INFO) << "Check if the objects exist";
  EXPECT_TRUE(sensorTree_->containObject("/org/openbmc/Chassis"));
  EXPECT_TRUE(sensorTree_->
    containObject("/org/openbmc/Chassis/CPU_1_temp_sensor"));
  EXPECT_TRUE(sensorTree_->
    containObject("/org/openbmc/Chassis/CPU_2_temp_sensor"));

  LOG(INFO) << "Check if the attributes exist";
  Object* object =
    sensorTree_->getObject("/org/openbmc/Chassis/CPU_1_temp_sensor/temp");
  ASSERT_TRUE(object != nullptr);
  EXPECT_TRUE(object->getAttribute("CPU_temp") != nullptr);
  EXPECT_TRUE(object->getAttribute("CPU_temp_max") != nullptr);
  EXPECT_TRUE(object->getAttribute("CPU_temp_max_hyst") != nullptr);
  object =
    sensorTree_->getObject("/org/openbmc/Chassis/CPU_2_temp_sensor/temp");
  ASSERT_TRUE(object != nullptr);
  EXPECT_TRUE(object->getAttribute("CPU_temp") != nullptr);
  EXPECT_TRUE(object->getAttribute("CPU_temp_max") != nullptr);
  EXPECT_TRUE(object->getAttribute("CPU_temp_max_hyst") != nullptr);
}

int main (int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  ::google::InitGoogleLogging(argv[0]);

  return RUN_ALL_TESTS();
}
