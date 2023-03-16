#### bic-support

- [ ] P0 **bic gpio support**
- [ ] P0 **bic i3c support**
- [ ] P1 **bic info**
- [ ] P0 **bic ipmb support**
- [ ] P0 **bic mctp support**
- [ ] P0 **bic reset**
- [ ] P2 **bic secure boot**
- [ ] P0 **bic self test**
- [ ] P0 **bic slot clear cmos support**
- [ ] P0 **bic slot postcode data**
- [ ] P1 **bic slot sdr data**
- [ ] P0 **bic slot sensor data**
- [ ] P1 **bic status check**
- [ ] P2 **bic usb connection status**

#### bmc-hardware-support

- [ ] P0 **aspeed**
- [ ] P2 **nuvoton**

#### bmc-reset

- [ ] P2 **bmc reset host side**
- [ ] P2 **bmc reset recovery redfish**
- [ ] P0 **bmc reset recovery ssh**

#### configuration

- [ ] P1 **configuration cmos restore oem command**
- [ ] P0 **configuration cmos restore physical reset**
- [ ] P2 **configuration get platform info**
- [ ] P0 **configuration set boot order sequence**
- [ ] P1 **configuration set power policy**

#### design-for-test

- [ ] P0 **dft asd jumper detection**
- [ ] P2 **dft bmc network recovery**
- [ ] P3 **dft firmware downgrade**
- [ ] P2 **dft firmware version reporting**
- [ ] P2 **dft firmware wdt auto recovery**
- [ ] P2 **dft nic reseat handling**
- [ ] P2 **dft power on error handling**

#### diagnostics

- [ ] P2 **diagnostics crashdump auto**
- [ ] P0 **diagnostics crashdump manual**
- [ ] P2 **diagnostics crashdump prevent power off**
- [ ] P3 **diagnostics memory post package repair**

#### event-logs

- [ ] P1 **log events to persistent storage**

#### firmware-signature-checking

- [ ] P2 **bic binary signature check**
- [ ] P2 **bios signature check**
- [ ] P2 **expansion card binary signature check**

#### firmware-update

- [ ] P0 **firmware update bic**
- [ ] P0 **firmware update bic recovery**
- [ ] P0 **firmware update bios**
- [ ] P0 **firmware update cpld**
- [ ] P2 **firmware update expansion bic**
- [ ] P2 **firmware update expansion bic recovery**
- [ ] P3 **firmware update flashy**
- [ ] P1 **firmware update**
- [ ] P1 **firmware update nic**
- [ ] P0 **firmware update openbmc**
- [ ] P0 **firmware update vr**
- [ ] P4 **wipe workflows**

#### firmware-versions

- [ ] P0 **firmware version bic**
- [ ] P0 **firmware version bios**
- [ ] P0 **firmware version cpld**
- [ ] P2 **firmware version expansion card**
- [ ] P1 **firmware version nic**
- [ ] P0 **firmware version openbmc**
- [ ] P0 **firmware versions**
- [ ] P0 **firmware version vr**

#### front-panel-control

- [ ] P2 **fpc console selection switch**
- [ ] P2 **fpc debug card console**
- [ ] P2 **fpc debug card reset button**
- [ ] P0 **fpc heartbeat led**
- [ ] P2 **fpc identity led**
- [ ] P0 **fpc sled cycle**

#### fru-information

- [ ] P2 **fru information**
- [ ] P1 **fru read fru**
- [ ] P1 **fru update fru**

#### health-monitoring

- [ ] P2 **health monitoring configuration**
- [ ] P2 **health monitoring cpu usage**
- [ ] P1 **health monitoring disable auto reboots**
- [ ] P2 **health monitoring memory usage**
- [ ] P0 **health monitoring wdt**

#### hot-plug-services

- [ ] P1 **hot plug service fan speed**
- [ ] P2 **hot plug service sdr caching**
- [ ] P2 **hot plug service sled insertion**

#### infrastructure

- [ ] P0 **adc device support**
- [ ] P2 **cpld gpio expander register layout**
- [ ] P0 **flash layout**
- [ ] P0 **gpio device support**
- [ ] P0 **gpio driver porting**
- [ ] P0 **i2c device support**
- [ ] P0 **kernel**
- [ ] P0 **pwm tach device support**
- [ ] P1 **qemu**
- [ ] P0 **spi device support**
- [ ] P0 **u boot porting**
- [ ] P0 **yocto**

