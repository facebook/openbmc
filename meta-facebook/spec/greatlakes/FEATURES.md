#### bic-support

- [X] P0 **bic gpio support**
- [ ] P0 **bic i3c support**
- [X] P1 **bic info**
- [X] P0 **bic ipmb support**
- [ ] P0 **bic mctp support**
- [X] P0 **bic reset**
- ~~[ ] P2 **bic secure boot**~~ (Not support this feature)
- [X] P0 **bic self test**
- ~~[ ] P0 **bic slot clear cmos support**~~ (Not support this feature)
- [X] P0 **bic slot postcode data**
- [X] P1 **bic slot sdr data**
- [X] P0 **bic slot sensor data**
- [X] P1 **bic status check**
- [X] P2 **bic usb connection status**

#### bmc-hardware-support

- [X] P0 **aspeed**
- ~~[ ] P2 **nuvoton**~~ (Not support this feature)

#### bmc-reset

- [ ] P2 **bmc reset host side**
- [ ] P2 **bmc reset recovery redfish**
- [X] P0 **bmc reset recovery ssh**

#### configuration

- [ ] P1 **configuration cmos restore oem command**
- [X] P0 **configuration cmos restore physical reset**
- [X] P2 **configuration get platform info**
- [X] P0 **configuration set boot order sequence**
- [X] P1 **configuration set power policy**

#### design-for-test

- [ ] P0 **dft asd jumper detection**
- [ ] P2 **dft bmc network recovery**
- [X] P3 **dft firmware downgrade**
- [X] P2 **dft firmware version reporting**
- [X] P2 **dft firmware wdt auto recovery**
- [X] P2 **dft nic reseat handling**
- [X] P2 **dft power on error handling**

#### diagnostics

- [ ] P2 **diagnostics crashdump auto**
- [ ] P0 **diagnostics crashdump manual**
- [ ] P2 **diagnostics crashdump prevent power off**
- [ ] P3 **diagnostics memory post package repair**

#### event-logs

- [X] P1 **log events to persistent storage**

#### firmware-signature-checking

- [ ] P2 **bic binary signature check**
- [ ] P2 **bios signature check**
- [ ] P2 **expansion card binary signature check**

#### firmware-update

- [X] P0 **firmware update bic**
- [X] P0 **firmware update bic recovery**
- [X] P0 **firmware update bios**
- [X] P0 **firmware update cpld**
- [X] P2 **firmware update expansion bic**
- [X] P2 **firmware update expansion bic recovery**
- [ ] P3 **firmware update flashy**
- [X] P1 **firmware update**
- [X] P1 **firmware update nic**
- [X] P0 **firmware update openbmc**
- [X] P0 **firmware update vr**
- [ ] P4 **wipe workflows**

#### firmware-versions

- [X] P0 **firmware version bic**
- [X] P0 **firmware version bios**
- [X] P0 **firmware version cpld**
- [X] P2 **firmware version expansion card**
- [X] P1 **firmware version nic**
- [X] P0 **firmware version openbmc**
- [X] P0 **firmware versions**
- [X] P0 **firmware version vr**

#### front-panel-control

- [ ] P2 **fpc console selection switch**
- [ ] P2 **fpc debug card console**
- [ ] P2 **fpc debug card reset button**
- [X] P0 **fpc heartbeat led**
- [X] P2 **fpc identity led**
- [ ] P0 **fpc sled cycle**

#### fru-information

- [X] P2 **fru information**
- [X] P1 **fru read fru**
- [X] P1 **fru update fru**

#### health-monitoring

- [X] P2 **health monitoring configuration**
- [X] P2 **health monitoring cpu usage**
- [ ] P1 **health monitoring disable auto reboots**
- [X] P2 **health monitoring memory usage**
- [X] P0 **health monitoring wdt**

#### hot-plug-services

- [X] P1 **hot plug service fan speed**
- [X] P2 **hot plug service sdr caching**
- [X] P2 **hot plug service sled insertion**

#### infrastructure

- [X] P0 **adc device support**
- [X] P2 **cpld gpio expander register layout (access through I2C raw command)**
- [X] P0 **flash layout**
- [X] P0 **gpio device support**
- [X] P0 **gpio driver porting**
- [X] P0 **i2c device support**
- [X] P0 **kernel**
- [X] P0 **pwm tach device support**
- ~~[ ] P1 **qemu**~~ (Not support this feature)
- [X] P0 **spi device support**
- [X] P0 **u boot porting**
- [X] P0 **yocto**

#### lcd-debug-card

