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

#include <memory>
#include <stdexcept>
#include <thread>
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <gio/gio.h>
#include <ipc-interface/Ipc.h>
#include <dbus-utils/DBus.h>
#include <dbus-utils/DBusInterfaceBase.h>
#include <object-tree/ObjectTree.h>
#include <object-tree/Object.h>
#include <object-tree/Attribute.h>
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
 * Test the objectTree constructor with dbus as ipc.
 * The root object should be registered immediately.
 */
TEST_F(DBusDefaultTest, DefaultObjectRegistration) {
  ObjectTree obTree(sDbus_, "org");
  DBusInterfaceBase &defaultInterface = dbus_->getDefaultInterface();
  ASSERT_TRUE(dbus_->isObjectRegistered("/org", defaultInterface));
  obTree.addObject("openbmc", "/org");
  ASSERT_TRUE(dbus_->isObjectRegistered("/org/openbmc", defaultInterface));
  obTree.addObject("Chassis", "/org/openbmc");
  ASSERT_TRUE(dbus_->isObjectRegistered("/org/openbmc/Chassis", defaultInterface));
  obTree.deleteObjectByPath("/org"); // failed since it is root
  ASSERT_TRUE(dbus_->isObjectRegistered("/org", defaultInterface));
  obTree.deleteObjectByName("openbmc", "/org"); // failed since it has children
  ASSERT_TRUE(dbus_->isObjectRegistered("/org/openbmc", defaultInterface));
  obTree.deleteObjectByPath("/org/openbmc/Chassis"); // deletion successful
  ASSERT_FALSE(dbus_->isObjectRegistered("/org/openbmc/Chassis", defaultInterface));
  obTree.deleteObjectByName("openbmc", "/org"); // deletion successful
  ASSERT_FALSE(dbus_->isObjectRegistered("/org/openbmc", defaultInterface));
}

class DBusObjectTreeTest : public ::testing::Test {
  protected:
    virtual void SetUp() {
      sDbus_ = std::shared_ptr<DBus>(new DBus("org.openbmc.Chassis",
                                              &testInterface));
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

TEST_F(DBusObjectTreeTest, ObjectTreeInterfaceRegistration) {
  ObjectTree obTree(sDbus_, "org");
  EXPECT_TRUE(&(dbus_->getDefaultInterface()) == &testInterface);
  EXPECT_TRUE(dbus_->isObjectRegistered("/org", testInterface));
  obTree.addObject("openbmc", "/org");
  EXPECT_TRUE(dbus_->isObjectRegistered("/org/openbmc", testInterface));
}

int main (int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  ::google::InitGoogleLogging(argv[0]);

  return RUN_ALL_TESTS();
}