#### lcd-debug-card

- [ ] P2 **lcd debug card critical log frame**
- [ ] P2 **lcd debug card critical sensor frame**
- [ ] P2 **lcd debug card gpio**
- [ ] P2 **lcd debug card memory loop code**
- [ ] P2 **lcd debug card post**
- [ ] P2 **lcd debug card user settings**

#### miscellaneous

- [ ] P2 **misc dimm location matching**
- [ ] P2 **misc enclosure util**
- [ ] P1 **misc guid util**
- [ ] P1 **misc json format**
- [ ] P4 **misc ods scuba integration**
- [ ] P4 **misc ods sensor names**
- [ ] P2 **misc openbmc test automation**
- [ ] P0 **misc reset default factory settings**

#### network

- [ ] P0 **nic host to bmc support**
- [ ] P1 **nic ipv4**
- [ ] P0 **nic ipv6**
- [ ] P1 **nic monitoring and upgrade**
- [ ] P2 **nic multi channel support**
- [ ] P2 **nic ncsi aen notifications**
- [ ] P2 **nic network link status detection**
- [ ] P0 **nic ocpv3 support**

#### pldm

- [ ] P2 **pldm firmware update**
- [ ] P2 **pldm nic sensor info**

#### post-code

- [ ] P1 **post code history**

#### power-control

- [ ] P1 **expansion card power cycle**
- [ ] P1 **expansion card power off**
- [ ] P1 **expansion card power on**
- [ ] P1 **phosphor state manager support**
- [ ] P2 **power failure detection**
- [ ] P0 **power status**
- [ ] P0 **sled cycle**
- [ ] P0 **slot 12v cycle**
- [ ] P0 **slot 12v off**
- [ ] P0 **slot 12v on**
- [ ] P0 **slot cycle**
- [ ] P2 **slot graceful shutdown**
- [ ] P0 **slot power off**
- [ ] P0 **slot power on**
- [ ] P0 **slot reset**

#### security

- [ ] P1 **prot support**
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
- [ ] P2 **sensor caching**
- [ ] P1 **sensor common naming spec**
- [ ] P2 **sensor display current status**
- [ ] P2 **sensor display thresholds**
- [ ] P3 **sensor history**
- [ ] P0 **sensor information**

#### serial-over-lan

- [ ] P0 **bic uart access**
- [ ] P1 **expansion card uart access**
- [ ] P0 **host sol access**
- [ ] P0 **m term**
- [ ] P1 **sol console history**
- [ ] P1 **sol host logger**

#### thermal-management

- [ ] P0 **airflow sensor**
- [ ] P3 **conditional sensor calibration**
- [ ] P2 **cpu throttle support**
- [ ] P1 **fan control automatic**
- [ ] P0 **fan control manual**
- [ ] P2 **fsc configuration update**
- [ ] P2 **fsc persistent configuration**
- [ ] P1 **nic otp protection**
- [ ] P2 **sensor calibration**
- [ ] P1 **sled otp protection**
- [ ] P3 **thermal management**

#### time-sync

- [ ] P2 **time ntp sync**
- [ ] P1 **time rtc**
- [ ] P0 **time timezone**

#### user-interface

- [ ] P0 **generic user interface**
- [ ] P2 **host2bmc usb**
- [ ] P0 **inband ethernet**
- [ ] P1 **inband kcs**
- [ ] P2 **oob rest**
- [ ] P0 **oob ssh**
- [ ] P2 **redfish**

#### intel

- [ ] P0 **diagnostics at scale debug**

#### platform-specific-requirements

- [ ] P2 **sled cable presence detect**
- [ ] P2 **ddr5 pmic power measurement**
- [ ] P2 **ddr5 pmic error injection**
- [ ] P3 **terminus nic enablement**
- [ ] P2 **platform root of trust support**
- [ ] P2 **cpld board id version info**
- [ ] P2 **dimm spd data**
- [ ] P2 **ti vr controller fw update cycle counter**