- [X] P2 **lcd debug card critical log frame**
- [X] P2 **lcd debug card critical sensor frame**
- [X] P2 **lcd debug card gpio**
- [ ] P2 **lcd debug card memory loop code**
- [X] P2 **lcd debug card post**
- [X] P2 **lcd debug card user settings**

#### miscellaneous

- [ ] P2 **misc dimm location matching**
- ~~[ ] P2 **misc enclosure util**~~ (Not support this feature)
- [X] P1 **misc guid util**
- [ ] P1 **misc json format**
- [ ] P4 **misc ods scuba integration**
- [ ] P4 **misc ods sensor names**
- ~~[ ] P2 **misc openbmc test automation**~~ (Not support this feature)
- [X] P0 **misc reset default factory settings**

#### network

- [X] P0 **nic host to bmc support**
- [X] P1 **nic ipv4**
- [X] P0 **nic ipv6**
- [X] P1 **nic monitoring and upgrade**
- [ ] P2 **nic multi channel support**
- [ ] P2 **nic ncsi aen notifications**
- [ ] P2 **nic network link status detection**
- [X] P0 **nic ocpv3 support**

#### pldm

- [ ] P2 **pldm firmware update**
- [ ] P2 **pldm nic sensor info**

#### post-code

- [X] P1 **post code history**

#### power-control

- ~~[ ] P1 **expansion card power cycle**~~ (Not support this feature)
- ~~[ ] P1 **expansion card power off**~~ (Not support this feature)
- ~~[ ] P1 **expansion card power on**~~ (Not support this feature)
- ~~[ ] P1 **phosphor state manager support**~~ (Not support this feature)
- [X] P2 **power failure detection**
- [X] P0 **power status**
- [X] P0 **sled cycle**
- [X] P0 **slot 12v cycle**
- [X] P0 **slot 12v off**
- [X] P0 **slot 12v on**
- [X] P0 **slot cycle**
- [X] P2 **slot graceful shutdown**
- [X] P0 **slot power off**
- [X] P0 **slot power on**
- [X] P0 **slot reset**

#### security

- ~~[ ] P1 **prot support**~~ (Not support this feature)
- [ ] P3 **security attest util**
- [ ] P2 **security signed firmware update**
- [ ] P2 **tpm 20 boot measurement**
- [ ] P2 **tpm 20 test**
- [ ] P1 **verified boot dual flash**
- [ ] P0 **verified boot general**
- [ ] P3 **verified boot hw enforcement**
- [ ] P3 **verified boot image signing**
- [ ] P2 **verified boot image verification**
- [ ] P2 **verified boot recovery**
- [ ] P2 **verified boot sw enforcement**
- [ ] P2 **verified boot tpm integration**

#### sensors

- [ ] P3 **sensor automatic snapshot**
- [X] P2 **sensor caching**
- [X] P1 **sensor common naming spec**
- [X] P2 **sensor display current status**
- [X] P2 **sensor display thresholds**
- [X] P3 **sensor history**
- [X] P0 **sensor information**

#### serial-over-lan

- [X] P0 **bic uart access**
- [X] P1 **expansion card uart access**
- [X] P0 **host sol access**
- [X] P0 **m term**
- [X] P1 **sol console history**
- ~~[ ] P1 **sol host logger**~~ (Not support this feature)

#### thermal-management

- [X] P0 **airflow sensor**
- [ ] P3 **conditional sensor calibration**
- [ ] P2 **cpu throttle support**
- [X] P1 **fan control automatic**
- [X] P0 **fan control manual**
- [ ] P2 **fsc configuration update**
- [ ] P2 **fsc persistent configuration**
- [X] P1 **nic otp protection**
- [ ] P2 **sensor calibration**
- [X] P1 **sled otp protection**
- [X] P3 **thermal management**

#### time-sync

- [X] P2 **time ntp sync**
- [ ] P1 **time rtc**
- [X] P0 **time timezone**

#### user-interface

- [X] P0 **generic user interface**
- ~~[ ] P2 **host2bmc usb**~~ (Not support this feature)
- [X] P0 **inband ethernet**
- [X] P1 **inband kcs**
- [ ] P2 **oob rest**
- [X] P0 **oob ssh**
- [ ] P2 **redfish**

#### intel

- [ ] P0 **diagnostics at scale debug**

#### platform-specific-requirements

- [X] P2 **sled cable presence detect**
- [ ] P2 **ddr5 pmic power measurement**
- [ ] P2 **ddr5 pmic error injection**
- [ ] P3 **terminus nic enablement**
- ~~[ ] P2 **platform root of trust support**~~ (Not support this feature)
- [X] P2 **cpld board id version info**
- [ ] P2 **dimm spd data**
- [X] P2 **mps vr controller fw update cycle counter**

