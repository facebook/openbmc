/*
 * FRUTest.cpp: Unit tests for FRU class
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
#include "../FRU.h"
#include "../HotPlugDetectionViaPath.h"
#include "HotPlugDetectionFile.h"

using namespace openbmc::qin;

TEST(FRUTest, NoHotPlugSupportTest) {
  //Create FRU object, fru doesnot support hot plug
  std::string fruName = "mb";
  std::string fruJson = "{}";
  FRU fru(fruName, nullptr, fruJson);

  //Check if fruName set correctly
  ASSERT_EQ(fru.getName(), fruName);
  //Check if fruJson set correctly
  ASSERT_EQ(fru.getFruJson(), fruJson);
  //by default fru should be available
  ASSERT_TRUE(fru.isAvailable());
  //fru does not support hot plug
  ASSERT_FALSE(fru.isHotPlugSupported());
}

TEST(FRUTest, HotPlugExternalDetection) {
  //Create FRU object, fru supports external hotplug detection.
  FRU fru("", nullptr, "", nullptr);
  //by default fru is unavailable
  ASSERT_FALSE(fru.isAvailable());
  //fru supports hotplug
  ASSERT_TRUE(fru.isHotPlugSupported());
  //fru supports external hotplug detection
  ASSERT_FALSE(fru.isIntHPDetectionSupported());
  //Set fru available, operation should be successful
  ASSERT_TRUE(fru.setAvailable(true));
  //check if fru available
  ASSERT_TRUE(fru.isAvailable());
  //set fru unavailable, operation should be successful
  ASSERT_TRUE(fru.setAvailable(false));
  //check if fru available
  ASSERT_FALSE(fru.isAvailable());
}

TEST(FRUTest, HotPlugInternalDetectionViaPath) {
  //Create file for hotplug detection
  HotPlugDetectionFile file("/tmp/hpDetectViaPathTest");
  std::unique_ptr<HotPlugDetectionViaPath>
             hpDetect(new HotPlugDetectionViaPath(file.getFileName()));
  //Create FRU object, fru supports internal hotplug detection
  FRU fru("", nullptr, "", std::move(hpDetect));
  //by default fru is unavailable
  ASSERT_FALSE(fru.isAvailable());
  //fru supports hotplug
  ASSERT_TRUE(fru.isHotPlugSupported());
  //fru supports internal hotplug detection
  ASSERT_TRUE(fru.isIntHPDetectionSupported());
  //Set fru available, operation should be unsuccessful
  ASSERT_FALSE(fru.setAvailable(true));
  //fru availability should not change
  ASSERT_FALSE(fru.isAvailable());

  //Call internal hotplug detection method, should return false
  ASSERT_FALSE(fru.detectAvailability());

  //write fru status to file
  file.writeHotPlugStatusToFile(1);
  //Call internal hotplug detection method
  ASSERT_TRUE(fru.detectAvailability());
  //check if fru available
  ASSERT_TRUE(fru.isAvailable());

  //Set fru available, operation should be unsuccessful
  ASSERT_FALSE(fru.setAvailable(false));
  //fru availability should not change
  ASSERT_TRUE(fru.isAvailable());

  //write fru status to file
  file.writeHotPlugStatusToFile(0);
  //Call internal hotplug detection method
  ASSERT_FALSE(fru.detectAvailability());
  //check if fru available
  ASSERT_FALSE(fru.isAvailable());
}

int main (int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  ::google::InitGoogleLogging(argv[0]);

  return RUN_ALL_TESTS();
}
