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
#include <glog/logging.h>
#include <nlohmann/json.hpp>
#include "../SensorAttribute.h"
using namespace openbmc::qin;

TEST(SensorAttributeTest, Accessibility) {
  SensorAttribute attr("CPU_temp");
  EXPECT_STREQ(attr.getName().c_str(), "CPU_temp");
  EXPECT_FALSE(attr.isAccessible());
  attr.setAddr("temp1_input");
  EXPECT_TRUE(attr.isAccessible());
  EXPECT_STREQ(attr.getAddr().c_str(), "temp1_input");

  nlohmann::json dump = attr.dumpToJson();
  bool isAccessible = dump.at("isAccessible");
  EXPECT_TRUE(isAccessible);
  const std::string &addr = dump["addr"];
  EXPECT_STREQ(addr.c_str(), "temp1_input");
}

int main (int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  ::google::InitGoogleLogging(argv[0]);

  return RUN_ALL_TESTS();
}
