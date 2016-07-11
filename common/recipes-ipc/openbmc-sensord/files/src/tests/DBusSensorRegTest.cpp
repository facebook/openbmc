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
#include "../SensorObject.h"
#include "../SensorApi.h"
#include "../SensorSysfsApi.h"
#include "DBusObjectTreeInterface.h"
using namespace openbmc::ipc;

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
  ASSERT_TRUE(dbus_->isObjectRegistered("/org/openbmc/Chassis", defaultInterface));
  EXPECT_TRUE(sensorTree.getObject("/org/openbmc/Chassis") != nullptr);

  sensorTree.deleteObjectByPath("/org"); // failed since it is root
  ASSERT_TRUE(dbus_->isObjectRegistered("/org", defaultInterface));
  sensorTree.deleteObjectByName("openbmc", "/org"); // failed since it has children
  ASSERT_TRUE(dbus_->isObjectRegistered("/org/openbmc", defaultInterface));
  sensorTree.deleteObjectByPath("/org/openbmc/Chassis"); // deletion successful
  ASSERT_FALSE(dbus_->isObjectRegistered("/org/openbmc/Chassis", defaultInterface));
  sensorTree.deleteObjectByName("openbmc", "/org"); // deletion successful
  ASSERT_FALSE(dbus_->isObjectRegistered("/org/openbmc", defaultInterface));
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
  SensorObject* obj = sensorTree.addSensorObject("CPU_sensor",
                                                 "/org/openbmc/Chassis",
                                                 std::move(uSysfsApi));
  EXPECT_TRUE(obj != nullptr);
  SensorSysfsApi* sysfsApi = static_cast<SensorSysfsApi*>(obj->getSensorApi());
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
  std::unique_ptr<SensorSysfsApi> uSysfsApi(new SensorSysfsApi(fsPath));
  SensorObject* obj = sensorTree.addSensorObject("CPU_sensor",
                                                 "/org/openbmc",
                                                 std::move(uSysfsApi));
  ASSERT_TRUE(obj != nullptr);

  // write to a file to test reading value
  const std::string fileName = "testfile";
  const std::string filePath = fsPath + "/" + fileName;
  const std::string value = "100";
  std::ofstream outFile(filePath);
  outFile << "100";
  outFile.close();

  // add a sensor attribute to test reading (writing) value from the file
  SensorObject* sobject;
  sobject = static_cast<SensorObject*>(sensorTree.getObject(
      "/org/openbmc/CPU_sensor"));
  ASSERT_TRUE(sobject != nullptr);
  SensorAttribute* sattr = sobject->addAttribute("CPU_temp", "temp");
  sattr->setAddr(fileName);
  EXPECT_STREQ(sattr->getValue().c_str(), "");
  EXPECT_STREQ(sobject->readAttrValue("CPU_temp", "temp").c_str(),
               value.c_str());
  EXPECT_STREQ(sattr->getValue().c_str(), value.c_str());

  sattr->setModes(Attribute::RW);
  const std::string newValue = "1000";
  sobject->writeAttrValue(newValue, "CPU_temp", "temp"); // write to file
  EXPECT_STREQ(sattr->getValue().c_str(), newValue.c_str());
  EXPECT_STREQ(sobject->readAttrValue("CPU_temp", "temp").c_str(),
               newValue.c_str());

  // the generic object does not read from the file
  Object* object;
  EXPECT_TRUE((object = sensorTree.getObject("/org/openbmc")) != nullptr);
  Attribute* attr = object->addAttribute("tag", "label");
  EXPECT_STREQ(attr->getValue().c_str(), "");
  EXPECT_STREQ(object->readAttrValue("tag", "label").c_str(), "");
  attr->setModes(Attribute::RW);
  object->writeAttrValue(value, "tag", "label");
  EXPECT_STREQ(attr->getValue().c_str(), value.c_str());
}

int main (int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  ::google::InitGoogleLogging(argv[0]);

  return RUN_ALL_TESTS();
}
