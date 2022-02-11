/*
 * PlatformObjectTreeTest.cpp: Unit tests for PlatformObjectTree
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
#include <string>
#include <dbus-utils/DBus.h>
#include "../PlatformObjectTree.h"
#include "../PlatformJsonParser.h"
#include "HotPlugDetectionFile.h"

using namespace openbmc::qin;

class PlatformObjectTreeTest : public ::testing::Test {
  protected:
    const std::string jsonFile = "/etc/test-platform-svcd-config.json";
    const std::string sensorServiceDBusName = "org.openbmc.SensorService";
    const std::string sensorServiceDBusPath = "/org/openbmc/SensorService";
    const std::string fruServiceDBusName = "org.openbmc.FruService";
    const std::string fruServiceDBusPath = "/org/openbmc/FruService";
    std::unique_ptr<PlatformObjectTree> platformTree;
    std::shared_ptr<DBus> sDbus;

    virtual void SetUp() {
      //Create platform tree from json
      sDbus = std::shared_ptr<DBus>(new DBus("org.openbmc.test", nullptr));
      platformTree = std::unique_ptr<PlatformObjectTree>(
                       new PlatformObjectTree(sDbus, "org"));
      platformTree->addObject("openbmc","/org");
      PlatformJsonParser::parse(jsonFile, *platformTree, "/org/openbmc");
    }
};

//Test to verify if platform tree is rightly constructed based on json file
TEST_F(PlatformObjectTreeTest, JsonParsingVarification) {
  PlatformService* platformService;
  FRU* fru;

  //Verify if PlatformService object is created at /org/openbmc/PlatformService
  platformService = dynamic_cast<PlatformService*>(
                      platformTree->getObject("/org/openbmc/PlatformService"));
  ASSERT_NE(platformService, nullptr);
  ASSERT_EQ(platformService->getChildMap().size(), 3);

  //Verify if FRU mb is created at "/org/openbmc/PlatformService/mb"
  fru = dynamic_cast<FRU*>(
                  platformTree->getObject("/org/openbmc/PlatformService/mb"));
  ASSERT_NE(fru, nullptr);
  //mb should be available by default
  ASSERT_TRUE(fru->isAvailable());
  //mb does not support hotplug
  ASSERT_FALSE(fru->isHotPlugSupported());
  //Verify mb's child objects
  ASSERT_EQ(fru->getChildMap().size(), 5);
  ASSERT_NE(dynamic_cast<Sensor*>(
              platformTree->getObject(
                "/org/openbmc/PlatformService/mb/MB_SENSOR_INLET_TEMP")),
            nullptr);
  ASSERT_NE(dynamic_cast<Sensor*>(
              platformTree->getObject(
                "/org/openbmc/PlatformService/mb/MB_SENSOR_OUTLET_TEMP")),
            nullptr);
  ASSERT_NE(dynamic_cast<Sensor*>(
              platformTree->getObject(
                "/org/openbmc/PlatformService/mb/MB_SENSOR_C2_AVA_FTEMP")),
            nullptr);
  ASSERT_NE(dynamic_cast<Sensor*>(
              platformTree->getObject(
                "/org/openbmc/PlatformService/mb/MB_SENSOR_C2_1_NVME_CTEMP")),
            nullptr);
  ASSERT_NE(dynamic_cast<Sensor*>(
              platformTree->getObject(
                "/org/openbmc/PlatformService/"
                "mb/MB_SENSOR_VR_CPU0_VCCIN_TEMP")),
            nullptr);

  //Verify if FRU nic is created at "/org/openbmc/PlatformService/nic"
  fru = dynamic_cast<FRU*>(
              platformTree->getObject("/org/openbmc/PlatformService/nic"));
  ASSERT_NE(fru, nullptr);
  //nic should be available by default
  ASSERT_TRUE(fru->isAvailable());
  //nic does not support hotplug
  ASSERT_FALSE(fru->isHotPlugSupported());
  //Verify nic's child object
  ASSERT_EQ(fru->getChildMap().size(), 1);
  ASSERT_NE(dynamic_cast<Sensor*>(
              platformTree->getObject(
                "/org/openbmc/PlatformService/nic/MEZZ_SENSOR_TEMP")),
            nullptr);

  //Verify if FRU IntDetectExampleFRU is created
  //at "/org/openbmc/PlatformService/IntDetectExampleFRU"
  fru = dynamic_cast<FRU*>(
              platformTree->getObject(
                "/org/openbmc/PlatformService/IntDetectExampleFRU"));
  ASSERT_NE(fru, nullptr);
  //IntDetectExampleFRU should be unavailable by default
  ASSERT_FALSE(fru->isAvailable());
  //IntDetectExampleFRU should support hotplug
  ASSERT_TRUE(fru->isHotPlugSupported());
  //IntDetectExampleFRU should support internal hotplug detection
  ASSERT_TRUE(fru->isIntHPDetectionSupported());
  //Verify IntDetectExampleFRU's child objects
  ASSERT_EQ(fru->getChildMap().size(), 2);
  ASSERT_NE(dynamic_cast<Sensor*>(
              platformTree->getObject(
                "/org/openbmc/PlatformService/"
                "IntDetectExampleFRU/HOT_PLUG_SENSOR1")),
            nullptr);

  //Verify if FRU ExtDetectExampleFRU is created
  //at "/org/openbmc/PlatformService/IntDetectExampleFRU/ExtDetectExampleFRU"
  fru = dynamic_cast<FRU*>(
              platformTree->getObject(
                "/org/openbmc/PlatformService/"
                "IntDetectExampleFRU/ExtDetectExampleFRU"));
  ASSERT_NE(fru, nullptr);
  //ExtDetectExampleFRU should be unavailable by default
  ASSERT_FALSE(fru->isAvailable());
  //ExtDetectExampleFRU should support hotplug
  ASSERT_TRUE(fru->isHotPlugSupported());
  //ExtDetectExampleFRU should support external hotplug detection
  ASSERT_FALSE(fru->isIntHPDetectionSupported());
  //Verify ExtDetectExampleFRU's child objects
  ASSERT_EQ(fru->getChildMap().size(), 1);
  ASSERT_NE(dynamic_cast<Sensor*>(
              platformTree->getObject(
                "/org/openbmc/PlatformService/"
                "IntDetectExampleFRU/ExtDetectExampleFRU/HOT_PLUG_SENSOR2")),
            nullptr);

  //Verify Sensor Service dbus name and dbus path
  ASSERT_EQ(platformTree->getSensorService()->getDBusName(),
            sensorServiceDBusName);
  ASSERT_EQ(platformTree->getSensorService()->getDBusPath(),
            sensorServiceDBusPath);

  //Verify FRU Service dbus name and dbus path
  ASSERT_EQ(platformTree->getFruService()->getDBusName(), fruServiceDBusName);
  ASSERT_EQ(platformTree->getFruService()->getDBusPath(), fruServiceDBusPath);
}

//Test to verify platform tree hot plug api
TEST_F(PlatformObjectTreeTest, HotPlugTest) {
  FRU* fru;

  //Only IntDetectExampleFRU supports internal hot plug detection
  ASSERT_EQ(platformTree->getNofHPIntDetectSupportedFrus(), 1);

  //Create empty file for IntDetectExampleFRU
  HotPlugDetectionFile file("/tmp/hpDetectViaPathTest");
  fru = dynamic_cast<FRU*>(
              platformTree->getObject(
                "/org/openbmc/PlatformService/IntDetectExampleFRU"));

  //Check for hotplug supported frus
  platformTree->checkHotPlugSupportedFrus();
  ASSERT_FALSE(fru->isAvailable());

  //write fru available to file
  file.writeHotPlugStatusToFile(1);
  platformTree->checkHotPlugSupportedFrus();
  ASSERT_TRUE(fru->isAvailable());

  //write fru unavailable to file
  file.writeHotPlugStatusToFile(0);
  platformTree->checkHotPlugSupportedFrus();
  ASSERT_FALSE(fru->isAvailable());

  //ExtDetectExampleFRU supports external hotplug detection
  fru = dynamic_cast<FRU*>(
              platformTree->getObject(
                "/org/openbmc/PlatformService/"
                "IntDetectExampleFRU/ExtDetectExampleFRU"));
  ASSERT_NE(fru, nullptr);
  //Set Fru available
  ASSERT_TRUE(platformTree->setFruAvailable(
                "/org/openbmc/PlatformService/"
                "IntDetectExampleFRU/ExtDetectExampleFRU", TRUE));
  ASSERT_TRUE(fru->isAvailable());
  //Set Fru available
  ASSERT_TRUE(platformTree->setFruAvailable(
                "/org/openbmc/PlatformService/"
                "IntDetectExampleFRU/ExtDetectExampleFRU", FALSE));
  ASSERT_FALSE(fru->isAvailable());
}

int main (int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  ::google::InitGoogleLogging(argv[0]);

  return RUN_ALL_TESTS();
}
