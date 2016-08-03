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
#include <ipc-interface/Ipc.h>
#include <dbus-utils/DBus.h>
#include <dbus-utils/DBusInterfaceBase.h>
#include "../SensorObjectTree.h"
#include "../SensorDevice.h"
#include "../SensorObject.h"
#include "../SensorTemp.h"
#include "../SensorPower.h"
#include "../SensorPwm.h"
#include "../SensorVoltage.h"
#include "../SensorCurrent.h"
#include "../SensorFan.h"
#include "../SensorApi.h"
#include "../SensorSysfsApi.h"
#include "DBusObjectTreeInterface.h"
using namespace openbmc::qin;

static DBusObjectTreeInterface testInterface;

void eventLoop(GMainLoop* loop) {
  LOG(INFO) << "Event loop begins";
  g_main_loop_run(loop);
  LOG(INFO) << "Event loop ends";
}

class DBusDefaultTest : public ::testing::Test {
  protected:
    virtual void SetUp() {
      sDbus_ = std::shared_ptr<DBus>(new DBus("org.openbmc.Chassis"));
      dbus_ = sDbus_.get();
      dbus_->registerConnection();
      loop = g_main_loop_new(nullptr, FALSE);
      t = std::thread(eventLoop, loop);
      dbus_->waitForConnection();
    }

    virtual void TearDown() {
      g_main_loop_quit(loop);
      g_main_loop_unref(loop);
      t.join();
      dbus_->unregisterConnection();
    }

    std::shared_ptr<DBus> sDbus_;
    DBus* dbus_;
    GMainLoop *loop;
    std::thread t;
};

/**
 * Test the SensorObjectTree constructor with dbus as ipc.
 * The root object should be registered immediately.
 */
TEST_F(DBusDefaultTest, DefaultObjectRegistration) {
  SensorObjectTree sensorTree(sDbus_, "org");
  DBusInterfaceBase &defaultInterface = dbus_->getDefaultInterface();
  ASSERT_TRUE(dbus_->isObjectRegistered("/org", defaultInterface));
  sensorTree.addObject("openbmc", "/org");
  ASSERT_TRUE(dbus_->isObjectRegistered("/org/openbmc", defaultInterface));
  sensorTree.addObject("Chassis", "/org/openbmc");
  ASSERT_TRUE(dbus_->isObjectRegistered("/org/openbmc/Chassis",
              defaultInterface));
  EXPECT_THROW(sensorTree.getSensorObject("/org/openbmc/Chassis"),
               std::invalid_argument);

  // failed since it is root
  EXPECT_ANY_THROW(sensorTree.deleteObjectByPath("/org"));
  ASSERT_TRUE(dbus_->isObjectRegistered("/org", defaultInterface));
  // failed since it has children
  EXPECT_ANY_THROW(sensorTree.deleteObjectByName("openbmc", "/org"));
  ASSERT_TRUE(dbus_->isObjectRegistered("/org/openbmc", defaultInterface));
  // deletion successful
  EXPECT_NO_THROW(sensorTree.deleteObjectByPath("/org/openbmc/Chassis"));
  ASSERT_FALSE(dbus_->isObjectRegistered("/org/openbmc/Chassis", defaultInterface));
  // deletion successful
  EXPECT_NO_THROW(sensorTree.deleteObjectByName("openbmc", "/org"));
  ASSERT_FALSE(dbus_->isObjectRegistered("/org/openbmc", defaultInterface));
}

/**
 * Test adding the subtypes of SensorObject such as SensorTemp and so on.
 */
