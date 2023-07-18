
# Time synchronization mechanism

## Abstract

This topic mainly discusses the time synchronization mechanism between BMC and BIOS under the architecture of YV4. 

## Description
In YV4 hardware design, server board doesn't have battery for maintaining persistent time in AMD internal RTC.
The block diagram is as follow:
### Block diagram
``` mermaid
graph LR;
subgraph  management board;
BMC---exRTC[external RTC]
exRTC---CMOS
end;
subgraph  serverboard board;
subgraph CPU;
InRTC[internal RTC] --- BIOS
BIOS
end
OS[Host OS] --- InRTC
BIOS --- BIC
end
BIC;
BIOS---OS
OS---NTP[NTP server]
BMC---NTP
BIC--PLDM/IPMI---BMC
```
To ensure accurate timekeeping and synchronization between the BMC, BIOS and host OS. BMC time synchronization flowchart are as follow:

### BMC time synchronization flowchart
``` mermaid
graph TD;
bmcStart[BMC boot up]--> defaultTime(BMC defualt build time)
defaultTime-->getFromRTC[Get time from external RTC]
getFromRTC-->isNTP{is NTP server set?}
isNTP--Y-->getNTPTime[Get time from NTP server]
isNTP--N-->Exit
getNTPTime-->setExRTC[Set time to external RTC]
setExRTC-->Exit
```
Following  the BMC time synchronization flow chart ,  BMC needs to set the following:
1. Add external RTC in DTS.
2. NTP server config set to time.facebook servers by default.
3. Set the function of NTP server configuration. (For example, set the configuration in /etc/systemd/timesyncd.conf.d)

and BIOS time synchronization flowchart are as follow:

### BIOS time synchronization flowchart
``` mermaid
graph TD;
hostStart[Host power on] ==BIOS==> getInRTCTime
getInRTCTime(Get default time from internal RTC) --> getBMCTime(Get time from BMC)
getBMCTime==OS==>isNTP{Is NTP server set?}
isNTP--Y-->syncNTP[Get time from NTP server]
isNTP--N-->Exit[Exit]
syncNTP --> Exit[Exit]
```
Following the BIOS time synchronization flow chart, BMC support IPMI/PLDM command - Set/Get event log timestamp and BIC need to support Set/Get SEL time.
