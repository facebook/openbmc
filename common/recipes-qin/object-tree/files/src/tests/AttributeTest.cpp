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
#include <gtest/gtest.h>
#include <glog/logging.h>
#include "../Attribute.h"

using namespace openbmc::qin;

/**
 * Fixture for testing Attribute class
 */
class AttributeTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    a_ = new Attribute("testName");
  }

  virtual void TearDown() {
    delete a_;
  }

  Attribute* a_;
};

TEST(ConstructorTest, Constructor) {
  EXPECT_ANY_THROW(Attribute attr1(NULL));

  Attribute gattr("genericAttr");
  EXPECT_STREQ(gattr.getName().c_str(), "genericAttr");
  EXPECT_STREQ(gattr.getValue().c_str(), "");
  EXPECT_EQ(gattr.getModes(), Attribute::RO);
}

TEST_F(AttributeTest, SetValue) {
  EXPECT_ANY_THROW(a_->setValue(NULL));
  EXPECT_STREQ(a_->getValue().c_str(), "");

  a_->setValue("1991");
  EXPECT_STREQ(a_->getValue().c_str(), "1991");
}

TEST_F(AttributeTest, SetModes) {
  EXPECT_EQ(a_->getModes(), Attribute::RO);

  a_->setModes(Attribute::WO);
  EXPECT_EQ(a_->getModes(), Attribute::WO);

  a_->setModes(Attribute::RW);
  EXPECT_EQ(a_->getModes(), Attribute::RW);
}

TEST_F(AttributeTest, IsReadableWritable) {
  EXPECT_EQ(a_->getModes(), Attribute::RO);
  EXPECT_TRUE(a_->isReadable());
  EXPECT_FALSE(a_->isWritable());

  a_->setModes(Attribute::WO);
  EXPECT_EQ(a_->getModes(), Attribute::WO);
  EXPECT_FALSE(a_->isReadable());
  EXPECT_TRUE(a_->isWritable());

  a_->setModes(Attribute::RW);
  EXPECT_EQ(a_->getModes(), Attribute::RW);
  EXPECT_TRUE(a_->isReadable());
  EXPECT_TRUE(a_->isWritable());
}

TEST_F(AttributeTest, Dump) {
  a_->setValue("100");
  a_->setModes(Attribute::RW);
  nlohmann::json attrInfo = a_->dumpToJson();

  std::cout << attrInfo.dump(2) << std::endl;

  const std::string &name = attrInfo.at("name");
  const std::string &value = attrInfo.at("value");
  const std::string &modes = attrInfo.at("modes");
  EXPECT_STREQ(name.c_str(), "testName");
  EXPECT_STREQ(value.c_str(), "100");
  EXPECT_STREQ(modes.c_str(), "RW");
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ::google::InitGoogleLogging(argv[0]);

  return RUN_ALL_TESTS();
}
