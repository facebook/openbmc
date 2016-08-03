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
#include <string.h>
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <ipc-interface/Ipc.h>
#include "DummyIpc.h"
#include "../ObjectTree.h"
#include "../Object.h"
#include "../Attribute.h"
using namespace openbmc::qin;

class ObjectTreeTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    objTree_ = new ObjectTree(std::shared_ptr<Ipc>(new DummyIpc()), "org");
  }

  virtual void TearDown() {
    delete objTree_;
  }

  ObjectTree* objTree_;
};

TEST(ConstructorTest, Constructor) {
  std::shared_ptr<Ipc> emptyPtr;
  EXPECT_THROW(ObjectTree ot(emptyPtr, "root"), std::invalid_argument);

  ObjectTree objTree(std::shared_ptr<Ipc>(new DummyIpc()), "root");
  EXPECT_EQ(objTree.getObjectCount(), 1);
  ASSERT_TRUE(objTree.getRoot() != nullptr);
  EXPECT_TRUE(objTree.containObject("/root"));
  Object* root = objTree.getRoot();
  EXPECT_STREQ(root->getName().c_str(), "root");
  EXPECT_TRUE(root->getParent() == nullptr);
  EXPECT_EQ(root->getChildMap().size(), 0);
}

TEST_F(ObjectTreeTest, ObjectOperation) {
  Object* obj0 = objTree_->addObject("openbmc", "/org");
  ASSERT_TRUE(obj0 != nullptr);
  EXPECT_STREQ(obj0->getName().c_str(), "openbmc");
  EXPECT_EQ(objTree_->getObjectCount(), 2);
  EXPECT_TRUE(objTree_->containObject("/org/openbmc"));
  ASSERT_TRUE(objTree_->getRoot()->getChildObject("openbmc") == obj0);

  Object* obj = objTree_->getObject("/org/openbmc");
  ASSERT_TRUE(obj != nullptr && obj == obj0);
  EXPECT_STREQ(obj->getName().c_str(), "openbmc");
  ASSERT_TRUE(obj->getParent() != nullptr);
  ASSERT_TRUE(obj->getParent() == objTree_->getRoot());
  EXPECT_EQ(objTree_->getRoot()->getChildMap().size(), 1);

  // wrong parent path
  EXPECT_THROW(objTree_->addObject("Chassis", "/openbmc"),
               std::invalid_argument);
  EXPECT_FALSE(objTree_->containObject("/openbmc/Chassis"));
  EXPECT_EQ(objTree_->getObjectCount(), 2);

  ASSERT_TRUE((obj = objTree_->addObject("Chassis", "/org/openbmc"))
              != nullptr);
  EXPECT_TRUE(objTree_->containObject("/org/openbmc/Chassis"));
  EXPECT_TRUE(objTree_->getObject("/org/openbmc")->getChildObject("Chassis")
              == obj);
  EXPECT_EQ(objTree_->getObjectCount(), 3);

  // cannot delete root
  EXPECT_THROW(objTree_->deleteObjectByPath("/org"), std::invalid_argument);
  EXPECT_EQ(objTree_->getObjectCount(), 3);
  ASSERT_TRUE(objTree_->containObject("/org"));

  // cannot delete object with children
  EXPECT_THROW(objTree_->deleteObjectByName("openbmc", "/org"),
               std::invalid_argument);
  EXPECT_THROW(objTree_->deleteObjectByPath("/org/openbmc"),
               std::invalid_argument);
  EXPECT_EQ(objTree_->getObjectCount(), 3);
  ASSERT_TRUE(objTree_->containObject("/org/openbmc"));

  objTree_->deleteObjectByName("Chassis", "/org/openbmc");
  EXPECT_FALSE(objTree_->containObject("/org/openbmc/Chassis"));
  EXPECT_EQ(objTree_->getObjectCount(), 2);

  // wrong name
  EXPECT_ANY_THROW(objTree_->deleteObjectByName("openbmc", "/root"));
  // wrong path
  EXPECT_ANY_THROW(objTree_->deleteObjectByPath("/root/openbmc"));
  EXPECT_EQ(objTree_->getObjectCount(), 2);

  objTree_->deleteObjectByPath("/org/openbmc");
  EXPECT_FALSE(objTree_->containObject("/org/openbmc"));
  EXPECT_EQ(objTree_->getObjectCount(), 1);
}

TEST_F(ObjectTreeTest, AddObjectByUniquePtr) {
  std::unique_ptr<Object> uObj(new Object("openbmc"));
  // failed since parent path cannot be found
  EXPECT_ANY_THROW(objTree_->addObject(std::move(uObj), "/orgg"));

  uObj = std::unique_ptr<Object>(new Object("openbmc"));
  // successful
  EXPECT_TRUE(objTree_->addObject(std::move(uObj), "/org") != nullptr);
  EXPECT_TRUE(objTree_->containObject("/org/openbmc"));
  Object* obj = nullptr;
  EXPECT_TRUE((obj = objTree_->getObject("/org/openbmc")) != nullptr);
  EXPECT_TRUE(obj->getParent() == objTree_->getObject("/org"));
  ASSERT_TRUE((obj = objTree_->getObject("/org")) != nullptr);
  EXPECT_TRUE(obj->getChildObject("openbmc") != nullptr);

  // failed since uObj is empty
  EXPECT_ANY_THROW(objTree_->addObject(std::move(uObj), "/org"));

  uObj = std::unique_ptr<Object>(new Object("openbmc"));
  // failed since /org already has an object named openbmc
  EXPECT_ANY_THROW(objTree_->addObject(std::move(uObj), "/org"));

  Object* root = objTree_->getRoot();
  uObj = std::unique_ptr<Object>(new Object("Chassis", root));
  ASSERT_TRUE(uObj.get()->getParent() != nullptr);
  // failed since the object has non-null parent
  EXPECT_ANY_THROW(objTree_->addObject(std::move(uObj), "/org"));

  uObj = std::unique_ptr<Object>(new Object("System"));
  Object child("sqlite", uObj.get());
  // failed since the object has non-empty children
  EXPECT_ANY_THROW(objTree_->addObject(std::move(uObj), "/org"));
}

int main (int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  ::google::InitGoogleLogging(argv[0]);

  return RUN_ALL_TESTS();
}
