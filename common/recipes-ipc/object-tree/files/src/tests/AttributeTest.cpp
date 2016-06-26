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
#include <gtest/gtest.h>
#include "../Attribute.h"

using namespace openbmc::ipc;

/**
 * Fixture for testing Attribute class
 */
class AttributeTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    a_ = new Attribute("testName", "testType");
  }

  virtual void TearDown() {
    delete a_;
  }

  Attribute* a_;
};

TEST(ConstructorTest, Constructor) {
  EXPECT_ANY_THROW(Attribute attr1(NULL, " "));
  EXPECT_ANY_THROW(Attribute attr2(" ", NULL));
  EXPECT_ANY_THROW(Attribute attr3(NULL, NULL));

  Attribute gattr("genericAttr");
  EXPECT_STREQ(gattr.getName().c_str(), "genericAttr");
  EXPECT_STREQ(gattr.getType().c_str(), "Generic");
  EXPECT_STREQ(gattr.getValue().c_str(), "");
  EXPECT_EQ(gattr.getModes(), Attribute::RO);

  Attribute attr("testName", "testType");
  EXPECT_STREQ(attr.getName().c_str(), "testName");
  EXPECT_STREQ(attr.getType().c_str(), "testType");
  EXPECT_STREQ(attr.getValue().c_str(), "");
  EXPECT_EQ(attr.getModes(), Attribute::RO);
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

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
