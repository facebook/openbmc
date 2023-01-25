/*
 * (C) Copyright 2016-Present, Facebook, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _VBS_H_
#define _VBS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

struct vbs {
  /* 00 */ uint32_t uboot_exec_address; /* Location in MMIO where U-Boot/Recovery U-Boot is execution */
  /* 04 */ uint32_t rom_exec_address;   /* Location in MMIO where ROM is executing from */
  /* 08 */ uint32_t rom_keys;           /* Location in MMIO where the ROM FDT is located */
  /* 0C */ uint32_t subordinate_keys;   /* Location in MMIO where subordinate FDT is located */
  /* 10 */ uint32_t rom_handoff;        /* Marker set when ROM is handing execution to U-Boot. */
  /* 14 */ uint8_t force_recovery;      /* Set by ROM when recovery is requested */
  /* 15 */ uint8_t hardware_enforce;    /* Set by ROM when WP pin of SPI0.0 is active low */
  /* 16 */ uint8_t software_enforce;    /* Set by ROM when RW environment uses verify=yes */
  /* 17 */ uint8_t recovery_boot;       /* Set by ROM when recovery is used */
  /* 18 */ uint8_t recovery_retries;    /* Number of attempts to recovery from verification failure */
  /* 19 */ uint8_t error_type;          /* Type of error, or 0 for success */
  /* 1A */ uint8_t error_code;          /* Unique error code, or 0 for success */
  /* 1B */ uint8_t error_tpm;           /* The last-most-recent error from the TPM. */
  /* 1C */ uint16_t crc;                /* A CRC of the vbs structure */
  /* 1E */ uint16_t error_tpm2;         /* The last-most-recent error from the TPM2. */
  /* 20 */ uint32_t subordinate_last;   /* Status reporting only: the last booted subordinate. */
  /* 24 */ uint32_t uboot_last;         /* Status reporting only: the last booted U-Boot. */
  /* 28 */ uint32_t kernel_last;        /* Status reporting only: the last booted kernel. */
  /* 2C */ uint32_t subordinate_current;/* Status reporting only: the current booted subordinate. */
  /* 30 */ uint32_t uboot_current;      /* Status reporting only: the current booted U-Boot. */
  /* 34 */ uint32_t kernel_current;     /* Status reporting only: the current booted kernel. */
  /* 38 */ uint8_t vbs_ver;             /* add vbs version for backward compatible */
  /* 39 */ uint8_t giu_mode;            /* golden image upgrade mode */
  /* 3A */ uint16_t op_cert_size;       /* vboot operation certificate data size */
  /* 3C */ uint32_t op_cert;            /* Location of vboot operation certificate data */
};

bool        vboot_supported(void);
bool        vboot_partition_exists(void);
struct vbs *vboot_status(void);
const char *vboot_error(uint32_t error_code);
const char *vboot_time(char *buf, size_t buf_len, uint32_t time);

