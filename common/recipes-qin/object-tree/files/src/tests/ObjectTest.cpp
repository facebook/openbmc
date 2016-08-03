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

#include <iostream>
#include <string>
#include <stdexcept>
#include <system_error>
#include <nlohmann/json.hpp>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include "../Object.h"
#include "../Attribute.h"

using namespace openbmc::qin;

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
  EXPECT_EQ(root.getChildMap().size(), 0);

  Object obj("Chassis", &root);
  EXPECT_STREQ(obj.getName().c_str(), "Chassis");
  EXPECT_TRUE(obj.getParent() == &root);
  EXPECT_EQ(root.getChildMap().size(), 1);
}

TEST_F(ObjectTest, AttributeAddDelete) {
  ASSERT_TRUE(obj_->addAttribute("1_input") != nullptr);
  EXPECT_ANY_THROW(obj_->addAttribute("1_input")); // duplicated attribute

  EXPECT_STREQ(obj_->getAttribute("1_input")->getName().c_str(), "1_input");
  EXPECT_EQ(obj_->getAttrMap().size(), 1);

  EXPECT_ANY_THROW(obj_->deleteAttribute("1_inbut")); // wrong name
  EXPECT_TRUE(obj_->getAttribute("1_input") != nullptr);

  obj_->deleteAttribute("1_input");
  EXPECT_TRUE(obj_->getAttribute("1_input") == nullptr);
}

TEST_F(ObjectTest, AttributeReadWrite) {
  Attribute* attr;
  ASSERT_TRUE((attr = obj_->addAttribute("1_input")) != nullptr);

  std::string value = "100";
  attr->setValue(value);
  EXPECT_STREQ(attr->getValue().c_str(), value.c_str());
  EXPECT_STREQ(obj_->readAttrValue("1_input").c_str(), value.c_str());
  EXPECT_THROW(obj_->readAttrValue("1_inbut"), std::invalid_argument);
  // modes is RO. Write is not allowed.
  EXPECT_THROW(obj_->writeAttrValue("1_input", "200"), std::system_error);
  EXPECT_STREQ(attr->getValue().c_str(), value.c_str());

  attr->setModes(Attribute::WO);
  // modes is WO. Read is not allowed.
  EXPECT_THROW(obj_->readAttrValue("1_input"), std::system_error);
  value = "200";
  EXPECT_NO_THROW(obj_->writeAttrValue("1_input", value));
  EXPECT_THROW(obj_->writeAttrValue("1", value), std::invalid_argument);
  EXPECT_STREQ(attr->getValue().c_str(), value.c_str());

  attr->setModes(Attribute::RW);
  value = "100";
  EXPECT_NO_THROW(obj_->writeAttrValue("1_input", value));
  EXPECT_STREQ(obj_->readAttrValue("1_input").c_str(), value.c_str());
}

TEST_F(ObjectTest, ChildOpertaion) {
  Object child1("child1", obj_);
  EXPECT_EQ(obj_->getChildMap().size(), 1);
  EXPECT_TRUE(obj_->getChildObject("child1") == &child1);
  EXPECT_TRUE(obj_->getChildObject("child2") == nullptr);

  Object child2("child2");
  obj_->addChildObject(child2);
  EXPECT_EQ(obj_->getChildMap().size(), 2);
  EXPECT_TRUE(obj_->getChildObject("child2") == &child2);
  EXPECT_TRUE(child2.getParent() == obj_);

  Object dChild2("child2");
  // failed since adding duplicated child
  EXPECT_ANY_THROW(obj_->addChildObject(dChild2));
  // failed since child1 has non-null parent
  EXPECT_ANY_THROW(dChild2.addChildObject(child1));

  Object child3("child3", &child2);
  // failed since child3 has non-null parent
  EXPECT_ANY_THROW(obj_->addChildObject(child3));
  EXPECT_EQ(child2.getChildMap().size(), 1);
  EXPECT_TRUE(child3.getParent() == &child2);
  EXPECT_TRUE(child2.getChildObject("child3") == &child3);

  // failed since no such child object
  EXPECT_ANY_THROW(child2.removeChildObject("child1"));
  EXPECT_EQ(child2.getChildMap().size(), 1);

  // failed since child2 has non-null children
  EXPECT_ANY_THROW(obj_->removeChildObject("child2"));
  EXPECT_TRUE(obj_->getChildObject("child2") == &child2);
  EXPECT_TRUE(child2.getParent() == obj_);
  EXPECT_EQ(child2.getChildMap().size(), 1);
  EXPECT_EQ(obj_->getChildMap().size(), 2);

  child2.removeChildObject("child3"); // success
  EXPECT_TRUE(child2.getChildObject("child3") == nullptr);
  EXPECT_EQ(child2.getChildMap().size(), 0);
}

TEST_F(ObjectTest, DumpToJson) {
  Object child1("child1", obj_);
  Object child2("child2", obj_);
  obj_->addAttribute("temp1_input");
  obj_->addAttribute("power1_input");
  obj_->addAttribute("fan1_intput");

  nlohmann::json objectInfo = obj_->dumpToJson();
  std::cout << objectInfo.dump(2) << std::endl;

  const std::string &objectName = objectInfo.at("objectName");
  const std::string &objectType = objectInfo.at("objectType");
  EXPECT_STREQ(objectName.c_str(), "root");
  EXPECT_STREQ(objectType.c_str(), "Generic");
  EXPECT_EQ(objectInfo.at("childObjectNames").size(), 2);
  EXPECT_EQ(objectInfo.at("attributes").size(), 3);
  EXPECT_TRUE(objectInfo.at("parentName").is_null());

  std::cout << obj_->dumpToJsonRecursive().dump(2) << std::endl;
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ::google::InitGoogleLogging(argv[0]);

  return RUN_ALL_TESTS();
}
