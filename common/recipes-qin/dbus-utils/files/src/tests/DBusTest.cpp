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
#include <thread>
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <gio/gio.h>
#include "../DBus.h"
#include "DBusTestInterface.h"
#include "DBusDummyInterface.h"
using namespace openbmc::qin;

static DBusTestInterface testInterface;

class DBusTest : public ::testing::Test {
  protected:
    virtual void SetUp() {
      dbus_ = new DBus("org.openbmc.Chassis");
    }

    virtual void TearDown() {
      delete dbus_;
    }

    DBus* dbus_;
};

TEST_F(DBusTest, Constructor) {
  EXPECT_THROW(DBus wrongName1(".."), std::invalid_argument);
  EXPECT_THROW(DBus wrongName2("./efgwe/."), std::invalid_argument);
  EXPECT_STREQ(dbus_->getDefaultInterface().getName().c_str(),
               "org.openbmc.DefaultInterface");
  DBus dbus0("org.openbmc.System", &testInterface);
  EXPECT_STREQ(dbus0.getDefaultInterface().getName().c_str(),
               "org.openbmc.TestInterface");
  EXPECT_STREQ(dbus_->getName().c_str(), "org.openbmc.Chassis");
  EXPECT_EQ(dbus_->getId(), 0);
  EXPECT_EQ(dbus_->getDBusObjectMap().size(), 0);
  EXPECT_FALSE(dbus_->isConnected());
}

void eventLoop(GMainLoop* loop) {
  LOG(INFO) << "Event loop begins";
  g_main_loop_run(loop);
  LOG(INFO) << "Event loop ends";
}

TEST_F(DBusTest, Connection) {
  ASSERT_NO_THROW(dbus_->registerConnection());
  GMainLoop *loop = g_main_loop_new(nullptr, FALSE);
  auto thread = std::thread(eventLoop, loop);
  dbus_->waitForConnection();
  ASSERT_TRUE(dbus_->getId() > 0);
  ASSERT_TRUE(dbus_->isConnected());

  // Unregistering the connection during event loop on
  // will cause the event loop to report error.
  // Don't do this in the user application.
  ASSERT_NO_THROW(dbus_->unregisterConnection());

  ASSERT_EQ(dbus_->getId(), 0);
  ASSERT_FALSE(dbus_->isConnected());
  ASSERT_NO_THROW(dbus_->registerConnection());
  dbus_->waitForConnection();
  EXPECT_GT(dbus_->getId(), 0);
  g_main_loop_quit(loop);
  g_main_loop_unref(loop);
  thread.join();
  ASSERT_NO_THROW(dbus_->unregisterConnection());
}

class DBusConnectedTest : public DBusTest {
  protected:
    virtual void SetUp() {
      DBusTest::SetUp();
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
      DBusTest::TearDown();
    }

    GMainLoop *loop;
    std::thread t;
};

TEST_F(DBusConnectedTest, DBusObject) {
  DBusTestInterface tface;
  DBusDummyInterface dface;
  int userData = 1;
  EXPECT_THROW(dbus_->registerObject("/org.gg", tface, &userData),
               std::invalid_argument);
  EXPECT_NO_THROW(dbus_->registerObject("/org/openbmc/Chassis/Device1",
                  tface, &userData));
  EXPECT_NO_THROW(dbus_->registerObject("/org/openbmc/Chassis/Card",
                  &userData)); // register default interface
  EXPECT_TRUE(dbus_->getDBusObject("/org/openbmc/Chassis/Card")
              ->containInterface(dbus_->getDefaultInterface().getName()));
  EXPECT_NO_THROW(dbus_->registerObject("/org/openbmc/Chassis/Device1",
                  tface, &userData));
  EXPECT_NO_THROW(dbus_->registerObject("/org/openbmc/Chassis/Device2",
                  tface, &userData));
  EXPECT_NO_THROW(dbus_->registerObject("/org/openbmc/Chassis/Device2",
                  dface, &userData)); // register the second interface
  EXPECT_TRUE(dbus_->isObjectRegistered("/org/openbmc/Chassis/Device1",
              tface));
  EXPECT_TRUE(dbus_->isObjectRegistered("/org/openbmc/Chassis/Device2",
              tface));
  EXPECT_TRUE(dbus_->isObjectRegistered("/org/openbmc/Chassis/Device2",
              dface)); // second interface should be registered
  EXPECT_NO_THROW(dbus_->unregisterObject("/org/openbmc/Chassis/Device1",
                  tface));
  EXPECT_NO_THROW(dbus_->unregisterObject("/org/openbmc/Chassis/Device2"));
  EXPECT_FALSE(dbus_->isObjectRegistered("/org/openbmc/Chassis/Device1",
               tface));
  EXPECT_FALSE(dbus_->isObjectRegistered("/org/openbmc/Chassis/Device2",
               tface));
  EXPECT_FALSE(dbus_->isObjectRegistered("/org/openbmc/Chassis/Device2",
               dface));
}

int main (int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  ::google::InitGoogleLogging(argv[0]);

  return RUN_ALL_TESTS();
}
