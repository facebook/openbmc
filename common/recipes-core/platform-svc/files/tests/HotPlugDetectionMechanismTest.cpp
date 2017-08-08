/*
 * HotPlugDetectionMechanismTest.cpp : Unit tests for Hot Plug Detection
 *                                     mechanisms
 *
 * Copyright 2017-present Facebook. All Rights Reserved.
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
#include "../HotPlugDetectionViaPath.h"
#include "HotPlugDetectionFile.h"

using namespace openbmc::qin;

TEST(HotPlugDetectionMechanismTest, HotPlugDetectionViaPathTest) {
  HotPlugDetectionFile file("/tmp/hpDetectViaPathTest");
  HotPlugDetectionViaPath hpDetect(file.getFileName());

  //Empty file, detectAvailability should return false
  ASSERT_FALSE(hpDetect.detectAvailability());

  //write fru status to file
  file.writeHotPlugStatusToFile(1);
  //detectAvailability should return true
  ASSERT_TRUE(hpDetect.detectAvailability());

  //write fru status to file
  file.writeHotPlugStatusToFile(0);
  //detectAvailability should return false
  ASSERT_FALSE(hpDetect.detectAvailability());
}

int main (int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  ::google::InitGoogleLogging(argv[0]);

  return RUN_ALL_TESTS();
}