#define VBS_ERROR_TABLE \
  VBS_ERROR(VBS_SUCCESS, 0, "OpenBMC was verified correctly"), \
  VBS_ERROR(VBS_ERROR_MISSING_SPI, 10, "FMC SPI0.1 (CS1) is not populated"), \
  VBS_ERROR(VBS_ERROR_EXECUTE_FAILURE, 11, "U-Boot on FMC SPI0.1 (CS1) did not execute properly"), \
  VBS_ERROR(VBS_ERROR_SPI_PROM, 20, "FMC SPI0.1 (CS1) PROM status invalid or invalid read-mode"), \
  VBS_ERROR(VBS_ERROR_SPI_SWAP, 21, "SPI Swap detected"), \
  VBS_ERROR(VBS_ERROR_BAD_MAGIC, 30, "Invalid FDT magic number for U-Boot FIT at 0x28080000"), \
  VBS_ERROR(VBS_ERROR_NO_IMAGES, 31, "U-Boot FIT did not contain the /images node"), \
  VBS_ERROR(VBS_ERROR_NO_FW, 32, "U-Boot FIT /images node has no 'firmware' subnode"), \
  VBS_ERROR(VBS_ERROR_NO_CONFIG, 33, "U-Boot FIT did not contain the /config node"), \
  VBS_ERROR(VBS_ERROR_NO_KEK, 34, "The ROM was not built with an embedded FDT"), \
  VBS_ERROR(VBS_ERROR_NO_KEYS, 35, "U-Boot FIT did not contain the /keys node"), \
  VBS_ERROR(VBS_ERROR_BAD_KEYS, 36, "The intermediate keys within the U-Boot FIT are missing"), \
  VBS_ERROR(VBS_ERROR_BAD_FW, 37, "U-Boot data is invalid or missing"), \
  VBS_ERROR(VBS_ERROR_INVALID_SIZE, 38, "U-Boot FIT total size is invalid"), \
  VBS_ERROR(VBS_ERROR_KEYS_INVALID, 40, "The intermediate keys could not be verified using the ROM keys"), \
  VBS_ERROR(VBS_ERROR_KEYS_UNVERIFIED, 41, "The intermediate keys were not verified using the ROM keys"), \
  VBS_ERROR(VBS_ERROR_FW_INVALID, 42, "U-Boot could not be verified using the intermediate keys"), \
  VBS_ERROR(VBS_ERROR_FW_UNVERIFIED, 43, "U-Boot was not verified using the intermediate keys"), \
  VBS_ERROR(VBS_ERROR_FORCE_RECOVERY, 50, "Recovery boot was forced using the force_recovery environment variable"), \
  VBS_ERROR(VBS_ERROR_OS_INVALID, 60, "The rootfs and kernel FIT is invalid"), \
  VBS_ERROR(VBS_ERROR_TPM_SETUP, 70, "There is a general TPM or TPM hardware setup error"), \
  VBS_ERROR(VBS_ERROR_TPM_FAILURE, 71, "There is a general TPM API failure"), \
  VBS_ERROR(VBS_ERROR_TPM_NO_PP, 72, "The TPM is not asserting physical presence"), \
  VBS_ERROR(VBS_ERROR_TPM_INVALID_PP, 73, "The TPM physical presence configuration is invalid"), \
  VBS_ERROR(VBS_ERROR_TPM_NO_PPLL, 74, "The TPM cannot set the lifetime lock for physical presence"), \
  VBS_ERROR(VBS_ERROR_TPM_PP_FAILED, 75, "The TPM cannot assert physical presence"), \
  VBS_ERROR(VBS_ERROR_TPM_NOT_ENABLED, 76, "The TPM is not enabled"), \
  VBS_ERROR(VBS_ERROR_TPM_ACTIVATE_FAILED, 77, "The TPM cannot be activated"), \
  VBS_ERROR(VBS_ERROR_TPM_RESET_NEEDED, 78, "The TPM and CPU must be reset"), \
  VBS_ERROR(VBS_ERROR_TPM_NOT_ACTIVATED, 79, "The TPM was not activated after a required reset"), \
  VBS_ERROR(VBS_ERROR_TPM_NV_LOCK_FAILED, 80, "There is a general TPM NV storage failure"), \
  VBS_ERROR(VBS_ERROR_TPM_NV_NOT_LOCKED, 81, "The TPM NV storage is not locked"), \
  VBS_ERROR(VBS_ERROR_TPM_NV_SPACE, 82, "Cannot define TPM NV regions or max writes exhausted"), \
  VBS_ERROR(VBS_ERROR_TPM_NV_BLANK, 83, "Cannot write blank data to TPM NV region"), \
  VBS_ERROR(VBS_ERROR_TPM_NV_READ_FAILED, 84, "There is a general TPM NV read failure"), \
  VBS_ERROR(VBS_ERROR_TPM_NV_WRITE_FAILED, 85, "There is a general TPM NV write failure"), \
  VBS_ERROR(VBS_ERROR_TPM_NV_NOTSET, 86, "The TPM NV region content is invalid"), \
  VBS_ERROR(VBS_ERROR_ROLLBACK_MISSING, 90, "Rollback protection timestamp missing"), \
  VBS_ERROR(VBS_ERROR_ROLLBACK_FAILED, 91, "Rollback protection failed"), \
  VBS_ERROR(VBS_ERROR_ROLLBACK_HUGE, 92, "Rollback protection is jumping too far into the future"), \
  VBS_ERROR(VBS_ERROR_ROLLBACK_FINISH, 99, "Rollback protection did not finish")

#define VBS_ERROR(type, val, string) type = val
enum {
  VBS_ERROR_TABLE,
};
#undef VBS_ERROR

#ifdef __cplusplus
}
#endif

#endif /* _VBS_H_ */
