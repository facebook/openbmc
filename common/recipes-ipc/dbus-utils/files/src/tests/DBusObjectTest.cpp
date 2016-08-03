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

#include <gtest/gtest.h>
#include <glog/logging.h>
#include "../DBusObject.h"
#include "DBusTestInterface.h"
#include "DBusDummyInterface.h"
using namespace openbmc::qin;

class DBusObjectTest : public ::testing::Test {
  protected:
    virtual void SetUp() {
      obj_ = new DBusObject("/org/openbmc");
    }

    virtual void TearDown() {
      delete obj_;
    }

    DBusObject* obj_;
};

TEST_F(DBusObjectTest, Constructor) {
  ASSERT_STREQ(obj_->getPath().c_str(), "/org/openbmc");
  EXPECT_TRUE(obj_->getInterfaceMap().size() == 0);
}

TEST_F(DBusObjectTest, InterfaceOperation) {
  DBusTestInterface testInterface;
  DBusTestInterface dupInterface;
  DBusDummyInterface dummyInterface;
  obj_->addInterface(testInterface, obj_);
  ASSERT_TRUE(obj_->containInterface(testInterface.getName()));
  EXPECT_TRUE(obj_->getInterface(testInterface.getName()) == &testInterface);
  EXPECT_EQ(obj_->getInterfaceMap().size(), 1);
  EXPECT_TRUE(obj_->getUserData(testInterface.getName()) == obj_);
  EXPECT_FALSE(obj_->containInterface(dummyInterface.getName()));

  unsigned int id;
  EXPECT_TRUE(obj_->getId(testInterface.getName(), id));
  EXPECT_TRUE(id == 0);
  EXPECT_FALSE(obj_->getId(dummyInterface.getName(), id));
  obj_->setId(testInterface.getName(), 2);
  EXPECT_TRUE(obj_->getId(testInterface.getName(), id));
  EXPECT_TRUE(id == 2);

  obj_->addInterface(dupInterface, obj_); // failed since duplicated
  EXPECT_EQ(obj_->getInterfaceMap().size(), 1);
  EXPECT_TRUE(obj_->getInterface(testInterface.getName()) == &testInterface);

  obj_->addInterface(dummyInterface, obj_);
  EXPECT_TRUE(obj_->getUserData(dummyInterface.getName()) == obj_);
  EXPECT_EQ(obj_->getInterfaceMap().size(), 2);
  EXPECT_TRUE(obj_->getInterface(dummyInterface.getName()) == &dummyInterface);

  obj_->removeInterface(testInterface.getName()); // failed since id != 0
  EXPECT_TRUE(obj_->containInterface(testInterface.getName()));
  EXPECT_TRUE(obj_->containInterface(dummyInterface.getName()));
  EXPECT_EQ(obj_->getInterfaceMap().size(), 2);

  obj_->setId(testInterface.getName(), 0);
  EXPECT_TRUE(obj_->getId(testInterface.getName(), id));
  EXPECT_TRUE(id == 0);
  obj_->removeInterface(testInterface.getName());
  EXPECT_FALSE(obj_->containInterface(testInterface.getName()));
  EXPECT_TRUE(obj_->containInterface(dummyInterface.getName()));
  EXPECT_EQ(obj_->getInterfaceMap().size(), 1);

  obj_->removeInterface(dummyInterface.getName());
  EXPECT_FALSE(obj_->containInterface(dummyInterface.getName()));
  EXPECT_EQ(obj_->getInterfaceMap().size(), 0);
}

int main (int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  ::google::InitGoogleLogging(argv[0]);

  return RUN_ALL_TESTS();
}