TEST_F(DBusDefaultTest, AddSensorObjectSubtypes) {
  SensorObjectTree sensorTree(sDbus_, "org");
  sensorTree.addObject("openbmc", "/org");
  // SensorObject can only be the child of SensorDevice
  EXPECT_THROW(sensorTree.addSensorObject("Sensor1_temp", "/org/openbmc"),
               std::invalid_argument);
  std::unique_ptr<SensorSysfsApi> uSysfsApi(new SensorSysfsApi("/"));
  EXPECT_NO_THROW(sensorTree.addSensorDevice("Sensor1",
                                             "/org/openbmc",
                                             std::move(uSysfsApi)));

  Object* sObject = nullptr;
  std::unique_ptr<SensorObject> uObj(new SensorTemp("temp"));
  sObject = sensorTree.addObject(std::move(uObj), "/org/openbmc/Sensor1");
  EXPECT_TRUE(sObject != nullptr);
  EXPECT_TRUE(dynamic_cast<SensorTemp*>(sObject) != nullptr);

  uObj = std::unique_ptr<SensorObject>(new SensorFan("fan"));
  sObject = sensorTree.addObject(std::move(uObj), "/org/openbmc/Sensor1");
  EXPECT_TRUE(sObject != nullptr);
  EXPECT_TRUE(dynamic_cast<SensorFan*>(sObject) != nullptr);

  uObj = std::unique_ptr<SensorObject>(new SensorPower("power"));
  sObject = sensorTree.addObject(std::move(uObj), "/org/openbmc/Sensor1");
  EXPECT_TRUE(sObject != nullptr);
  EXPECT_TRUE(dynamic_cast<SensorPower*>(sObject) != nullptr);

  uObj = std::unique_ptr<SensorObject>(new SensorPwm("pwm"));
  sObject = sensorTree.addObject(std::move(uObj), "/org/openbmc/Sensor1");
  EXPECT_TRUE(sObject != nullptr);
  EXPECT_TRUE(dynamic_cast<SensorPwm*>(sObject) != nullptr);

  uObj = std::unique_ptr<SensorObject>(new SensorVoltage("voltage"));
  sObject = sensorTree.addObject(std::move(uObj), "/org/openbmc/Sensor1");
  EXPECT_TRUE(sObject != nullptr);
  EXPECT_TRUE(dynamic_cast<SensorVoltage*>(sObject) != nullptr);

  uObj = std::unique_ptr<SensorObject>(new SensorCurrent("current"));
  sObject = sensorTree.addObject(std::move(uObj), "/org/openbmc/Sensor1");
  EXPECT_TRUE(sObject != nullptr);
  EXPECT_TRUE(dynamic_cast<SensorCurrent*>(sObject) != nullptr);
}

class DBusSensorObjectTreeTest : public ::testing::Test {
  protected:
    virtual void SetUp() {
      sDbus_ = std::shared_ptr<DBus>(new DBus("org.openbmc.Chassis", &testInterface));
      dbus_ = sDbus_.get();
      dbus_->registerConnection();
      loop = g_main_loop_new(nullptr, FALSE);
      t = std::thread(eventLoop, loop);
      dbus_->waitForConnection();
    }

    virtual void TearDown() {
      g_main_loop_quit(loop);
      g_main_loop_unref(loop);
      t.join();
      dbus_->unregisterConnection();
    }

    std::shared_ptr<DBus> sDbus_;
    DBus* dbus_;
    GMainLoop *loop;
    std::thread t;
};

TEST_F(DBusSensorObjectTreeTest, AddObjectByUniquePtr) {
  const std::string fsPath = "/sys/class/hwmon/hwmon1";
  SensorObjectTree sensorTree(sDbus_, "org");

  std::unique_ptr<Object> uObj(new Object("openbmc"));
  // successful
  EXPECT_NO_THROW(sensorTree.addObject(std::move(uObj), "/org"));

  std::unique_ptr<SensorSysfsApi> uSysfsApi(new SensorSysfsApi(fsPath));
  std::unique_ptr<SensorDevice> uDev(new SensorDevice(
      "CPU_sensor", std::move(uSysfsApi)));

  // failed since parent path cannot be found
  EXPECT_ANY_THROW(sensorTree.addObject(std::move(uDev), "/orgg"));

  uSysfsApi = std::unique_ptr<SensorSysfsApi>(new SensorSysfsApi(fsPath));
  uDev = std::unique_ptr<SensorDevice>(new SensorDevice(
      "CPU_sensor", std::move(uSysfsApi)));
  // successful
  EXPECT_TRUE(sensorTree.addObject(std::move(uDev), "/org/openbmc")
      != nullptr);
  EXPECT_TRUE(sensorTree.containObject("/org/openbmc/CPU_sensor"));
  Object* obj = nullptr;
  EXPECT_TRUE((obj = sensorTree.getObject("/org/openbmc/CPU_sensor"))
      != nullptr);
  EXPECT_NO_THROW(sensorTree.getSensorDevice("/org/openbmc/CPU_sensor"));
  EXPECT_TRUE(obj->getParent() == sensorTree.getObject("/org/openbmc"));
  ASSERT_TRUE((obj = sensorTree.getObject("/org/openbmc")) != nullptr);
  EXPECT_TRUE(obj->getChildObject("CPU_sensor") != nullptr);

  std::unique_ptr<SensorObject> uSObj(new SensorObject("temp"));

  // failed since parent is not SensorDevice
  EXPECT_ANY_THROW(sensorTree.addObject(std::move(uSObj), "/org"));

  uSObj = std::unique_ptr<SensorObject>(new SensorObject("temp"));
  // successful
  EXPECT_TRUE(sensorTree.addObject(std::move(uSObj), "/org/openbmc/CPU_sensor")
      != nullptr);
  EXPECT_TRUE(sensorTree.containObject("/org/openbmc/CPU_sensor/temp"));
  obj = nullptr;
  EXPECT_TRUE((obj = sensorTree.getObject("/org/openbmc/CPU_sensor/temp"))
      != nullptr);
  EXPECT_NO_THROW(sensorTree.getSensorObject("/org/openbmc/CPU_sensor/temp"));
  EXPECT_TRUE(obj->getParent() ==
      sensorTree.getObject("/org/openbmc/CPU_sensor"));
  ASSERT_TRUE((obj = sensorTree.getObject("/org/openbmc/CPU_sensor")) != nullptr);
  EXPECT_TRUE(obj->getChildObject("temp") != nullptr);
}

