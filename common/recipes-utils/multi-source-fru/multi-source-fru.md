# Multi-source INFO FRU

## FRU format
The FRU data on the EEPROM will be organized per the [IPMI FRU Information Storage Definition v1.2](https://www.intel.com/content/dam/www/public/us/en/documents/product-briefs/platform-management-fru-document-rev-1-2-feb-2013.pdf)

- All ASCII Strings fields are fixed length ASCII + Latin 1. Any unused characters should be filled with 0x00 (Null).
- All version fields are in BCD+ with the “.” character included.
- The checksum is taken as the negative of the 2’s complement sum of all of the data. Hence, if all the data is added with the checksum, the result should be zero. 

## Multi-source INFO OEM Multi-record FRU format
- This field is used to store component part number for source identification
- Each record in this field in stored in TLV(type/length/value) format
  - Type: component ID which should be defined in component ID table
  - Length: data length of the value field
  - Value: ASCII string of the part number which don't need null terminated
- It could store multiple record in this field but the total size is limited in 255 bytes

### General Header
The general header following IPMI FRU general header format
| offset | type | value | length | meaning | field |
|--------|------|-------|--------|---------|-------|
| 0x00 | Binary | 0x01 | 1 | V1 | Common Header Format Version |
| 0x01 | Binary | 0x00 | 1 | unused | Internal User Area offset |
| 0x02 | Binary | 0x01 | 1 | offset @ byte 8 | Chassis Info Area offset |
| 0x03 | Binary | 0x05 | 1 | offset @ byte 40 | Board Area offset |
| 0x04 | Binary | 0x11 | 1 | offset @ byte 136 | Product Info Area offset |
| 0x05 | Binary | 0x1C | 1 | offset @ byte 224 | MultiRecord Area offset |
| 0x06 | Binary | 0x00 | 1 | - | PAD |
| 0x07 | Binary | 0xXX | 1 | - | Zero Checksum |


### Multi-source INFO OEM Multi-record
#### Multi-source INFO OEM multi-record Header (5 Bytes)
| offset | type | value | length | meaning | field | comments |
|--------|------|-------|--------|---------|-------|----------|
| 0x00 | Binary | 0xC1 | 1 | Multi-source info | Record Type ID | [FB OEM RecordID](https://github.com/facebook/openbmc/blob/helium/common/recipes-lib/fruid/files/fruid.h) |
| 0x01 | Binary | 0x82 | 1 | End of list | End of List/Version | - |
| 0x02 | Binary | 0xXX | 1 | - | Record Length | Max: 255 bytes |
| 0x03 | Binary | 0xXX | 1 | - | Record Checksum | - |
| 0x04 | Binary | 0xXX | 1 | - | Header checksum | - |


#### Multi-source INFO OEM multi-record (X Bytes)
| offset | type | value | length | meaning | field | comments |
|--------|------|-------|--------|---------|-------|----------|
| 0x00 | Binary | 0x00 | 1 | Type (0: BIC) | Component | - |
| 0x01 | Binary | 0x0E | 1 | length(14 bytes) | length of the value field | - |
| 0x02 | ASCII | BXX.XXXXX.XXXX | 14 | value | Part number | - |
| 0x10 | Binary | 0x01 | 1 | Type (1: CPLD) | Component | - |
| 0x11 | Binary | 0x0E | 1 | length(14 bytes) | length of the value field | - |
| 0x12 | ASCII | BXX.XXXXX.XXXX | 14 | value | Part number | - |


### Component ID Table

| ID | Component |
|----|-----------|
| 0x00 | BIC |
| 0x01 | CPLD |
