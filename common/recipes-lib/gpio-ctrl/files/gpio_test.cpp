/*
 * Copyright 2019-present Facebook. All Rights Reserved.
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
#include <cstdlib>
#include <exception>
#include <gtest/gtest.h>
#include "libgpio.hpp"

using namespace std;
using namespace testing;

class GPIOTest : public ::testing::Test {
  void SetUp() {
    ASSERT_EQ(system("rm -rf /tmp/gpionames"), 0);
    ASSERT_EQ(system("rm -rf /tmp/test"), 0);
    ASSERT_EQ(system("mkdir /tmp/test"), 0);
    ASSERT_EQ(system("mkdir /tmp/test/gpio123"), 0);
    ASSERT_EQ(system("echo 0 > /tmp/test/gpio123/value"), 0);
    ASSERT_EQ(system("echo in > /tmp/test/gpio123/direction"), 0);
    ASSERT_EQ(system("echo none > /tmp/test/gpio123/edge"), 0);
    ASSERT_EQ(system("mkdir /tmp/gpionames"), 0);
    ASSERT_EQ(system("ln -s /tmp/test/gpio123 /tmp/gpionames/TEST1"), 0);
  }

  void TearDown() {
    ASSERT_EQ(system("rm -rf /tmp/gpionames"), 0);
    ASSERT_EQ(system("rm -rf /tmp/test"), 0);
  }
};

TEST_F(GPIOTest, getValue) {
  GPIO x("TEST1");
  ASSERT_THROW(x.get_value(), std::system_error);
  x.open();
  ASSERT_EQ(x.get_value(), GPIO_VALUE_LOW);
  ASSERT_EQ(x.get_direction(), GPIO_DIRECTION_IN);
  ASSERT_EQ(x.get_edge(), GPIO_EDGE_NONE);
  x.close();
  ASSERT_THROW(x.get_value(), std::system_error);
  GPIO y("TEST2");
  ASSERT_THROW(y.open(), std::system_error);
}

TEST_F(GPIOTest, setValue) {
  GPIO x("TEST1");
  x.open();
  x.set_value(GPIO_VALUE_HIGH);
  ASSERT_EQ(x.get_value(), GPIO_VALUE_HIGH);
  x.set_direction(GPIO_DIRECTION_OUT);
  ASSERT_EQ(x.get_direction(), GPIO_DIRECTION_OUT);
  x.set_edge(GPIO_EDGE_BOTH);
  ASSERT_EQ(x.get_edge(), GPIO_EDGE_BOTH);
}
