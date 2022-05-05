# TPM Event Log

## Background

OpenBMC verified boot (u-boot SPL and Proper) implements the
measured boot feature, which will record all the hashes of
its loaded f/w components into TPM PCRs to facilitate the attestation
of OpenBMC.

The OpenBMC Attestation service will retrieve these recorded hashes from
TPM via OpenBMC Attestation Agent. This retrieval process takes two steps:

1. Get the TPM signed quotes, essentially get the PCR values.
2. Get the TPM event log, essentially to replay the hash recording process.

This file documents the TPM event log requirement, design and implementation.

## Requirement

TCG offers an extensive standard for the event logs [here](https://trustedcomputinggroup.org/wp-content/uploads/TCG_IWG_CEL_v1_r0p30_13feb2021.pdf)

In a nutshell, each event log record needs to include this information:

1. Index number
2. PCR Number
3. Digest
4. Context on what the measurement means
5. Misc. metadata like hash algo used etc.

### Index number

1. Start at zero at TPM2_Startup (CLEAR).
2. Monotonically incremented with each measurement.
3. Be maintained separately per PCR

### PCR number

1. This field indicates which PCR was affected by a measurement
2. MUST be the integer representation of the PCR

### Digest

1. This field contains the list of the digest values Extended
2. If using the TPM2_PCR_Extend command, this field is the data sent to the TPM

### Context on what the measurement means

1. This mapping to *Record Event Content* specified in CEL-IM (Core Event Log - Information Model)
2. The *content* will tell what has been measured.

### Misc

1. Will indicate which PCR bank (indicate what hash algo) was extended

## Design

### Overall

The event log will be saved as a binary blob in a dedicated SRAM. U-Boot SPL and proper code which execute the measurement will write the log.
OpenBMC user space, the attestation agent can access the event log from SRAM. And decode based on the following event log encoding.

### Event Log Format (V1)

TPM event log format v1:

* First 4 byte indicates the length in bytes of all event log records.
* Followed by all event log records
* Each event log record will be saved in SRAM event log area catenated *without* padding
* refer to [Event Log Record Encoding](### Event Log Record Encoding) for the binary format of each event log record.
* Finished with 3 byte event log end mark which consists of
* 2 byte magic code (0xFBBE)
* 2 byte TPM event log format version, currently is 1

```txt
+-------+----...........--------+--------+--------+
| len   |  event log records    | 0xFBBE | fmt-ver|
+-------+----...........--------+--------+--------+
```

The following c code descrbing the TPM event log structure

```c
union tpm_event_log_end_mark {
    uint32_t endmark;
    struct {
        uint16_t magic;
        uint16_t fmtver;
    };
};

struct tpm_event_log {
    uint32_t len; /* length of the event logs */
    union {
        uint8_t data[0]; /* tpm_event_log_records */
        union tpm_event_log_end_mark end;  /* end of log */
    };
};
```

### Event Log Record Encoding

To

1. Save SRAM space
2. Accommodate variable length record, though currently does not needed
as we use the same sha algo for all measurements
3. Easy to parse, don't need to worry about padding.

Each event log record will be encoded as the following c structure:

```c
struct tpm_event_log_record {
    uint16_t measurement; // enumeration of measurements
    uint8_t  pcrid;
    uint8_t  algo;
    uint32_t index;
    uint8_t  digest[0]; // hash value, variable length dependents on algo
};

enum measurements {
    measure_unknown = 0,
    measure_spl = 1,
    measure_keystore = 2,
    measure_uboot = 3,
    measure_recv_uboot = 4,
    measure_uboot_env = 5,
    measure_vbs = 6,
    measure_os_kernel = 7,
    measure_os_rootfs = 8,
    measure_os_dtb = 9,
    measure_recv_os_kernel = 10,
    measure_recv_os_rootfs = 11,
    measure_recv_os_dtb = 12,
    // mark the end
    measure_log_end = TPM_EVENT_LOG_MAGIC_CODE,
};
```

Currently the supported algo is SHA256, so the digest is 32 Byte (256bits)
then the `sizeof(tpm_event_log_record)` is 40 Byte

And each boot up will execute 8 measurements, which will generate 8 records.
That is `4 + 40 * 8 + 4 = 328` Byte requirement at least.

Reserved 2KB (2048) bytes for event log, in current design, which can contain `( 2048 - 8 ) / 40 = 51`

### Event Log Location and SRAM memory map

#### AST2500

AST2500 BMC has 36KB SRAM with physical base address 0x1E72_0000.
The current layout

```txt
+===================================+ 0x1E72_9000
|             HEAP (12KB)           |
.                                   .
+-----------------------------------+ 0x1E72_6000
|                GD                 |
+-----------------------------------+ 0x1E72_5F10
|               stack               |
.                                   .
.                                   .
x.......healthd reboot flag.........x 0x1E72_1200
+-----------------------------------+ 0x1E72_1000
.         MISC scratch pad (4KB)     .
.                                   .
+............... vbs ...............+ 0x1E72_0200
|                                   |
+===================================+ 0x1E72_0000
```

The new layout, using the NOT USED SRAM

```txt
+===================================+ 0x1E72_9000
|          TPM event log (2KB)      |
+-----------------------------------+ 0x1E72_8800
|             HEAP (12KB)           |
.                                   .
+-----------------------------------+ 0x1E72_5800
|                GD                 |
+-----------------------------------+ 0x1E72_5710
|               stack               |
.                                   .
.                                   .
x.......healthd reboot flag.........x 0x1E72_1200
+-----------------------------------+ 0x1E72_1000
.         MISC scratch pad (4KB)     .
.                                   .
+............... vbs ...............+ 0x1E72_0200
|                                   |
+===================================+ 0x1E72_0000

```

As the above SRAM memory map showed, the TPM event log will be located at **[0x1E72_8800 ~ 0x1E72_9000)** 2KB.

#### AST2600

AST2600 BMC has 89KB SRAM with physical base address 0x1000_0000,
And the SRAM gets divided into three sections

1. First 64KB. Address range is from 0x1000 0000 to 0x1000 FFFF.
• Available at boot.
• With Parity Check. Parity Check only supports Double-Word access.
2. 24KB. Address range is from 0x1001 0000 to 0x1001 5FFF.
• Available at boot.
• Without Parity Check.
3. 1KB. Address range is from 0x1001 6000 to 0x1001 63FF.
• Available at boot.
• Without Parity Check.
• Each byte is written once. Unlock until reset.
• One global control bit to lock all at once. Unlock until reset

Before the change SRAM usage

```txt
+===================================+ 0x1001_6400
|             NOT USED              |
+===================================+ 0x1001_6000
|              rsv                  | reboot flag @0x1001_5C00, 0x1001_5C04, 0x1001_5C08
............Reboot Flag ............. 0x1001_5C00
|               vbs (max 1KB)       |
+-----------------------------------+ 0x1001_5800
|      SPL Heap (malloc_f) (10KB)   |
.                                   .
+-----------------------------------+ 0x1001_2800
|                GD                 |
+-----------------------------------+ 0x1001_2710
|               stack               |
.                                   .
.                                   .
|                                   |
+===================================+ 0x1000_0000

```

Reserved 2KB SRAM after vbs rsv, from 0x1001_5000, for TPM event log

```txt
+===================================+ 0x1001_6400
|             NOT USED              |
+===================================+ 0x1001_6000
|              rsv                  | reboot flag @0x1001_5C00, 0x1001_5C04, 0x1001_5C08
............Reboot Flag ............. 0x1001_5C00
|               vbs (max 1KB)       |
+-----------------------------------+ 0x1001_5800
|          TPM event log (2KB)      |
+-----------------------------------+ 0x1001_5000
|      SPL Heap (malloc_f) (10KB)   |
.                                   .
+-----------------------------------+ 0x1001_2000
|                GD                 |
+-----------------------------------+ 0x1001_1F10
|               stack               |
.                                   .
.                                   .
|                                   |
+===================================+ 0x1000_0000
```

As the above SRAM memory map showed, the TPM event log will be located at **[0x1001_5000 ~ 0x1001_5800)** 2KB.

## Test

mboot_check is the built-in tool of OpenBMC to test the measured boot.
This tool provides comprehensive options to support print, check and debug measured boot and TPM event log. Run ```mboot-check -h``` to get the usage help of this tool.

 Tl;Dr

* ```mboot-check -t --ce --pe``` to let tool not only **automatically load and check the TPM event log** but also **print** the event log.
* Testing tool can dependents on return code of the tool to make the decision the measured boot testing *pass* or *failed*
* use "-j|--json" to let the tool print out the TPM event log (if ```--pe``` specified) in json format, when what to integate this tool with automation test cases.

### Event log loading modes

The mboot_check tool support 4 modes, which can be specified via ```-e|--event``` command line option, to using the TPM event log.

#### auto mode

the fault mode, measurement boot check will detects whether wellformed TPM event log exists at well known designed location in SRAM. If so, the TPM event log will get load and used to check the measured boot. If not, fall back to do measurement boot check without TPM event log. i.e.

measurement check, omit -e option using default mode(auto)

```sh
root@dhcp-100-96-192-172:~# mboot-check -t
 0:[926f8aac73adc3d793fb490742413acbef6cc8e4e43c69fec7b9ffc4adb11baf] (spl)
   [926f8aac73adc3d793fb490742413acbef6cc8e4e43c69fec7b9ffc4adb11baf] (pcr0)
 1:[523d00ec601dd572f003a2ede471946e495bbb7e9a7cf265acd288116c7005de] (key-store)
   [523d00ec601dd572f003a2ede471946e495bbb7e9a7cf265acd288116c7005de] (pcr1)
 2:[1d1a034a050d25aeebae3d0cb86fc76bda16b2a78c7b51ea89eb9d7e7936d4de] (u-boot)
   [1d1a034a050d25aeebae3d0cb86fc76bda16b2a78c7b51ea89eb9d7e7936d4de] (pcr2)
 3:[21876f6a102caeb6f526c0d4cc5073dc41b8ca20c3995a839335dca22417ebdb] (u-boot-env)
   [21876f6a102caeb6f526c0d4cc5073dc41b8ca20c3995a839335dca22417ebdb] (pcr3)
 5:[4fff524b6803c5c808e107eb7fbea6cb754403405c906910f878502337a24128] (vbs)
   [4fff524b6803c5c808e107eb7fbea6cb754403405c906910f878502337a24128] (pcr5)
 9:[32cbc7cdf9f94ef3d8afb4b3ddd9dc185a1814af01e547770f836e6f9818c0dd] (os)
   [32cbc7cdf9f94ef3d8afb4b3ddd9dc185a1814af01e547770f836e6f9818c0dd] (pcr9)

```

measurement check, omit -e option using default mode(auto), and print the event log

```sh
root@dhcp-100-96-192-172:~# mboot-check -t --pe
 0:[926f8aac73adc3d793fb490742413acbef6cc8e4e43c69fec7b9ffc4adb11baf] (spl)
   [926f8aac73adc3d793fb490742413acbef6cc8e4e43c69fec7b9ffc4adb11baf] (pcr0)
 1:[523d00ec601dd572f003a2ede471946e495bbb7e9a7cf265acd288116c7005de] (key-store)
   [523d00ec601dd572f003a2ede471946e495bbb7e9a7cf265acd288116c7005de] (pcr1)
 2:[1d1a034a050d25aeebae3d0cb86fc76bda16b2a78c7b51ea89eb9d7e7936d4de] (u-boot)
   [1d1a034a050d25aeebae3d0cb86fc76bda16b2a78c7b51ea89eb9d7e7936d4de] (pcr2)
 3:[21876f6a102caeb6f526c0d4cc5073dc41b8ca20c3995a839335dca22417ebdb] (u-boot-env)
   [21876f6a102caeb6f526c0d4cc5073dc41b8ca20c3995a839335dca22417ebdb] (pcr3)
 5:[4fff524b6803c5c808e107eb7fbea6cb754403405c906910f878502337a24128] (vbs)
   [4fff524b6803c5c808e107eb7fbea6cb754403405c906910f878502337a24128] (pcr5)
 9:[32cbc7cdf9f94ef3d8afb4b3ddd9dc185a1814af01e547770f836e6f9818c0dd] (os)
   [32cbc7cdf9f94ef3d8afb4b3ddd9dc185a1814af01e547770f836e6f9818c0dd] (pcr9)
==============================================================================================================
ID  MID     Measure-Name    PCR Index   Algo                               Digest
--------------------------------------------------------------------------------------------------------------
1    1          spl          0    0    sha256 d1b8d62b917b5615d6af5214dba25ba7bc634169b99e94804b63ac53762733ff
2    2       key-store       1    0    sha256 108053f5bdda4094469170e5aa6335cedeba9a65c45893935889c6295325af30
3    3         u-boot        2    0    sha256 e6df451f9df7b9fc887f51e80dac525fbd533f2c0c0f7950021cd7868d81f229
4    5       u-boot-env      3    0    sha256 b0ef519ec3f84e61a6d70ae189a6bc805efb68ada558a4dfff627ed88fef3af5
5    6          vbs          5    0    sha256 f2fd7731a1337f0162075985138a88ef1e491cf6bc7b30958e3582606098e4d8
6    7       os:kernel       9    0    sha256 c13a50d836e51377dd9421ac8c2b722298f605edd0fc0ed58edce526bb413331
7    8       os:rootfs       9    1    sha256 f3a14668e1e029ad309029c8172ab05c1b28e9536cb0f269d061b5b55d965553
8    9         os:dtb        9    2    sha256 400d78865e6a555f46f9a1a32eaea89c8d96faff3c25f0e2083a26f133e3824d
==============================================================================================================
```

measurement check, omit -e option using default mode(auto), and print the event log

#### sram mode

Explictly specify to use tpm event log saved in SRAM. If no well formed TPM event log exists in SRAM, report error.

```sh
# corrupted on purpose of the TPM event log in SRAM at 0x1001_5000
root@dhcp-100-96-192-172:~# devmem 0x10015000 l
0x00000140
root@dhcp-100-96-192-172:~# devmem 0x10015000 l 0
# run the mboot-check explictly request load log from SRAM and failed
# as we set the event log len to 0 at 0x10015000, the endmark following is not valid
# the tool will report the error. and provide debug data collection suggestion 
root@dhcp-100-96-192-172:~# mboot-check -t -e sram
Malformed TPM log, magic code 0x0001 != 0xfbbe
run command to dump the raw event log to debug

    SOC_MODEL_ASPEED_G6:
    $> phymemdump 0x10015000 0x800 -b | hexdump -e '4/4 "%08x " "\n"'
    or
    $> phymemdump 0x10015000 0x800 -b > /tmp/event-log.bin
    and download event-log.bin

root@dhcp-100-96-192-172:~# mboot-check -t -e sram
Malformed TPM log, magic code 0x0001 != 0xfbbe
run command to dump the raw event log to debug

    SOC_MODEL_ASPEED_G6:
    $> phymemdump 0x10015000 0x800 -b | hexdump -e '4/4 "%08x " "\n"'
    or
    $> phymemdump 0x10015000 0x800 -b > /tmp/event-log.bin
    and download event-log.bin

# the tool will return non-success return TPM_EVENT_LOG_BAD error code
# EC_TPM_EVENT_LOG_BAD = 5
root@dhcp-100-96-192-172:~# echo $?
5

# run the debug data collection command suggested will dump the whole TPM event log arena.
root@dhcp-100-96-192-172:~# phymemdump 0x10015000 0x800 -b | hexdump -e '4/4 "%08x " "\n"'
00000000 0b000001 00000000 2bd6b8d1
15567b91 1452afd6 a75ba2db 694163bc
80949eb9 53ac634b ff332776 0b010002
00000000 f5538010 9440dabd e5709146
ce3563aa 659abade 939358c4 29c68958
30af2553 0b020003 00000000 1f45dfe6
fcb9f79d e8517f88 5f52ac0d 2c3f53bd
50790f0c 86d71c02 29f2818d 0b030005
00000000 9e51efb0 614ef8c3 e10ad7a6
80bca689 ad68fb5e dfa458a5 d87e62ff
f53aef8f 0b050006 00000000 3177fdf2
017f33a1 85590762 ef888a13 f61c491e
95307bbc 6082358e d8e49860 0b090007
00000000 d8503ac1 7713e536 ac2194dd
22722b8c ed05f698 d50efcd0 26e5dc8e
313341bb 0b090008 00000001 6846a1f3
ad29e0e1 c8299030 5cb02a17 53e9281b
69f2b06c b5b561d0 5355965d 0b090009
00000002 86780d40 5f556a5e a3a1f946
9ca8ae2e fffa968d e2f0253c f1263a08
4d82e333 0001fbbe 00000000 00000000
00000000 00000000 00000000 00000000
*

# now run with 'auto' mode, will tell 'event log is not used' as it is not valid
root@dhcp-100-96-192-172:~# mboot-check -t --pe
 0:[926f8aac73adc3d793fb490742413acbef6cc8e4e43c69fec7b9ffc4adb11baf] (spl)
   [926f8aac73adc3d793fb490742413acbef6cc8e4e43c69fec7b9ffc4adb11baf] (pcr0)
 1:[523d00ec601dd572f003a2ede471946e495bbb7e9a7cf265acd288116c7005de] (key-store)
   [523d00ec601dd572f003a2ede471946e495bbb7e9a7cf265acd288116c7005de] (pcr1)
 2:[1d1a034a050d25aeebae3d0cb86fc76bda16b2a78c7b51ea89eb9d7e7936d4de] (u-boot)
   [1d1a034a050d25aeebae3d0cb86fc76bda16b2a78c7b51ea89eb9d7e7936d4de] (pcr2)
 5:[4fff524b6803c5c808e107eb7fbea6cb754403405c906910f878502337a24128] (vbs)
   [4fff524b6803c5c808e107eb7fbea6cb754403405c906910f878502337a24128] (pcr5)
 9:[32cbc7cdf9f94ef3d8afb4b3ddd9dc185a1814af01e547770f836e6f9818c0dd] (os)
   [32cbc7cdf9f94ef3d8afb4b3ddd9dc185a1814af01e547770f836e6f9818c0dd] (pcr9)
=== TPM event log is not used ===


```

#### File mode

Specify to load the TPM event log from a file, which is specified by ```--ef``` option.This mode is mostly used during development or debug. You can use the phymemdump to dump the event log and run the tool offline (not on the BMC), i.e. on your dev-server.

```sh
# download the dumpped event log from BMC to dev-server
[~/workspaces/openbmc.ubu/build-fbgc] scp root@[${GC2B_IPV6}]:/tmp/test-event-log.bin .
test-event-log.bin

# check it with the image offline on the dev-server
# the measure.py is the script name of mboot-check
[~/workspaces/openbmc.ubu/build-fbgc] ../common/recipes-utils/vboot-utils/files/measure.py -e file --ef test-event-log.bin --pe -i tmp/deploy/images/grandcanyon/flash-grandcanyon.signed --ce -r

 0:[926f8aac73adc3d793fb490742413acbef6cc8e4e43c69fec7b9ffc4adb11baf] (spl)
   [NA] (pcr0)
 1:[523d00ec601dd572f003a2ede471946e495bbb7e9a7cf265acd288116c7005de] (key-store)
   [NA] (pcr1)
 2:[1d1a034a050d25aeebae3d0cb86fc76bda16b2a78c7b51ea89eb9d7e7936d4de] (u-boot)
   [NA] (pcr2)
 3:[21876f6a102caeb6f526c0d4cc5073dc41b8ca20c3995a839335dca22417ebdb] (u-boot-env)
   [NA] (pcr3)
 5:[4fff524b6803c5c808e107eb7fbea6cb754403405c906910f878502337a24128] (vbs)
   [NA] (pcr5)
 9:[32cbc7cdf9f94ef3d8afb4b3ddd9dc185a1814af01e547770f836e6f9818c0dd] (os)
   [NA] (pcr9)
==============================================================================================================
ID  MID     Measure-Name    PCR Index   Algo                               Digest
--------------------------------------------------------------------------------------------------------------
1    1          spl          0    0    sha256 d1b8d62b917b5615d6af5214dba25ba7bc634169b99e94804b63ac53762733ff
2    2       key-store       1    0    sha256 108053f5bdda4094469170e5aa6335cedeba9a65c45893935889c6295325af30
3    3         u-boot        2    0    sha256 e6df451f9df7b9fc887f51e80dac525fbd533f2c0c0f7950021cd7868d81f229
4    5       u-boot-env      3    0    sha256 b0ef519ec3f84e61a6d70ae189a6bc805efb68ada558a4dfff627ed88fef3af5
5    6          vbs          5    0    sha256 f2fd7731a1337f0162075985138a88ef1e491cf6bc7b30958e3582606098e4d8
6    7       os:kernel       9    0    sha256 c13a50d836e51377dd9421ac8c2b722298f605edd0fc0ed58edce526bb413331
7    8       os:rootfs       9    1    sha256 f3a14668e1e029ad309029c8172ab05c1b28e9536cb0f269d061b5b55d965553
8    9         os:dtb        9    2    sha256 400d78865e6a555f46f9a1a32eaea89c8d96faff3c25f0e2083a26f133e3824d
==============================================================================================================
```

#### None mode

None mode will explictly disable loading or use TPM event log even valid event log exists.

```sh

root@dhcp-100-96-192-172:~# mboot-check -t --pe -e none
 0:[926f8aac73adc3d793fb490742413acbef6cc8e4e43c69fec7b9ffc4adb11baf] (spl)
   [926f8aac73adc3d793fb490742413acbef6cc8e4e43c69fec7b9ffc4adb11baf] (pcr0)
 1:[523d00ec601dd572f003a2ede471946e495bbb7e9a7cf265acd288116c7005de] (key-store)
   [523d00ec601dd572f003a2ede471946e495bbb7e9a7cf265acd288116c7005de] (pcr1)
 2:[1d1a034a050d25aeebae3d0cb86fc76bda16b2a78c7b51ea89eb9d7e7936d4de] (u-boot)
   [1d1a034a050d25aeebae3d0cb86fc76bda16b2a78c7b51ea89eb9d7e7936d4de] (pcr2)
 5:[4fff524b6803c5c808e107eb7fbea6cb754403405c906910f878502337a24128] (vbs)
   [4fff524b6803c5c808e107eb7fbea6cb754403405c906910f878502337a24128] (pcr5)
 9:[32cbc7cdf9f94ef3d8afb4b3ddd9dc185a1814af01e547770f836e6f9818c0dd] (os)
   [32cbc7cdf9f94ef3d8afb4b3ddd9dc185a1814af01e547770f836e6f9818c0dd] (pcr9)
=== TPM event log is not used ===

```

### The checking options

Three checking options can be specified:

* ```-t|--tpm``` this is the major option, checking the TPM PCR reading is as spected.
* ```--ce``` let the tool make sure the digest value in the TPM event log is correct. Essentially the tool will recalculate the digest of the measured source indicated in the TPM event log.
* ```--r|--recal``` OpenBMC measured boot measured the hash values exists in the u-boot and OS FIT to avoid re-calculating the hash values of u-boot and OS components during boot up time. This option will recalute the hash from u-boot and os components to make sure the build time, when the FIT is created, hash calculation is correct. This check option is not related to TPM event log checking and tool a lot of time when running on low performance AST2500 BMC, so use it with patient on AST2500 BMC.


```sh
# with all check enabled, especially --recal
# on AST2600 platform, the command took around 20s to finish
root@dhcp-100-96-192-172:~# time mboot-check -t --ce --recal
 0:[926f8aac73adc3d793fb490742413acbef6cc8e4e43c69fec7b9ffc4adb11baf] (spl)
   [926f8aac73adc3d793fb490742413acbef6cc8e4e43c69fec7b9ffc4adb11baf] (pcr0)
 1:[523d00ec601dd572f003a2ede471946e495bbb7e9a7cf265acd288116c7005de] (key-store)
   [523d00ec601dd572f003a2ede471946e495bbb7e9a7cf265acd288116c7005de] (pcr1)
 2:[1d1a034a050d25aeebae3d0cb86fc76bda16b2a78c7b51ea89eb9d7e7936d4de] (u-boot)
   [1d1a034a050d25aeebae3d0cb86fc76bda16b2a78c7b51ea89eb9d7e7936d4de] (pcr2)
 3:[21876f6a102caeb6f526c0d4cc5073dc41b8ca20c3995a839335dca22417ebdb] (u-boot-env)
   [21876f6a102caeb6f526c0d4cc5073dc41b8ca20c3995a839335dca22417ebdb] (pcr3)
 5:[4fff524b6803c5c808e107eb7fbea6cb754403405c906910f878502337a24128] (vbs)
   [4fff524b6803c5c808e107eb7fbea6cb754403405c906910f878502337a24128] (pcr5)
 9:[32cbc7cdf9f94ef3d8afb4b3ddd9dc185a1814af01e547770f836e6f9818c0dd] (os)
   [32cbc7cdf9f94ef3d8afb4b3ddd9dc185a1814af01e547770f836e6f9818c0dd] (pcr9)

real    0m9.817s
user    0m5.649s
sys     0m3.962s

# on AST2500 platform, the command took around 15 mins to recalculate the hash
root@dhcp-100-96-192-244:~# time mboot-check -t --ce
 0:[e67599de57e9195d4edd31c01584f2dd1abe329c8d2567681eb52552d8e6f155] (spl)
   [e67599de57e9195d4edd31c01584f2dd1abe329c8d2567681eb52552d8e6f155] (pcr0)
 1:[9d40f708e15d33952b335352af8fe8cbcb44e18ece59ddf6c060999cec37dba3] (key-store)
   [9d40f708e15d33952b335352af8fe8cbcb44e18ece59ddf6c060999cec37dba3] (pcr1)
 2:[50c340c8665a84890ba791004b48a0ae65a7d649e572424f40dfde077473cc83] (u-boot)
   [50c340c8665a84890ba791004b48a0ae65a7d649e572424f40dfde077473cc83] (pcr2)
 3:[1236753881c5d0eb086d27f984139e2068cad80eec5bc7e40d91ced183e5656a] (u-boot-env)
   [1236753881c5d0eb086d27f984139e2068cad80eec5bc7e40d91ced183e5656a] (pcr3)
 5:[17fcc4e9db5385b7e81d5133c29b9f07baa3f41baac35b8cd6f3ae116d538947] (vbs)
   [17fcc4e9db5385b7e81d5133c29b9f07baa3f41baac35b8cd6f3ae116d538947] (pcr5)
 9:[f2d055a528fb175b5b6fa29fb5d8e22b5322729f85498e3cbca7035f7291fd66] (os)
   [f2d055a528fb175b5b6fa29fb5d8e22b5322729f85498e3cbca7035f7291fd66] (pcr9)

real    1m0.705s
user    0m5.719s
sys     0m0.571s
root@dhcp-100-96-192-244:~# time mboot-check -t --ce --recal

 0:[e67599de57e9195d4edd31c01584f2dd1abe329c8d2567681eb52552d8e6f155] (spl)
   [e67599de57e9195d4edd31c01584f2dd1abe329c8d2567681eb52552d8e6f155] (pcr0)
 1:[9d40f708e15d33952b335352af8fe8cbcb44e18ece59ddf6c060999cec37dba3] (key-store)
   [9d40f708e15d33952b335352af8fe8cbcb44e18ece59ddf6c060999cec37dba3] (pcr1)
 2:[50c340c8665a84890ba791004b48a0ae65a7d649e572424f40dfde077473cc83] (u-boot)
   [50c340c8665a84890ba791004b48a0ae65a7d649e572424f40dfde077473cc83] (pcr2)
 3:[1236753881c5d0eb086d27f984139e2068cad80eec5bc7e40d91ced183e5656a] (u-boot-env)
   [1236753881c5d0eb086d27f984139e2068cad80eec5bc7e40d91ced183e5656a] (pcr3)
 5:[17fcc4e9db5385b7e81d5133c29b9f07baa3f41baac35b8cd6f3ae116d538947] (vbs)
   [17fcc4e9db5385b7e81d5133c29b9f07baa3f41baac35b8cd6f3ae116d538947] (pcr5)
 9:[f2d055a528fb175b5b6fa29fb5d8e22b5322729f85498e3cbca7035f7291fd66] (os)
   [f2d055a528fb175b5b6fa29fb5d8e22b5322729f85498e3cbca7035f7291fd66] (pcr9)

real    13m57.832s
user    1m41.905s
sys     0m7.064s
root@dhcp-100-96-192-244:~#
```

### Misc test example

Booting from golden image

```sh
[zhdaniel@devvm4263.ftw0 /data/users/zhdaniel/workspaces/openbmc.ubu/build/u-boot/u-boot-v2019.04] g2gc2b
root@bmc-oob:~# vboot-util
ROM executed from:       0x00000000
ROM KEK certificates:    0x000182e4
ROM handoff marker:      0x00000000
U-Boot executed from:    0x20040000
U-Boot certificates:     0x00000000
Certificates fallback:   not set
Certificates time:       not set
U-Boot fallback:         not set
U-Boot time:             not set
Kernel fallback:         not set
Kernel time:             not set
Flags force_recovery:    0x01
Flags hardware_enforce:  0x00
Flags software_enforce:  0x00
Flags recovery_boot:     0x01
Flags recovery_retried:  0x01

ROM version:             grandcanyon-062a9f6810b5
ROM U-Boot version:      2019.04
FLASH0 Signed:           0x01
FLASH0 Locked:           0x00
FLASH1 Signed:           0x01
FLASH1 Locked:           0x00

Status CRC: 0x70d7
TPM2 status  (0)
Status type (5) code (50)
Recovery boot was forced using the force_recovery environment variable
root@bmc-oob:~# mboot-check -t --ce --pe
 0:[926f8aac73adc3d793fb490742413acbef6cc8e4e43c69fec7b9ffc4adb11baf] (spl)
   [926f8aac73adc3d793fb490742413acbef6cc8e4e43c69fec7b9ffc4adb11baf] (pcr0)
 1:[523d00ec601dd572f003a2ede471946e495bbb7e9a7cf265acd288116c7005de] (key-store)
   [523d00ec601dd572f003a2ede471946e495bbb7e9a7cf265acd288116c7005de] (pcr1)
 2:[527e24d56d5853a3b550ae5a04a5244a9377bf56838bdc6ab7d5c580a25a5eb9] (rec-u-boot)
   [527e24d56d5853a3b550ae5a04a5244a9377bf56838bdc6ab7d5c580a25a5eb9] (pcr2)
 3:[5e5aaf6fb3aeed74a2af5939885497df2e3e282538a9e5a76b1744a7b698e4db] (u-boot-env)
   [5e5aaf6fb3aeed74a2af5939885497df2e3e282538a9e5a76b1744a7b698e4db] (pcr3)
 5:[2ccec166c9a0828cad105b19e798a228e9c2686601fad72bd2f7df01824a6f79] (vbs)
   [2ccec166c9a0828cad105b19e798a228e9c2686601fad72bd2f7df01824a6f79] (pcr5)
 9:[32cbc7cdf9f94ef3d8afb4b3ddd9dc185a1814af01e547770f836e6f9818c0dd] (recovery-os)
   [32cbc7cdf9f94ef3d8afb4b3ddd9dc185a1814af01e547770f836e6f9818c0dd] (pcr9)
==============================================================================================================
ID  MID     Measure-Name    PCR Index   Algo                               Digest
--------------------------------------------------------------------------------------------------------------
1    1          spl          0    0    sha256 d1b8d62b917b5615d6af5214dba25ba7bc634169b99e94804b63ac53762733ff
2    2       key-store       1    0    sha256 108053f5bdda4094469170e5aa6335cedeba9a65c45893935889c6295325af30
3    4       rec-u-boot      2    0    sha256 d8554b7499a567e73d09aa5f1e1e6a73ba2c496e239be7df438ffb5fd191d3b1
4    5       u-boot-env      3    0    sha256 c5c42e10fdf15b4675171b4ce91a626a0ee44e0c4bf1bb4c167b77eae51f48f7
5    6          vbs          5    0    sha256 7eef269cb8bedb9ba51811200f668b6446a94b03924661f328bbbb036c93314d
6    10  recovery-os:kernel  9    0    sha256 c13a50d836e51377dd9421ac8c2b722298f605edd0fc0ed58edce526bb413331
7    11  recovery-os:rootfs  9    1    sha256 f3a14668e1e029ad309029c8172ab05c1b28e9536cb0f269d061b5b55d965553
8    12   recovery-os:dtb    9    2    sha256 400d78865e6a555f46f9a1a32eaea89c8d96faff3c25f0e2083a26f133e3824d
==============================================================================================================

```

the digest checking is working

```sh
# modify the digest in tpm event log on purpose
root@bmc-oob:~# devmem 0x1001500c l
0x2BD6B8D1
root@bmc-oob:~# devmem 0x1001500c l 0x2BD6B8D0
# the tool find out the mismatch
root@bmc-oob:~# mboot-check -t --ce --pe
spl digest does't match
        [d0b8d62b917b5615d6af5214dba25ba7bc634169b99e94804b63ac53762733ff](event)
        [d1b8d62b917b5615d6af5214dba25ba7bc634169b99e94804b63ac53762733ff](expect)
run command to dump the raw event log to debug

    SOC_MODEL_ASPEED_G6:
    $> phymemdump 0x10015000 0x800 -b | hexdump -e '4/4 "%08x " "\n"'
    or
    $> phymemdump 0x10015000 0x800 -b > /tmp/event-log.bin
    and download event-log.bin

root@bmc-oob:~#

```
