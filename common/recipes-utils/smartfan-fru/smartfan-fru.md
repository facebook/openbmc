# SmartFan FRU

## Access interface
- Smart Fan FRU is stored in smart fan I2C interfaced EEPROM.
- The EEPROM are compatible with the standard 24LCXX EEPROMs with **2-byte** offset.
- The EEPROM slave address is 7-bit 0x50 (1010000)


## FRU format
The FRU data on the EEPROM will be organized per the [IPMI FRU Information Storage Definition v1.2](https://www.intel.com/content/dam/www/public/us/en/documents/product-briefs/platform-management-fru-document-rev-1-2-feb-2013.pdf)

- All ASCII Strings fields are fixed length ASCII + Latin 1. Any unused characters should be filled with 0x00 (Null).
- All version fields are in BCD+ with the “.” character included.
- The checksum is taken as the negative of the 2’s complement sum of all of the data. Hence, if all the data is added with the checksum, the result should be zero. 


### Over all structure
SMART Fan extended information was provided in OEM multi-record Area (SmartFan = 0xC0).

| OFFSET | DATA |
|--------|------|
| 0x0000 ~ 0x0007 | General Header |
| 0x0008 ~ 0x0087 | Internal User Area |
| 0x0088 ~ 0x0127 | Product Info |
| 0x0128 ~ 0x012C | OEM Multirecord Header (SmartFan) |
| 0x012D ~ 0x0156 | OEM Multirecord (SmartFan) |

### General Header
The general header following IPMI FRU general header format
| offset | type | value | length | meaning | field |
|--------|------|-------|--------|---------|-------|
| 0x00 | Binary | 0x01 | 1 | V1 | Common Header Format Version |
| 0x01 | Binary | 0x01 | 1 | offset @ byte 8 | Internal User Area offset |
| 0x02 | Binary | 0x00 | 1 | unused | Chassis Info Area offset |
| 0x03 | Binary | 0x00 | 1 | unused | Board Area offset |
| 0x04 | Binary | 0x11 | 1 | offset @ byte 136 | Product Info Area offset |
| 0x05 | Binary | 0x25 | 1 | offset @ byte 296 | MultiRecord Area offset |
| 0x06 | Binary | 0x00 | 1 | - | PAD |
| 0x07 | Binary | 0xXX | 1 | - | Zero Checksum |

### Internal Area
The internal user area provides a region of memory for internal management controller data

| offset | type | value | length | meaning | field |
|--------|------|-------|--------|---------|-------|
| 0x00 | Binary | 0x01 | 1 | - | Internal User Format Version |
| 0x01 | Any | Any | 127 | - | Internal user data for management controllers|


### Prodcut Info Area
The standard IPMI product info area populated with fan information

| offset | type | value | length | meaning | field | comments |
|--------|------|-------|--------|---------|-------|----------|
| 0x00 | Binary | 0x01 | 1 | V1 | Product Area Format Version | - |
| 0x01 | Binary | 0x14 | 1 | 160 bytes | Production Area length | - |
| 0x02 | Binary | 0x00 | 1 | English | Language code | - |
| 0x03 | Binary | 0xD0 | 1 | ASCII 16 chars | Manufactur Name type/length byte| - |
| 0x04 | ASCII | 16-chars | 16 | - | Manufactur name | - |
| 0x14 | Binary| 0xDE | 1 | ASCII 30 chars | Product Name type/length byte | - |
| 0x15 | ASCII | 30-chars | 30 | - | Product Name | - |
| 0x33 | Binary| 0xDE | 1 | ASCII 30 chars | Product Model Number type/name byte | - |
| 0x34 | ASCII | 30-chars | 30 | - | Product model number | - |
| 0x52 | Binary | 0x44 | 1 | BCD 4-chars | Product Version type/length byte | - |
| 0x53 | BCD+ | XX.XX.XX | 4 | - | Proudct version (Maj/Min/Patch)| Fan hw version specified by Major/Minor/Patch |
| 0x57 | Binary | 0xDE | 1 | ASCII 30 chars | Product Serial Number type/length byte | - |
| 0x58 | ASCII | 30-chars | 30 | - | Product Serial Number | - |
| 0x76 | Binary | 0xD0 | 1 | ASCII 16 chars | Asset Tag type/length byte| - |
| 0x77 | ASCII | 16-chars | 16 | - | Asset Tag | - |
| 0x87 | Binary| 0x00 | 1 | Empty Field ID | FRU File ID type/length byte | - |
| 0x88 | Binary | 0xD0 | 1 | ASCII 16 chars | CUSTOM - FBPN type/length byte | - |
| 0x89 | ASCII | 16-chars | 16 | - | CUSTOM - FBPN | FB internal part number |
| 0x99 | Binary | 0xC1 | 1 | - | No more fields | - |
| 0x9A | Binary | 0x00 | 5 | - | Padding | - |
| 0x9F | Binary | 0xXX | 1 | - | Zero Checksum | - |

**Total 160 byte**

### SMART FAN OEM Multi-record
#### FAN OEM multi-record Header (5 Bytes)
| offset | type | value | length | meaning | field | comments |
|--------|------|-------|--------|---------|-------|----------|
| 0x00 | Binary | 0xC0 | 1 | Smart Fan Record | Record Type ID | OEM SMART FAN |
| 0x01 | Binary | 0x82 | 1 | End of list | End of List/Version | - |
| 0x02 | Binary | 0x2A | 1 | 42 bytes | Record Length
| 0x03 | Binary | 0xXX | 1 | - | Record Checksum | - |
| 0x04 | Binary | 0xXX | 1 | - | Header checksum | - |


#### FAN OEM multi-record (42 Bytes)
| offset | type | value | length | meaning | field | comments |
|--------|------|-------|--------|---------|-------|----------|
| 0x00 | Binary | 0x15A000 | 3 | FB IANA-PEN 40981 | Manufacture ID. LS byte first | The manufacture ID |
| 0x03 | BCD+ | xx.xx.xx | 4 | - | CUSTOM - Smartfan version | The smart fan spec version (major.minor.rev) |
| 0x07 | BCD+ | xx.xx.xx | 4 | - | CUSTOM - FW version | The fan firmware version (major.minor.rev) |
| 0x0B | Binary | 0xXXXXXX | 3 | - | CUSTOM - Mfg. Date/Time | manufacturing date and time expressed as number of minutes from 00:00 1/1/1996 (LS byte first). The same as Board info section. |
| 0x0E | ASCII | 8-chars | 8 | - | CUSTOM - Mfg. Line | The manufacturing plant and line the fan came off. Vendor specific codes. Can be blank |
| 0x16 | ASCII | 10-chars | 10 | - | CUSTOM - CLEI Code | Fan's CLEI code. Can be blanded to denote no code. |
| 0x20 | Binary | 0xXXXX | 2 | - | CUSTOM - Voltage | Nominal operating voltage for the fan. (10mV, LSB first) |
| 0x22 | Binary | 0xXXXX | 2 | - | CUSTOM - Current | Nominal maximum operating current for the fan. (10mA,LSB first) |
| 0x24 | Binary | 0xXXXXXX | 3 | - | CUSTOM - RPM | Nominal maximum operating RPM of the Fan (Front Rotor, LSB first) |
| 0x27 | Binary | 0xXXXXXX | 3 | - | CUSTOM - RPM | Nominal maximum operating RPM of the FAN (Rear Rotor, LSB first)
