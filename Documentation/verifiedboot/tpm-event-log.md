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
