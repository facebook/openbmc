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
#include <glog/logging.h>
#include <gtest/gtest.h>
#include "../Object.h"
#include "../Attribute.h"

using namespace openbmc::ipc;

/**
 * Fixture for testing Attribute class
 */
class ObjectTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    obj_ = new Object("root");
  }

  virtual void TearDown() {
    delete obj_;
  }

  Object* obj_;
};

TEST(ConstructorTest, Constructor) {
  Object root("root"); // initialize with parent == nullptr
  EXPECT_STREQ(root.getName().c_str(), "root");
  EXPECT_TRUE(root.getParent() == nullptr);
  EXPECT_EQ(root.getChildMap()->size(), 0);
  EXPECT_EQ(root.getTypeMap()->size(), 0);

  Object obj("Chassis", &root);
  EXPECT_STREQ(obj.getName().c_str(), "Chassis");
  EXPECT_TRUE(obj.getParent() == &root);
  EXPECT_EQ(root.getChildMap()->size(), 1);
  EXPECT_EQ(root.getTypeMap()->size(), 0);
}

TEST_F(ObjectTest, AttributeOperation) {
  EXPECT_EQ(obj_->getTypeMap()->size(), 0);

  ASSERT_TRUE(obj_->addAttribute("1_input", "temp") != nullptr);
  EXPECT_TRUE(obj_->getAttribute("1_input") == nullptr); // not default type
  EXPECT_STREQ(obj_->getAttribute("1_input", "temp")->getName().c_str(),
               "1_input");
  EXPECT_STREQ(obj_->getAttribute("1_input", "temp")->getType().c_str(),
               "temp");
  EXPECT_EQ(obj_->getTypeMap()->size(), 1);
  EXPECT_EQ(obj_->getAttrMap("temp")->size(), 1);
  EXPECT_TRUE(obj_->getAttrMap("Generic") == nullptr);

  obj_->addAttribute("1_input");
  EXPECT_TRUE(obj_->getAttribute("1_input") != nullptr);
  EXPECT_TRUE(obj_->getAttribute("1_input") ==
              obj_->getAttribute("1_input", "Generic"));
  EXPECT_STREQ(obj_->getAttribute("1_input")->getName().c_str(), "1_input");
  EXPECT_STREQ(obj_->getAttribute("1_input")->getType().c_str(), "Generic");
  EXPECT_EQ(obj_->getTypeMap()->size(), 2);
  EXPECT_EQ(obj_->getAttrMap("Generic")->size(), 1);

  obj_->deleteAttribute("1_input", "power"); // wrong type
  EXPECT_TRUE(obj_->getAttribute("1_input") != nullptr);
  EXPECT_TRUE(obj_->getAttribute("1_input", "temp") != nullptr);
  EXPECT_EQ(obj_->getTypeMap()->size(), 2);

  obj_->deleteAttribute("1_input");
  EXPECT_TRUE(obj_->getAttribute("1_input") == nullptr);
  EXPECT_TRUE(obj_->getAttribute("1_input", "temp") != nullptr);
  EXPECT_EQ(obj_->getTypeMap()->size(), 1);
  EXPECT_TRUE(obj_->getAttrMap("Generic") == nullptr);
  EXPECT_EQ(obj_->getAttrMap("temp")->size(), 1);

  obj_->deleteAttribute("1_input", "temp");
  EXPECT_TRUE(obj_->getAttribute("1_input", "temp") == nullptr);
  EXPECT_EQ(obj_->getTypeMap()->size(), 0);
  EXPECT_TRUE(obj_->getAttrMap("temp") == nullptr);
}

TEST_F(ObjectTest, ChildOpertaion) {
  Object child1("child1", obj_);
  EXPECT_EQ(obj_->getChildMap()->size(), 1);
  EXPECT_TRUE(obj_->getChildObject("child1") == &child1);
  EXPECT_TRUE(obj_->getChildObject("child2") == nullptr);

  Object child2("child2");
  obj_->addChildObject(child2);
  EXPECT_EQ(obj_->getChildMap()->size(), 2);
  EXPECT_TRUE(obj_->getChildObject("child2") == &child2);
  EXPECT_TRUE(child2.getParent() == obj_);

  Object child3("child3", &child2);
  obj_->addChildObject(child3); // failed since child3 has non-null parent
  EXPECT_EQ(child2.getChildMap()->size(), 1);
  EXPECT_TRUE(child3.getParent() == &child2);
  EXPECT_TRUE(child2.getChildObject("child3") == &child3);

  child2.removeChildObject("child1"); // failed since no such child object
  EXPECT_EQ(child2.getChildMap()->size(), 1);

  obj_->removeChildObject("child2"); // failed since child2 has non-null children
  EXPECT_TRUE(obj_->getChildObject("child2") == &child2);
  EXPECT_TRUE(child2.getParent() == obj_);
  EXPECT_EQ(child2.getChildMap()->size(), 1);
  EXPECT_EQ(obj_->getChildMap()->size(), 2);

  child2.removeChildObject("child3"); // success
  EXPECT_TRUE(child2.getChildObject("child3") == nullptr);
  EXPECT_EQ(child2.getChildMap()->size(), 0);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ::google::InitGoogleLogging(argv[0]);

  return RUN_ALL_TESTS();
}