/**
 * Test the sensor tree for registering dbus with test interface.
 */
TEST_F(DBusSensorObjectTreeTest, SensorTreeInterfaceRegistration) {
  const std::string fsPath = "/sys/class/hwmon/hwmon1";
  SensorObjectTree sensorTree(sDbus_, "org");
  EXPECT_TRUE(&(dbus_->getDefaultInterface()) == &testInterface);
  EXPECT_TRUE(dbus_->isObjectRegistered("/org", testInterface));
  sensorTree.addObject("openbmc", "/org");
  sensorTree.addObject("Chassis", "/org/openbmc");
  std::unique_ptr<SensorSysfsApi> uSysfsApi(new SensorSysfsApi(fsPath));
  SensorDevice* dev = sensorTree.addSensorDevice("CPU_sensor",
                                                 "/org/openbmc/Chassis",
                                                 std::move(uSysfsApi));
  EXPECT_TRUE(dev != nullptr);
  SensorSysfsApi* sysfsApi = static_cast<SensorSysfsApi*>(dev->getSensorApi());
  EXPECT_STREQ(sysfsApi->getFsPath().c_str(), fsPath.c_str());
}

/**
 * Test sensor tree over object reading and writing functions over attriburte
 * value. Here the object has two different types, the generic object and
 * the sensor object. Only the sensor object can read to and write from a
 * file.
 */
TEST_F(DBusSensorObjectTreeTest, AttributeReadWriteTest) {
  // create a sensor object with sysfs SensorApi
  const std::string fsPath = "/tmp";
  SensorObjectTree sensorTree(sDbus_, "org");
  sensorTree.addObject("openbmc", "/org");
  EXPECT_THROW(sensorTree.getSensorObject("/org/openbmc"),
               std::invalid_argument); // not a SensorObject
  EXPECT_THROW(sensorTree.getSensorDevice("/org/openbmc"),
               std::invalid_argument); // not a SensorDevice

  std::unique_ptr<SensorSysfsApi> uSysfsApi(new SensorSysfsApi(fsPath));
  SensorDevice* dev = sensorTree.addSensorDevice("CPU_sensor",
                                                 "/org/openbmc",
                                                 std::move(uSysfsApi));
  ASSERT_TRUE(dev != nullptr);

  // write to a file to test reading value
  const std::string fileName = "testfile";
  const std::string filePath = fsPath + "/" + fileName;
  const std::string value = "100";
  std::ofstream outFile(filePath);
  outFile << "100";
  outFile.close();

  // add temp Sen
  SensorDevice* sDevice = nullptr;
  EXPECT_THROW(sensorTree.getSensorObject("/org/openbmc/CPU_sensor"),
               std::invalid_argument); // not a SensorObject
  EXPECT_NO_THROW(sDevice =
                    sensorTree.getSensorDevice("/org/openbmc/CPU_sensor"));
  EXPECT_TRUE(sDevice != nullptr);

  // add a sensor attribute to test reading (writing) value from the file
  SensorAttribute* sattr = sDevice->addAttribute("CPU_temp");
  sattr->setAddr(fileName);
  EXPECT_STREQ(sattr->getValue().c_str(), "");
  EXPECT_STREQ(sDevice->readAttrValue("CPU_temp").c_str(),
               value.c_str());
  EXPECT_STREQ(sattr->getValue().c_str(), value.c_str());

  sattr->setModes(Attribute::RW);
  const std::string newValue = "1000";
  sDevice->writeAttrValue("CPU_temp", newValue); // write to file
  EXPECT_STREQ(sattr->getValue().c_str(), newValue.c_str());
  EXPECT_STREQ(sDevice->readAttrValue("CPU_temp").c_str(), newValue.c_str());

  // the generic object does not read from the file
  Object* object;
  EXPECT_TRUE((object = sensorTree.getObject("/org/openbmc")) != nullptr);
  Attribute* attr = object->addAttribute("tag");
  EXPECT_STREQ(attr->getValue().c_str(), "");
  EXPECT_STREQ(object->readAttrValue("tag").c_str(), "");
  attr->setModes(Attribute::RW);
  object->writeAttrValue("tag", value);
  EXPECT_STREQ(attr->getValue().c_str(), value.c_str());
}

int main (int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  ::google::InitGoogleLogging(argv[0]);

  return RUN_ALL_TESTS();
}
