/*
 * otp
 *
 * Copyright 2021-present Facebook. All Rights Reserved.
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

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <termios.h>
#include <termios.h>
#include <ctype.h>
#include "aspeed-otp.h"
#include "otp_info.h"
#include "sha256.h"

#define OTP_VER				"1.1.0"

#define BIT(nr)					(1UL << (nr))
#define OTP_REGION_STRAP		BIT(0)
#define OTP_REGION_CONF			BIT(1)
#define OTP_REGION_DATA			BIT(2)

#define OTP_USAGE				-1
#define OTP_FAILURE				-2
#define OTP_SUCCESS				0

#define OTP_PROG_SKIP			1

#define OTP_KEY_TYPE_RSA_PUB	1
#define OTP_KEY_TYPE_RSA_PRIV	2
#define OTP_KEY_TYPE_AES		3
#define OTP_KEY_TYPE_VAULT		4
#define OTP_KEY_TYPE_HMAC		5

#define OTP_MAGIC		        "SOCOTP"
#define CHECKSUM_LEN	        32
#define OTP_INC_DATA	        BIT(31)
#define OTP_INC_CONF	        BIT(30)
#define OTP_INC_STRAP	        BIT(29)
#define OTP_ECC_EN		        BIT(28)
#define OTP_REGION_SIZE(info)	(((info) >> 16) & 0xffff)
#define OTP_REGION_OFFSET(info)	((info) & 0xffff)
#define OTP_IMAGE_SIZE(info)	((info) & 0xffff)

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

struct otp_header {
	u8	otp_magic[8];
	u8	otp_version[8];
	u32	image_info;
	u32	data_info;
	u32	conf_info;
	u32	strap_info;
	u32	checksum_offset;
} __packed;

struct otpstrap_status {
	int value;
	int option_array[7];
	int remain_times;
	int writeable_option;
	int reg_protected;
	int protected;
};

struct otpconf_parse {
	int dw_offset;
	int bit;
	int length;
	int value;
	int ignore;
	char status[80];
};

struct otpkey_type {
	int value;
	int key_type;
	int need_id;
	char information[110];
};

struct otp_info_cb {
	int otp_fd;
	int version;
	const struct otpstrap_info *strap_info;
	int strap_info_len;
	const struct otpconf_info *conf_info;
	int conf_info_len;
	const struct otpkey_type *key_info;
	int key_info_len;
};

struct otp_image_layout {
	int data_length;
	int conf_length;
	int strap_length;
	uint8_t *data;
	uint8_t *data_ignore;
	uint8_t *conf;
	uint8_t *conf_ignore;
	uint8_t *strap;
	uint8_t *strap_reg_pro;
	uint8_t *strap_pro;
	uint8_t *strap_ignore;
};

static struct otp_info_cb info_cb;

static const struct otpkey_type a0_key_type[] = {
	{0, OTP_KEY_TYPE_AES,   0, "AES-256 as OEM platform key for image encryption/decryption"},
	{1, OTP_KEY_TYPE_VAULT, 0, "AES-256 as secret vault key"},
	{4, OTP_KEY_TYPE_HMAC,  1, "HMAC as encrypted OEM HMAC keys in Mode 1"},
	{8, OTP_KEY_TYPE_RSA_PUB,   1, "RSA-public as OEM DSS public keys in Mode 2"},
	{9, OTP_KEY_TYPE_RSA_PUB,   0, "RSA-public as SOC public key"},
	{10, OTP_KEY_TYPE_RSA_PUB,  0, "RSA-public as AES key decryption key"},
	{13, OTP_KEY_TYPE_RSA_PRIV,  0, "RSA-private as SOC private key"},
	{14, OTP_KEY_TYPE_RSA_PRIV,  0, "RSA-private as AES key decryption key"},
};

static const struct otpkey_type a1_key_type[] = {
	{1, OTP_KEY_TYPE_VAULT, 0, "AES-256 as secret vault key"},
	{2, OTP_KEY_TYPE_AES,   1, "AES-256 as OEM platform key for image encryption/decryption in Mode 2 or AES-256 as OEM DSS keys for Mode GCM"},
	{8, OTP_KEY_TYPE_RSA_PUB,   1, "RSA-public as OEM DSS public keys in Mode 2"},
	{10, OTP_KEY_TYPE_RSA_PUB,  0, "RSA-public as AES key decryption key"},
	{14, OTP_KEY_TYPE_RSA_PRIV,  0, "RSA-private as AES key decryption key"},
};

static const struct otpkey_type a2_key_type[] = {
	{1, OTP_KEY_TYPE_VAULT, 0, "AES-256 as secret vault key"},
	{2, OTP_KEY_TYPE_AES,   1, "AES-256 as OEM platform key for image encryption/decryption in Mode 2 or AES-256 as OEM DSS keys for Mode GCM"},
	{8, OTP_KEY_TYPE_RSA_PUB,   1, "RSA-public as OEM DSS public keys in Mode 2"},
	{10, OTP_KEY_TYPE_RSA_PUB,  0, "RSA-public as AES key decryption key"},
	{14, OTP_KEY_TYPE_RSA_PRIV,  0, "RSA-private as AES key decryption key"},
};

static const struct otpkey_type a3_key_type[] = {
	{1, OTP_KEY_TYPE_VAULT, 0, "AES-256 as secret vault key"},
	{2, OTP_KEY_TYPE_AES,   1, "AES-256 as OEM platform key for image encryption/decryption in Mode 2 or AES-256 as OEM DSS keys for Mode GCM"},
	{8, OTP_KEY_TYPE_RSA_PUB,   1, "RSA-public as OEM DSS public keys in Mode 2"},
	{9, OTP_KEY_TYPE_RSA_PUB,   1, "RSA-public as OEM DSS public keys in Mode 2(big endian)"},
	{10, OTP_KEY_TYPE_RSA_PUB,  0, "RSA-public as AES key decryption key"},
	{11, OTP_KEY_TYPE_RSA_PUB,  0, "RSA-public as AES key decryption key(big endian)"},
	{12, OTP_KEY_TYPE_RSA_PRIV,  0, "RSA-private as AES key decryption key"},
	{13, OTP_KEY_TYPE_RSA_PRIV,  0, "RSA-private as AES key decryption key(big endian)"},
};

int confirm_yesno(void)
{
	int i;
	char str_input[5];

	i = 0;
	while (i < sizeof(str_input)) {
		str_input[i] = getc(stdin);
		if (str_input[i] == '\n')
			break;
		i++;
	}

	if (strncmp(str_input, "y\n", 2) == 0 ||
	    strncmp(str_input, "Y\n", 2) == 0 ||
	    strncmp(str_input, "yes\n", 4) == 0 ||
	    strncmp(str_input, "YES\n", 4) == 0)
		return 1;
	return 0;
}

static void buf_print(uint8_t *buf, int len)
{
	int i;

	printf("      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
	for (i = 0; i < len; i++) {
		if (i % 16 == 0)
			printf("%04X: ", i);
		printf("%02X ", buf[i]);
		if ((i + 1) % 16 == 0)
			printf("\n");
	}
}

static int get_dw_bit(uint32_t *rid, int offset)
{
	int bit_offset;
	int i;

	if (offset < 32) {
		i = 0;
		bit_offset = offset;
	} else {
		i = 1;
		bit_offset = offset - 32;
	}
	if ((rid[i] >> bit_offset) & 0x1)
		return 1;
	else
		return 0;
}

static int get_rid_num(uint32_t *rid)
{
	int i;
	int fz = 0;
	int rid_num = 0;
	int ret = 0;

	for (i = 0; i < 64; i++) {
		if (get_dw_bit(rid, i) == 0) {
			if (!fz)
				fz = 1;

		} else {
			rid_num++;
			if (fz)
				ret = OTP_FAILURE;
		}
	}
	if (ret)
		return ret;

	return rid_num;
}

static int otp_strap_bit_confirm(struct otpstrap_status *otpstrap, int offset, int ibit, int bit, int pbit, int rpbit)
{
	int prog_flag = 0;

	// ignore this bit
	if (ibit == 1)
		return OTP_SUCCESS;
	printf("OTPSTRAP[%X]:\n", offset);

	if (bit == otpstrap->value) {
		if (!pbit && !rpbit) {
			printf("    The value is same as before, skip it.\n");
			return OTP_PROG_SKIP;
		}
		printf("    The value is same as before.\n");
	} else {
		prog_flag = 1;
	}
	if (otpstrap->protected == 1 && prog_flag) {
		printf("    This bit is protected and is not writable\n");
		return OTP_FAILURE;
	}
	if (otpstrap->remain_times == 0 && prog_flag) {
		printf("    This bit is no remaining times to write.\n");
		return OTP_FAILURE;
	}
	if (pbit == 1)
		printf("    This bit will be protected and become non-writable.\n");
	if (rpbit == 1 && info_cb.version != OTP_A0)
		printf("    The relative register will be protected.\n");
	if (prog_flag)
		printf("    Write 1 to OTPSTRAP[%X] OPTION[%X], that value becomes from %d to %d.\n", offset, otpstrap->writeable_option + 1, otpstrap->value, otpstrap->value ^ 1);

	return OTP_SUCCESS;
}

static uint32_t chip_version(void)
{
	uint32_t ver;
	int ret;

	ret = ioctl(info_cb.otp_fd, ASPEED_OTP_VER, &ver);

	if (ret < 0) {
		printf("ioctl err:%d\n", ret);
		return OTP_FAILURE;
	}

	return ver;
}

static uint32_t sw_revid(u32 *sw_rid)
{
	int ret;

	ret = ioctl(info_cb.otp_fd, ASPEED_OTP_SW_RID, sw_rid);

	if (ret < 0) {
		printf("ioctl err:%d\n", ret);
		return OTP_FAILURE;
	}

	return OTP_SUCCESS;
}

static int _otp_read(uint32_t offset, int len, uint32_t *data, unsigned long req)
{
	int ret;
	struct otp_read xfer;

	xfer.data = data;
	xfer.offset = offset;
	xfer.len = len;
	ret = ioctl(info_cb.otp_fd, req, &xfer);
	if (ret < 0) {
		printf("ioctl err:%d\n", ret);
		return OTP_FAILURE;
	}
	return OTP_SUCCESS;
}

static int _otp_prog(uint32_t dw_offset, uint32_t bit_offset, uint32_t value, unsigned long req)
{
	int ret;
	struct otp_prog prog;

	prog.dw_offset = dw_offset;
	prog.bit_offset = bit_offset;
	prog.value = value;

	ret = ioctl(info_cb.otp_fd, req, &prog);
	if (ret < 0) {
		printf("ioctl err:%d\n", ret);
		return OTP_FAILURE;
	}
	return OTP_SUCCESS;
}

static int otp_read_conf_buf(uint32_t offset, int len, uint32_t *data)
{
	return _otp_read(offset, len, data, ASPEED_OTP_READ_CONF);
}

static int otp_read_data_buf(uint32_t offset, int len, uint32_t *data)
{
	return _otp_read(offset, len, data, ASPEED_OTP_READ_DATA);
}

static int otp_read_conf(uint32_t offset, uint32_t *data)
{
	return _otp_read(offset, 1, data, ASPEED_OTP_READ_CONF);
}

static int otp_read_data(uint32_t offset, uint32_t *data)
{
	return _otp_read(offset, 1, data, ASPEED_OTP_READ_DATA);
}

static void otp_read_strap(struct otpstrap_status *otpstrap)
{
	uint32_t OTPSTRAP_RAW[16];
	int strap_end;
	int i, j, k;
	char bit_value;
	int option;

	if (info_cb.version == OTP_A0) {
		for (j = 0; j < 64; j++) {
			otpstrap[j].value = 0;
			otpstrap[j].remain_times = 7;
			otpstrap[j].writeable_option = -1;
			otpstrap[j].protected = 0;
		}
		strap_end = 30;
	} else {
		for (j = 0; j < 64; j++) {
			otpstrap[j].value = 0;
			otpstrap[j].remain_times = 6;
			otpstrap[j].writeable_option = -1;
			otpstrap[j].reg_protected = 0;
			otpstrap[j].protected = 0;
		}
		strap_end = 28;
	}

	otp_read_conf_buf(16, 16, OTPSTRAP_RAW);

	for (i = 16, k = 0; i < strap_end; i += 2, k += 2) {
		option = (i - 16) / 2;

		for (j = 0; j < 32; j++) {
			bit_value = ((OTPSTRAP_RAW[k] >> j) & 0x1);

			if (bit_value == 0 && (otpstrap[j].writeable_option == -1))
				otpstrap[j].writeable_option = option;
			if (bit_value == 1)
				otpstrap[j].remain_times--;
			otpstrap[j].value ^= bit_value;
			otpstrap[j].option_array[option] = bit_value;
		}
		for (j = 32; j < 64; j++) {
			bit_value = ((OTPSTRAP_RAW[k + 1] >> (j - 32)) & 0x1);

			if (bit_value == 0 && otpstrap[j].writeable_option == -1)
				otpstrap[j].writeable_option = option;
			if (bit_value == 1)
				otpstrap[j].remain_times--;
			otpstrap[j].value ^= bit_value;
			otpstrap[j].option_array[option] = bit_value;
		}
	}

	if (info_cb.version != OTP_A0) {
		for (j = 0; j < 32; j++) {
			if (((OTPSTRAP_RAW[12] >> j) & 0x1) == 1)
				otpstrap[j].reg_protected = 1;
		}
		for (j = 32; j < 64; j++) {
			if (((OTPSTRAP_RAW[13] >> (j - 32)) & 0x1) == 1)
				otpstrap[j].reg_protected = 1;
		}
	}

	for (j = 0; j < 32; j++) {
		if (((OTPSTRAP_RAW[14] >> j) & 0x1) == 1)
			otpstrap[j].protected = 1;
	}
	for (j = 32; j < 64; j++) {
		if (((OTPSTRAP_RAW[15] >> (j - 32)) & 0x1) == 1)
			otpstrap[j].protected = 1;
	}
}

static int otp_prog_data_b(uint32_t dw_offset, uint32_t bit_offset, uint32_t value)
{
	return _otp_prog(dw_offset, bit_offset, value, ASPEED_OTP_PROG_DATA);
}

static int otp_prog_conf_b(uint32_t dw_offset, uint32_t bit_offset, uint32_t value)
{
	return _otp_prog(dw_offset, bit_offset, value, ASPEED_OTP_PROG_CONF);
}

static int otp_prog_strap_b(int bit_offset, int value)
{
	struct otpstrap_status otpstrap[64];
	uint32_t prog_address;
	int offset;
	int ret;

	otp_read_strap(otpstrap);

	ret = otp_strap_bit_confirm(&otpstrap[bit_offset], bit_offset, 0, value, 0, 0);

	if (ret != OTP_SUCCESS)
		return ret;

	if (bit_offset < 32) {
		offset = bit_offset;
		prog_address = otpstrap[bit_offset].writeable_option * 2 + 16;

	} else {
		offset = (bit_offset - 32);
		prog_address = otpstrap[bit_offset].writeable_option * 2 + 17;
	}

	return otp_prog_conf_b(prog_address, offset, 1);
}

static int otp_prog_data_dw(uint32_t value, uint32_t ignore, uint32_t prog_address)
{
	int j, bit_value;
	int ret;

	for (j = 0; j < 32; j++) {
		if ((ignore >> j) & 0x1)
			continue;
		bit_value = (value >> j) & 0x1;
		if (prog_address % 2 == 0) {
			if (!bit_value)
				continue;
		} else {
			if (bit_value)
				continue;
		}
		ret = otp_prog_data_b(prog_address, j, bit_value);
		if (ret)
			return ret;
	}
	return OTP_SUCCESS;
}

static int otp_print_conf(uint32_t offset, int dw_count)
{
	int i, j;
	uint32_t ret[32];

	if (otp_read_conf_buf(offset, dw_count, ret))
		return OTP_FAILURE;

	for (i = offset, j = 0; j < dw_count; i++, j++)
		printf("OTPCFG%X: %08X\n", i, ret[j]);
	printf("\n");
	return OTP_SUCCESS;
}

static int otp_print_data(uint32_t offset, int dw_count)
{
	int i, j;
	uint32_t ret[2048];

	if (otp_read_data_buf(offset, dw_count, ret))
		return OTP_FAILURE;

	for (i = offset, j = 0; j < dw_count; i++, j++) {
		if (i % 4 == 0)
			printf("%03X: %08X ", i * 4, ret[j]);
		else
			printf("%08X ", ret[j]);
		if ((j + 1) % 4 == 0)
			printf("\n");
	}

	printf("\n");
	return OTP_SUCCESS;
}

static int otp_print_strap(int offset, int count)
{
	int i, j;
	int remains;
	struct otpstrap_status otpstrap[64];

	otp_read_strap(otpstrap);

	if (info_cb.version == OTP_A0) {
		remains = 7;
		printf("BIT(hex)  Value  Option           Status\n");
	} else {
		remains = 6;
		printf("BIT(hex)  Value  Option         Reg_Protect Status\n");
	}
	printf("______________________________________________________________________________\n");

	for (i = offset; i < offset + count; i++) {
		printf("0x%-8X", i);
		printf("%-7d", otpstrap[i].value);
		for (j = 0; j < remains; j++)
			printf("%d ", otpstrap[i].option_array[j]);
		printf("   ");
		if (info_cb.version != OTP_A0)
			printf("%d           ", otpstrap[i].reg_protected);
		if (otpstrap[i].protected == 1) {
			printf("protected and not writable");
		} else {
			printf("not protected ");
			if (otpstrap[i].remain_times == 0)
				printf("and no remaining times to write.");
			else
				printf("and still can write %d times", otpstrap[i].remain_times);
		}
		printf("\n");
	}

	return OTP_SUCCESS;
}

static void otp_print_revid(uint32_t *rid)
{
	int bit_offset;
	int i, j;

	printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
	printf("___________________________________________________\n");
	for (i = 0; i < 64; i++) {
		if (i < 32) {
			j = 0;
			bit_offset = i;
		} else {
			j = 1;
			bit_offset = i - 32;
		}
		if (i % 16 == 0)
			printf("%2x | ", i);
		printf("%d  ", (rid[j] >> bit_offset) & 0x1);
		if ((i + 1) % 16 == 0)
			printf("\n");
	}
}

static int otp_print_conf_image(struct otp_image_layout *image_layout)
{
	const struct otpconf_info *conf_info = info_cb.conf_info;
	uint32_t *OTPCFG = (uint32_t *)image_layout->conf;
	uint32_t *OTPCFG_IGNORE = (uint32_t *)image_layout->conf_ignore;
	uint32_t mask;
	uint32_t dw_offset;
	uint32_t bit_offset;
	uint32_t otp_value;
	uint32_t otp_ignore;
	int fail = 0;
	int mask_err;
	int rid_num = 0;
	char valid_bit[20];
	int fz;
	int i;
	int j;

	printf("DW    BIT        Value       Description\n");
	printf("__________________________________________________________________________\n");
	for (i = 0; i < info_cb.conf_info_len; i++) {
		mask_err = 0;
		dw_offset = conf_info[i].dw_offset;
		bit_offset = conf_info[i].bit_offset;
		mask = BIT(conf_info[i].length) - 1;
		otp_value = (OTPCFG[dw_offset] >> bit_offset) & mask;
		otp_ignore = (OTPCFG_IGNORE[dw_offset] >> bit_offset) & mask;

		if (conf_info[i].value == OTP_REG_VALID_BIT) {
			if (((otp_value + otp_ignore) & mask) != mask) {
				fail = 1;
				mask_err = 1;
			}
		} else {
			if (otp_ignore == mask) {
				continue;
			} else if (otp_ignore != 0) {
				fail = 1;
				mask_err = 1;
			}
		}

		if (otp_value != conf_info[i].value &&
		    conf_info[i].value != OTP_REG_RESERVED &&
		    conf_info[i].value != OTP_REG_VALUE &&
		    conf_info[i].value != OTP_REG_VALID_BIT)
			continue;
		printf("0x%-4X", dw_offset);

		if (conf_info[i].length == 1) {
			printf("0x%-9X", conf_info[i].bit_offset);
		} else {
			printf("0x%-2X:0x%-4X",
			       conf_info[i].bit_offset + conf_info[i].length - 1,
			       conf_info[i].bit_offset);
		}
		printf("0x%-10x", otp_value);

		if (mask_err) {
			printf("Ignore mask error\n");
			continue;
		}
		if (conf_info[i].value == OTP_REG_RESERVED) {
			printf("Reserved\n");
		} else if (conf_info[i].value == OTP_REG_VALUE) {
			printf(conf_info[i].information, otp_value);
			printf("\n");
		} else if (conf_info[i].value == OTP_REG_VALID_BIT) {
			if (otp_value != 0) {
				for (j = 0; j < 7; j++) {
					if (otp_value == (1 << j))
						valid_bit[j * 2] = '1';
					else
						valid_bit[j * 2] = '0';
					valid_bit[j * 2 + 1] = ' ';
				}
				valid_bit[15] = 0;
			} else {
				strcpy(valid_bit, "0 0 0 0 0 0 0 0\0");
			}
			printf(conf_info[i].information, valid_bit);
			printf("\n");
		} else {
			printf("%s\n", conf_info[i].information);
		}
	}

	if (OTPCFG[0xa] != 0 || OTPCFG[0xb] != 0) {
		if (OTPCFG_IGNORE[0xa] != 0 && OTPCFG_IGNORE[0xb] != 0) {
			printf("OTP revision ID is invalid.\n");
			fail = 1;
		} else {
			fz = 0;
			for (i = 0; i < 64; i++) {
				if (get_dw_bit(&OTPCFG[0xa], i) == 0) {
					if (!fz)
						fz = 1;
				} else {
					rid_num++;
					if (fz) {
						printf("OTP revision ID is invalid.\n");
						fail = 1;
						break;
					}
				}
			}
		}
		if (!fail)
			printf("OTP revision ID: 0x%x\n", rid_num);
		else
			printf("OTP revision ID\n");

		otp_print_revid(&OTPCFG[0xa]);
	}

	if (fail)
		return OTP_FAILURE;

	return OTP_SUCCESS;
}

static int otp_print_conf_info(int input_offset)
{
	const struct otpconf_info *conf_info = info_cb.conf_info;
	uint32_t OTPCFG[16];
	uint32_t mask;
	uint32_t dw_offset;
	uint32_t bit_offset;
	uint32_t otp_value;
	char valid_bit[20];
	int i;
	int j;

	otp_read_conf_buf(0, 16, OTPCFG);

	printf("DW    BIT        Value       Description\n");
	printf("__________________________________________________________________________\n");
	for (i = 0; i < info_cb.conf_info_len; i++) {
		if (input_offset != -1 && input_offset != conf_info[i].dw_offset)
			continue;
		dw_offset = conf_info[i].dw_offset;
		bit_offset = conf_info[i].bit_offset;
		mask = BIT(conf_info[i].length) - 1;
		otp_value = (OTPCFG[dw_offset] >> bit_offset) & mask;

		if (otp_value != conf_info[i].value &&
		    conf_info[i].value != OTP_REG_RESERVED &&
		    conf_info[i].value != OTP_REG_VALUE &&
		    conf_info[i].value != OTP_REG_VALID_BIT)
			continue;
		printf("0x%-4X", dw_offset);

		if (conf_info[i].length == 1) {
			printf("0x%-9X", conf_info[i].bit_offset);
		} else {
			printf("0x%-2X:0x%-4X",
			       conf_info[i].bit_offset + conf_info[i].length - 1,
			       conf_info[i].bit_offset);
		}
		printf("0x%-10x", otp_value);

		if (conf_info[i].value == OTP_REG_RESERVED) {
			printf("Reserved\n");
		} else if (conf_info[i].value == OTP_REG_VALUE) {
			printf(conf_info[i].information, otp_value);
			printf("\n");
		} else if (conf_info[i].value == OTP_REG_VALID_BIT) {
			if (otp_value != 0) {
				for (j = 0; j < 7; j++) {
					if (otp_value == (1 << j))
						valid_bit[j * 2] = '1';
					else
						valid_bit[j * 2] = '0';
					valid_bit[j * 2 + 1] = ' ';
				}
				valid_bit[15] = 0;
			} else {
				strcpy(valid_bit, "0 0 0 0 0 0 0 0\0");
			}
			printf(conf_info[i].information, valid_bit);
			printf("\n");
		} else {
			printf("%s\n", conf_info[i].information);
		}
	}
	return OTP_SUCCESS;
}

static int otp_print_strap_image(struct otp_image_layout *image_layout)
{
	const struct otpstrap_info *strap_info = info_cb.strap_info;
	uint32_t *OTPSTRAP;
	uint32_t *OTPSTRAP_REG_PRO;
	uint32_t *OTPSTRAP_PRO;
	uint32_t *OTPSTRAP_IGNORE;
	int i;
	int fail = 0;
	uint32_t bit_offset;
	uint32_t dw_offset;
	uint32_t mask;
	uint32_t otp_value;
	uint32_t otp_reg_protect;
	uint32_t otp_protect;
	uint32_t otp_ignore;

	OTPSTRAP = (uint32_t *)image_layout->strap;
	OTPSTRAP_PRO = (uint32_t *)image_layout->strap_pro;
	OTPSTRAP_IGNORE = (uint32_t *)image_layout->strap_ignore;
	if (info_cb.version == OTP_A0) {
		OTPSTRAP_REG_PRO = NULL;
		printf("BIT(hex)   Value       Protect     Description\n");
	} else {
		OTPSTRAP_REG_PRO = (uint32_t *)image_layout->strap_reg_pro;
		printf("BIT(hex)   Value       Reg_Protect Protect     Description\n");
	}
	printf("__________________________________________________________________________________________\n");

	for (i = 0; i < info_cb.strap_info_len; i++) {
		fail = 0;
		if (strap_info[i].bit_offset > 31) {
			dw_offset = 1;
			bit_offset = strap_info[i].bit_offset - 32;
		} else {
			dw_offset = 0;
			bit_offset = strap_info[i].bit_offset;
		}

		mask = BIT(strap_info[i].length) - 1;
		otp_value = (OTPSTRAP[dw_offset] >> bit_offset) & mask;
		otp_protect = (OTPSTRAP_PRO[dw_offset] >> bit_offset) & mask;
		otp_ignore = (OTPSTRAP_IGNORE[dw_offset] >> bit_offset) & mask;

		if (info_cb.version != OTP_A0)
			otp_reg_protect = (OTPSTRAP_REG_PRO[dw_offset] >> bit_offset) & mask;
		else
			otp_reg_protect = 0;

		if (otp_ignore == mask)
			continue;
		else if (otp_ignore != 0)
			fail = 1;

		if (otp_value != strap_info[i].value &&
		    strap_info[i].value != OTP_REG_RESERVED)
			continue;

		if (strap_info[i].length == 1) {
			printf("0x%-9X", strap_info[i].bit_offset);
		} else {
			printf("0x%-2X:0x%-4X",
			       strap_info[i].bit_offset + strap_info[i].length - 1,
			       strap_info[i].bit_offset);
		}
		printf("0x%-10x", otp_value);
		if (info_cb.version != OTP_A0)
			printf("0x%-10x", otp_reg_protect);
		printf("0x%-10x", otp_protect);

		if (fail) {
			printf("Ignore mask error\n");
		} else {
			if (strap_info[i].value != OTP_REG_RESERVED)
				printf("%s\n", strap_info[i].information);
			else
				printf("Reserved\n");
		}
	}

	if (fail)
		return OTP_FAILURE;

	return OTP_SUCCESS;
}

static bool is_reserved_bit(uint8_t bit) {
	static uint8_t reserved_bit[5] = {14, 30, 31, 61, 63};
	int i = 0;
	for (i = 0; i < sizeof(reserved_bit); i++) {
		if (bit == reserved_bit[i])
			return true;
	}
	return false;
}

static int otp_print_strap_info(int view)
{
	const struct otpstrap_info *strap_info = info_cb.strap_info;
	struct otpstrap_status strap_status[64];
	int i, j;
	int fail = 0;
	uint32_t bit_offset;
	uint32_t length;
	uint32_t otp_value;
	uint32_t otp_protect;

	otp_read_strap(strap_status);

	if (view) {
		if (info_cb.version == OTP_A0)
			printf("BIT(hex) Value  Remains  Protect   Description\n");
		else
			printf("BIT(hex) Value  Remains  Reg_Protect Protect   Description\n");
		printf("___________________________________________________________________________________________________\n");
	} else {
		printf("BIT(hex)   Value       Description\n");
		printf("________________________________________________________________________________\n");
	}
	for (i = 0; i < info_cb.strap_info_len; i++) {
		otp_value = 0;
		bit_offset = strap_info[i].bit_offset;
		length = strap_info[i].length;
		for (j = 0; j < length; j++) {
			otp_value |= strap_status[bit_offset + j].value << j;
			otp_protect |= strap_status[bit_offset + j].protected << j;
		}
		if (otp_value != strap_info[i].value &&
		    strap_info[i].value != OTP_REG_RESERVED)
			continue;
		if (view) {
			for (j = 0; j < length; j++) {
				printf("0x%-7X", strap_info[i].bit_offset + j);
				printf("0x%-5X", strap_status[bit_offset + j].value);
				printf("%-9d", strap_status[bit_offset + j].remain_times);
				if (info_cb.version != OTP_A0) {
					if (is_reserved_bit(bit_offset)) { // reserved bit doesn't map to any SCU strap bit
						printf("N/A         ");
					} else if (bit_offset < 32) { // SCU500
						printf("0x%-10X", strap_status[bit_offset + j + 1].reg_protected);
					} else { // SCU510
						printf("0x%-10X", strap_status[bit_offset + j].reg_protected);
					}
				}
				printf("0x%-7X", strap_status[bit_offset + j].protected);
				if (strap_info[i].value == OTP_REG_RESERVED) {
					printf(" Reserved\n");
					continue;
				}
				if (length == 1) {
					printf(" %s\n", strap_info[i].information);
					continue;
				}

				if (j == 0)
					printf("/%s\n", strap_info[i].information);
				else if (j == length - 1)
					printf("\\ \"\n");
				else
					printf("| \"\n");
			}
		} else {
			if (length == 1) {
				printf("0x%-9X", strap_info[i].bit_offset);
			} else {
				printf("0x%-2X:0x%-4X",
				       bit_offset + length - 1, bit_offset);
			}

			printf("0x%-10X", otp_value);

			if (strap_info[i].value != OTP_REG_RESERVED)
				printf("%s\n", strap_info[i].information);
			else
				printf("Reserved\n");
		}
	}

	if (fail)
		return OTP_FAILURE;

	return OTP_SUCCESS;
}

static int otp_print_data_image(struct otp_image_layout *image_layout)
{
	int key_id, key_offset, last, key_type, key_length, exp_length;
	const struct otpkey_type *key_info_array = info_cb.key_info;
	struct otpkey_type key_info;
	uint32_t *buf;
	uint8_t *byte_buf;
	char empty = 1;
	int i = 0, len = 0;
	int j;

	byte_buf = image_layout->data;
	buf = (uint32_t *)byte_buf;

	for (i = 0; i < 16; i++) {
		if (buf[i] != 0)
			empty = 0;
	}
	if (empty)
		return OTP_SUCCESS;

	i = 0;
	while (1) {
		key_id = buf[i] & 0x7;
		key_offset = buf[i] & 0x1ff8;
		last = (buf[i] >> 13) & 1;
		key_type = (buf[i] >> 14) & 0xf;
		key_length = (buf[i] >> 18) & 0x3;
		exp_length = (buf[i] >> 20) & 0xfff;

		for (j = 0; j < info_cb.key_info_len; j++) {
			if (key_type == key_info_array[j].value) {
				key_info = key_info_array[j];
				break;
			}
		}

		printf("\nKey[%d]:\n", i);
		printf("Key Type: ");
		printf("%s\n", key_info.information);

		if (key_info.key_type == OTP_KEY_TYPE_HMAC) {
			printf("HMAC SHA Type: ");
			switch (key_length) {
			case 0:
				printf("HMAC(SHA224)\n");
				break;
			case 1:
				printf("HMAC(SHA256)\n");
				break;
			case 2:
				printf("HMAC(SHA384)\n");
				break;
			case 3:
				printf("HMAC(SHA512)\n");
				break;
			}
		} else if (key_info.key_type == OTP_KEY_TYPE_RSA_PRIV ||
			   key_info.key_type == OTP_KEY_TYPE_RSA_PUB) {
			printf("RSA SHA Type: ");
			switch (key_length) {
			case 0:
				printf("RSA1024\n");
				len = 0x100;
				break;
			case 1:
				printf("RSA2048\n");
				len = 0x200;
				break;
			case 2:
				printf("RSA3072\n");
				len = 0x300;
				break;
			case 3:
				printf("RSA4096\n");
				len = 0x400;
				break;
			}
			printf("RSA exponent bit length: %d\n", exp_length);
		}
		if (key_info.need_id)
			printf("Key Number ID: %d\n", key_id);
		printf("Key Value:\n");
		if (key_info.key_type == OTP_KEY_TYPE_HMAC) {
			buf_print(&byte_buf[key_offset], 0x40);
		} else if (key_info.key_type == OTP_KEY_TYPE_AES) {
			printf("AES Key:\n");
			buf_print(&byte_buf[key_offset], 0x20);
			if (info_cb.version == OTP_A0) {
				printf("AES IV:\n");
				buf_print(&byte_buf[key_offset + 0x20], 0x10);
			}

		} else if (key_info.key_type == OTP_KEY_TYPE_VAULT) {
			if (info_cb.version == OTP_A0) {
				printf("AES Key:\n");
				buf_print(&byte_buf[key_offset], 0x20);
				printf("AES IV:\n");
				buf_print(&byte_buf[key_offset + 0x20], 0x10);
			} else {
				printf("AES Key 1:\n");
				buf_print(&byte_buf[key_offset], 0x20);
				printf("AES Key 2:\n");
				buf_print(&byte_buf[key_offset + 0x20], 0x20);
			}
		} else if (key_info.key_type == OTP_KEY_TYPE_RSA_PRIV) {
			printf("RSA mod:\n");
			buf_print(&byte_buf[key_offset], len / 2);
			printf("RSA exp:\n");
			buf_print(&byte_buf[key_offset + (len / 2)], len / 2);
		} else if (key_info.key_type == OTP_KEY_TYPE_RSA_PUB) {
			printf("RSA mod:\n");
			buf_print(&byte_buf[key_offset], len / 2);
			printf("RSA exp:\n");
			buf_print((uint8_t *)"\x01\x00\x01", 3);
		}
		if (last)
			break;
		i++;
	}
	return OTP_SUCCESS;
}

static int otp_strap_image_confirm(struct otp_image_layout *image_layout)
{
	int i;
	uint32_t *strap;
	uint32_t *strap_ignore;
	uint32_t *strap_reg_protect;
	uint32_t *strap_pro;
	int bit, pbit, ibit, rpbit;
	int fail = 0;
	int ret;
	struct otpstrap_status otpstrap[64];

	strap = (uint32_t *)image_layout->strap;
	strap_pro = (uint32_t *)image_layout->strap_pro;
	strap_ignore = (uint32_t *)image_layout->strap_ignore;
	strap_reg_protect = (uint32_t *)image_layout->strap_reg_pro;

	otp_read_strap(otpstrap);
	for (i = 0; i < 64; i++) {
		if (i < 32) {
			bit = (strap[0] >> i) & 0x1;
			ibit = (strap_ignore[0] >> i) & 0x1;
			pbit = (strap_pro[0] >> i) & 0x1;
		} else {
			bit = (strap[1] >> (i - 32)) & 0x1;
			ibit = (strap_ignore[1] >> (i - 32)) & 0x1;
			pbit = (strap_pro[1] >> (i - 32)) & 0x1;
		}

		if (info_cb.version != OTP_A0) {
			if (i < 32)
				rpbit = (strap_reg_protect[0] >> i) & 0x1;
			else
				rpbit = (strap_reg_protect[1] >> (i - 32)) & 0x1;
		} else {
			rpbit = 0;
		}
		ret = otp_strap_bit_confirm(&otpstrap[i], i, ibit, bit, pbit, rpbit);

		if (ret == OTP_FAILURE)
			fail = 1;
	}
	if (fail == 1)
		return OTP_FAILURE;
	else
		return OTP_SUCCESS;
}

static int otp_prog_data(struct otp_image_layout *image_layout)
{
	int i;
	int ret;
	int data_dw;
	uint32_t data[2048];
	uint32_t *buf;
	uint32_t *buf_ignore;

	uint32_t data_masked;
	uint32_t buf_masked;

	buf = (uint32_t *)image_layout->data;
	buf_ignore = (uint32_t *)image_layout->data_ignore;

	data_dw = 2048;

	printf("Read OTP Data:\n");

	// ignore last two dw, the last two dw is used for slt otp write check.
	otp_read_data_buf(0, data_dw - 2, data);

	printf("Check writable...\n");
	for (i = 0; i < data_dw - 2; i++) {
		data_masked = data[i]  & ~buf_ignore[i];
		buf_masked  = buf[i] & ~buf_ignore[i];
		if (data_masked == buf_masked)
			continue;
		if (i % 2 == 0) {
			if ((data_masked | buf_masked) == buf_masked) {
				continue;
			} else {
				printf("Input image can't program into OTP, please check.\n");
				printf("OTP_ADDR[%x] = %x\n", i, data[i]);
				printf("Input   [%x] = %x\n", i, buf[i]);
				printf("Mask    [%x] = %x\n", i, ~buf_ignore[i]);
				return OTP_FAILURE;
			}
		} else {
			if ((data_masked & buf_masked) == buf_masked) {
				continue;
			} else {
				printf("Input image can't program into OTP, please check.\n");
				printf("OTP_ADDR[%x] = %x\n", i, data[i]);
				printf("Input   [%x] = %x\n", i, buf[i]);
				printf("Mask    [%x] = %x\n", i, ~buf_ignore[i]);
				return OTP_FAILURE;
			}
		}
	}

	printf("Start Programing...\n");

	// programing ecc region first
	for (i = 1792; i < 2046; i++) {
		data_masked = data[i]  & ~buf_ignore[i];
		buf_masked  = buf[i] & ~buf_ignore[i];
		if (data_masked == buf_masked)
			continue;
		ret = otp_prog_data_dw(buf[i], buf_ignore[i], i);
		if (ret != OTP_SUCCESS) {
			printf("address: %08x, data: %08x, buffer: %08x mask: %08x\n",
			       i, data[i], buf[i], buf_ignore[i]);
			return ret;
		}
	}

	for (i = 0; i < 1792; i++) {
		data_masked = data[i]  & ~buf_ignore[i];
		buf_masked  = buf[i] & ~buf_ignore[i];
		if (data_masked == buf_masked)
			continue;
		ret = otp_prog_data_dw(buf[i], buf_ignore[i], i);
		if (ret != OTP_SUCCESS) {
			printf("address: %08x, data: %08x, buffer: %08x mask: %08x\n",
			       i, data[i], buf[i], buf_ignore[i]);
			return ret;
		}
	}
	return OTP_SUCCESS;
}

static int otp_prog_strap(struct otp_image_layout *image_layout)
{
	uint32_t *strap;
	uint32_t *strap_ignore;
	uint32_t *strap_pro;
	uint32_t *strap_reg_protect;
	uint32_t prog_address;
	int i;
	int bit, pbit, ibit, offset, rpbit, rp_offset = 0;
	int fail = 0;
	int ret;
	int prog_flag = 0;
	struct otpstrap_status otpstrap[64];

	strap = (uint32_t *)image_layout->strap;
	strap_pro = (uint32_t *)image_layout->strap_pro;
	strap_ignore = (uint32_t *)image_layout->strap_ignore;
	strap_reg_protect = (uint32_t *)image_layout->strap_reg_pro;

	printf("Read OTP Strap Region:\n");
	otp_read_strap(otpstrap);

	printf("Check writable...\n");
	if (otp_strap_image_confirm(image_layout) == OTP_FAILURE) {
		printf("Input image can't program into OTP, please check.\n");
		return OTP_FAILURE;
	}

	for (i = 0; i < 64; i++) {
		if (i < 32) {
			offset = i;
			rp_offset = i + 1;
			bit = (strap[0] >> offset) & 0x1;
			ibit = (strap_ignore[0] >> offset) & 0x1;
			pbit = (strap_pro[0] >> offset) & 0x1;
			prog_address = otpstrap[i].writeable_option * 2 + 16;

		} else {
			offset = (i - 32);
			rp_offset = (i - 32);
			bit = (strap[1] >> offset) & 0x1;
			ibit = (strap_ignore[1] >> offset) & 0x1;
			pbit = (strap_pro[1] >> offset) & 0x1;
			prog_address = otpstrap[i].writeable_option * 2 + 17;
		}
		if (info_cb.version != OTP_A0) {
			if (i == 30 || i == 31)
				rpbit = 0;
			else if (i < 32)
				rpbit = (strap_reg_protect[0] >> i) & 0x1;
			else
				rpbit = (strap_reg_protect[1] >> (i - 32)) & 0x1;
		} else {
			rpbit = 0;
		}

		if (ibit == 1)
			continue;
		if (bit == otpstrap[i].value)
			prog_flag = 0;
		else
			prog_flag = 1;

		if (otpstrap[i].protected == 1 && prog_flag) {
			fail = 1;
			continue;
		}
		if (otpstrap[i].remain_times == 0 && prog_flag) {
			fail = 1;
			continue;
		}
		if (prog_flag) {
			ret = otp_prog_conf_b(prog_address, offset, 1);
			if (ret)
				return OTP_FAILURE;
		}

		if (rpbit == 1 && info_cb.version != OTP_A0) {
			if (i < 32)
				prog_address = 28;
			else
				prog_address = 29;

			ret = otp_prog_conf_b(prog_address, rp_offset, 1);
			if (ret)
				return OTP_FAILURE;
		}

		if (pbit != 0) {
			if (i < 32)
				prog_address = 30;
			else
				prog_address = 31;

			ret = otp_prog_conf_b(prog_address, offset, 1);
			if (ret)
				return OTP_FAILURE;
		}
	}
	if (fail == 1)
		return OTP_FAILURE;
	else
		return OTP_SUCCESS;
}

static int otp_prog_conf(struct otp_image_layout *image_layout)
{
	int i, j;
	int pass = 0;
	uint32_t data[16];
	uint32_t *conf = (uint32_t *)image_layout->conf;
	uint32_t *conf_ignore = (uint32_t *)image_layout->conf_ignore;
	uint32_t data_masked;
	uint32_t buf_masked;

	printf("Read OTP configuration Region:\n");

	otp_read_conf_buf(0, 16, data);

	printf("Check writable...\n");
	for (i = 0; i < 16; i++) {
		data_masked = data[i]  & ~conf_ignore[i];
		buf_masked  = conf[i] & ~conf_ignore[i];
		if (data_masked == buf_masked)
			continue;
		if ((data_masked | buf_masked) == buf_masked) {
			continue;
		} else {
			printf("Input image can't program into OTP, please check.\n");
			printf("OTPCFG[%X] = %x\n", i, data[i]);
			printf("Input [%X] = %x\n", i, conf[i]);
			printf("Mask  [%X] = %x\n", i, ~conf_ignore[i]);
			return OTP_FAILURE;
		}
	}

	printf("Start Programing...\n");
	pass = 1;
	for (i = 0; i < 16; i++) {
		data_masked = data[i]  & ~conf_ignore[i];
		buf_masked  = conf[i] & ~conf_ignore[i];
		if (data_masked == buf_masked)
			continue;
		for (j = 0; j < 32; j++) {
			if ((conf_ignore[i] >> j) & 0x1)
				continue;
			if (!((buf_masked >> j) & 0x1))
				continue;
			if (otp_prog_conf_b(i, j, 1)) {
				pass = 0;
				break;
			}
		}
		if (pass == 0) {
			printf("address: %08x, data: %08x, buffer: %08x, mask: %08x\n",
			       i, data[i], conf[i], conf_ignore[i]);
			break;
		}
	}

	if (!pass)
		return OTP_FAILURE;

	return OTP_SUCCESS;
}

static int otp_verify_image(uint8_t *src_buf, uint32_t length, uint8_t *digest_buf)
{
	SHA256_CTX ctx;
	u8 digest_ret[CHECKSUM_LEN];

	sha256_init(&ctx);
	sha256_update(&ctx, src_buf, length);
	sha256_final(&ctx, digest_ret);

	if (!memcmp(digest_buf, digest_ret, CHECKSUM_LEN))
		return OTP_SUCCESS;
	else
		return OTP_FAILURE;
}

static int otp_prog_image(uint8_t *buf, int nconfirm)
{
	int ret;
	int image_version = 0;
	struct otp_header *otp_header;
	struct otp_image_layout image_layout;
	int image_size;
	uint8_t *checksum;

	otp_header = (struct otp_header *)buf;

	image_size = OTP_IMAGE_SIZE(otp_header->image_info);

	checksum = buf + otp_header->checksum_offset;

	if (strcmp(OTP_MAGIC, (char *)otp_header->otp_magic) != 0) {
		printf("Image is invalid\n");
		return OTP_FAILURE;
	}

	image_layout.data_length = (int)(OTP_REGION_SIZE(otp_header->data_info) / 2);
	image_layout.data = buf + OTP_REGION_OFFSET(otp_header->data_info);
	image_layout.data_ignore = image_layout.data + image_layout.data_length;

	image_layout.conf_length = (int)(OTP_REGION_SIZE(otp_header->conf_info) / 2);
	image_layout.conf = buf + OTP_REGION_OFFSET(otp_header->conf_info);
	image_layout.conf_ignore = image_layout.conf + image_layout.conf_length;

	image_layout.strap = buf + OTP_REGION_OFFSET(otp_header->strap_info);

	if (!strcmp("A0", (char *)otp_header->otp_version)) {
		image_version = OTP_A0;
		image_layout.strap_length = (int)(OTP_REGION_SIZE(otp_header->strap_info) / 3);
		image_layout.strap_pro = image_layout.strap + image_layout.strap_length;
		image_layout.strap_ignore = image_layout.strap + 2 * image_layout.strap_length;
	} else if (!strcmp("A1", (char *)otp_header->otp_version)) {
		image_version = OTP_A1;
		image_layout.strap_length = (int)(OTP_REGION_SIZE(otp_header->strap_info) / 4);
		image_layout.strap_reg_pro = image_layout.strap + image_layout.strap_length;
		image_layout.strap_pro = image_layout.strap + 2 * image_layout.strap_length;
		image_layout.strap_ignore = image_layout.strap + 3 * image_layout.strap_length;
	} else if (!strcmp("A2", (char *)otp_header->otp_version)) {
		image_version = OTP_A2;
		image_layout.strap_length = (int)(OTP_REGION_SIZE(otp_header->strap_info) / 4);
		image_layout.strap_reg_pro = image_layout.strap + image_layout.strap_length;
		image_layout.strap_pro = image_layout.strap + 2 * image_layout.strap_length;
		image_layout.strap_ignore = image_layout.strap + 3 * image_layout.strap_length;
	} else if (!strcmp("A3", (char *)otp_header->otp_version)) {
		image_version = OTP_A3;
		image_layout.strap_length = (int)(OTP_REGION_SIZE(otp_header->strap_info) / 4);
		image_layout.strap_reg_pro = image_layout.strap + image_layout.strap_length;
		image_layout.strap_pro = image_layout.strap + 2 * image_layout.strap_length;
		image_layout.strap_ignore = image_layout.strap + 3 * image_layout.strap_length;
	} else {
		puts("Version is not supported\n");
		return OTP_FAILURE;
	}

	if (image_version != info_cb.version) {
		puts("Version is not match\n");
		return OTP_FAILURE;
	}

	if (otp_verify_image(buf, image_size, checksum)) {
		puts("checksum is invalid\n");
		return OTP_FAILURE;
	}

	if (!nconfirm) {
		if (otp_header->image_info & OTP_INC_DATA) {
			printf("\nOTP data region :\n");
			if (otp_print_data_image(&image_layout) < 0) {
				printf("OTP data error, please check.\n");
				return OTP_FAILURE;
			}
		}
		if (otp_header->image_info & OTP_INC_STRAP) {
			printf("\nOTP strap region :\n");
			if (otp_print_strap_image(&image_layout) < 0) {
				printf("OTP strap error, please check.\n");
				return OTP_FAILURE;
			}
		}
		if (otp_header->image_info & OTP_INC_CONF) {
			printf("\nOTP configuration region :\n");
			if (otp_print_conf_image(&image_layout) < 0) {
				printf("OTP config error, please check.\n");
				return OTP_FAILURE;
			}
		}

		printf("type \"YES\" (no quotes) to continue:\n");
		if (!confirm_yesno()) {
			printf(" Aborting\n");
			return OTP_FAILURE;
		}
	}

	if (otp_header->image_info & OTP_INC_DATA) {
		printf("programing data region ...\n");
		ret = otp_prog_data(&image_layout);
		if (ret != 0) {
			printf("Error\n");
			return ret;
		}
		printf("Done\n");
	}
	if (otp_header->image_info & OTP_INC_CONF) {
		printf("programing configuration region ...\n");
		ret = otp_prog_conf(&image_layout);
		if (ret != 0) {
			printf("Error\n");
			return ret;
		}
		printf("Done\n");
	}
	if (otp_header->image_info & OTP_INC_STRAP) {
		printf("programing strap region ...\n");
		ret = otp_prog_strap(&image_layout);
		if (ret != 0) {
			printf("Error\n");
			return ret;
		}
		printf("Done\n");
	}

	return OTP_SUCCESS;
}

static int otp_prog_bit(int mode, int otp_dw_offset, int bit_offset, int value, int nconfirm)
{
	uint32_t read;
	struct otpstrap_status otpstrap[64];
	int otp_bit;
	int ret = 0;

	switch (mode) {
	case OTP_REGION_CONF:
		otp_read_conf(otp_dw_offset, &read);
		otp_bit = (read >> bit_offset) & 0x1;
		if (otp_bit == value) {
			printf("OTPCFG%X[%X] = %d\n", otp_dw_offset, bit_offset, value);
			printf("No need to program\n");
			return OTP_SUCCESS;
		}
		if (otp_bit == 1 && value == 0) {
			printf("OTPCFG%X[%X] = 1\n", otp_dw_offset, bit_offset);
			printf("OTP is programed, which can't be clean\n");
			return OTP_FAILURE;
		}
		printf("Program OTPCFG%X[%X] to 1\n", otp_dw_offset, bit_offset);
		break;
	case OTP_REGION_DATA:
		otp_read_data(otp_dw_offset, &read);
		otp_bit = (read >> bit_offset) & 0x1;
		if (otp_dw_offset % 2 == 0) {
			if (otp_bit == 1 && value == 0) {
				printf("OTPDATA%X[%X] = 1\n", otp_dw_offset, bit_offset);
				printf("OTP is programed, which can't be cleaned\n");
				return OTP_FAILURE;
			}
		} else {
			if (otp_bit == 0 && value == 1) {
				printf("OTPDATA%X[%X] = 1\n", otp_dw_offset, bit_offset);
				printf("OTP is programed, which can't be writen\n");
				return OTP_FAILURE;
			}
		}
		if (otp_bit == value) {
			printf("OTPDATA%X[%X] = %d\n", otp_dw_offset, bit_offset, value);
			printf("No need to program\n");
			return OTP_SUCCESS;
		}

		printf("Program OTPDATA%X[%X] to 1\n", otp_dw_offset, bit_offset);
		break;
	case OTP_REGION_STRAP:
		otp_read_strap(otpstrap);
		otp_print_strap(bit_offset, 1);
		ret = otp_strap_bit_confirm(&otpstrap[bit_offset], bit_offset, 0, value, 0, 0);
		if (ret == OTP_FAILURE)
			return OTP_FAILURE;
		else if (ret == OTP_PROG_SKIP)
			return OTP_SUCCESS;
		break;
	}

	if (!nconfirm) {
		printf("type \"YES\" (no quotes) to continue:\n");
		if (!confirm_yesno()) {
			printf(" Aborting\n");
			return OTP_FAILURE;
		}
	}

	switch (mode) {
	case OTP_REGION_STRAP:
		ret = otp_prog_strap_b(bit_offset, value);
		break;
	case OTP_REGION_CONF:
		ret = otp_prog_conf_b(otp_dw_offset, bit_offset, value);
		break;
	case OTP_REGION_DATA:
		ret = otp_prog_data_b(otp_dw_offset, bit_offset, value);
		break;
	}
	if (ret == OTP_SUCCESS) {
		printf("SUCCESS\n");
		return OTP_SUCCESS;
	}
	printf("OTP cannot be programed\n");
	printf("FAILED\n");
	return OTP_FAILURE;
}

static int otp_update_rid(uint32_t update_num, int force)
{
	uint32_t otp_rid[2];
	u32 sw_rid[2];
	int rid_num = 0;
	int sw_rid_num = 0;
	int bit_offset;
	int dw_offset;
	int i;
	int ret;

	if (otp_read_conf_buf(0xa, 2, otp_rid))
		return OTP_FAILURE;

	if (sw_revid(sw_rid))
		return OTP_FAILURE;

	rid_num = get_rid_num(otp_rid);
	sw_rid_num = get_rid_num(sw_rid);

	if (sw_rid_num < 0) {
		printf("SW revision id is invalid, please check.\n");
		return OTP_FAILURE;
	}

	if (update_num > sw_rid_num) {
		printf("current SW revision ID: 0x%x\n", sw_rid_num);
		printf("update number could not bigger than current SW revision id\n");
		return OTP_FAILURE;
	}

	if (rid_num < 0) {
		printf("Currennt OTP revision ID cannot handle by this command,\n"
		       "plase use 'otp pb' command to update it manually\n");
		otp_print_revid(otp_rid);
		return OTP_FAILURE;
	}

	printf("current OTP revision ID: 0x%x\n", rid_num);
	otp_print_revid(otp_rid);
	printf("input update number: 0x%X\n", update_num);

	if (rid_num > update_num) {
		printf("OTP rev_id is bigger than 0x%X\n", update_num);
		printf("Skip\n");
		return OTP_FAILURE;
	} else if (rid_num == update_num) {
		printf("OTP rev_id is same as input\n");
		printf("Skip\n");
		return OTP_FAILURE;
	}

	for (i = rid_num; i < update_num; i++) {
		if (i < 32) {
			dw_offset = 0xa;
			bit_offset = i;
		} else {
			dw_offset = 0xb;
			bit_offset = i - 32;
		}
		printf("OTPCFG%X[%d]", dw_offset, bit_offset);
		if (i + 1 != update_num)
			printf(", ");
	}

	printf(" will be programmed\n");
	if (force == 0) {
		printf("type \"YES\" (no quotes) to continue:\n");
		if (!confirm_yesno()) {
			printf(" Aborting\n");
			return OTP_FAILURE;
		}
	}

	ret = 0;
	for (i = rid_num; i < update_num; i++) {
		if (i < 32) {
			dw_offset = 0xa;
			bit_offset = i;
		} else {
			dw_offset = 0xb;
			bit_offset = i - 32;
		}
		if (otp_prog_conf_b(dw_offset, bit_offset, 1)) {
			printf("OTPCFG%X[%d] programming failed\n", dw_offset, bit_offset);
			ret = OTP_FAILURE;
			break;
		}
	}

	otp_read_conf_buf(0xa, 2, otp_rid);
	rid_num = get_rid_num(otp_rid);
	if (rid_num >= 0)
		printf("OTP revision ID: 0x%x\n", rid_num);
	else
		printf("OTP revision ID\n");
	otp_print_revid(otp_rid);
	if (!ret)
		printf("SUCCESS\n");
	else
		printf("FAILED\n");
	return ret;
}

static int do_otpread(int argc, char *const argv[])
{
	uint32_t offset, count;
	int ret;

	if (argc == 4) {
		offset = strtoul(argv[2], NULL, 16);
		count = strtoul(argv[3], NULL, 16);
	} else if (argc == 3) {
		offset = strtoul(argv[2], NULL, 16);
		count = 1;
	} else {
		return OTP_USAGE;
	}

	if (!strcmp(argv[1], "conf")) {
		if (offset + count > 32)
			return OTP_USAGE;
		ret = otp_print_conf(offset, count);
	} else if (!strcmp(argv[1], "data")) {
		if (offset + count > 2048)
			return OTP_USAGE;
		ret = otp_print_data(offset, count);
	} else if (!strcmp(argv[1], "strap")) {
		if (offset + count > 64)
			return OTP_USAGE;
		ret = otp_print_strap(offset, count);
	} else {
		return OTP_USAGE;
	}

	return ret;
}

static int do_otpprog(int argc, char *const argv[])
{
	FILE *fd;
	int ret;
	int force = 0;
	char *path;
	uint8_t *buf;
	long fsize;

	if (argc == 3) {
		if (strcmp(argv[1], "o"))
			return OTP_USAGE;
		path = argv[2];
		force = 1;
	} else if (argc == 2) {
		path = argv[1];
		force = 0;
	} else {
		return OTP_USAGE;
	}
	fd = fopen(path, "rb");
	if (!fd) {
		printf("failed to open %s\n", path);
		return OTP_FAILURE;
	}
	fseek(fd, 0, SEEK_END);
	fsize = ftell(fd);
	fseek(fd, 0, SEEK_SET);
	buf = malloc(fsize + 1);
	ret = fread(buf, 1, fsize, fd);
	if (ret != fsize) {
		printf("Reading \"%s\" failed\n", path);
		return OTP_FAILURE;
	}
	fclose(fd);
	buf[fsize] = 0;

	return otp_prog_image(buf, force);
}

static int do_otppb(int argc, char *const argv[])
{
	int mode = 0;
	int nconfirm = 0;
	int otp_addr = 0;
	int bit_offset;
	int value;

	if (argc != 4 && argc != 5 && argc != 6)
		return OTP_USAGE;

	/* Drop the pb cmd */
	argc--;
	argv++;

	if (!strcmp(argv[0], "conf"))
		mode = OTP_REGION_CONF;
	else if (!strcmp(argv[0], "strap"))
		mode = OTP_REGION_STRAP;
	else if (!strcmp(argv[0], "data"))
		mode = OTP_REGION_DATA;
	else
		return OTP_USAGE;

	/* Drop the region cmd */
	argc--;
	argv++;

	if (!strcmp(argv[0], "o")) {
		nconfirm = 1;
		/* Drop the force option */
		argc--;
		argv++;
	}

	if (mode == OTP_REGION_STRAP) {
		if (argc != 2)
			return OTP_USAGE;
		bit_offset = strtoul(argv[0], NULL, 16);
		value = strtoul(argv[1], NULL, 16);
		if (bit_offset >= 64 || (value != 0 && value != 1))
			return OTP_USAGE;
	} else {
		if (argc != 3)
			return OTP_USAGE;
		otp_addr = strtoul(argv[0], NULL, 16);
		bit_offset = strtoul(argv[1], NULL, 16);
		value = strtoul(argv[2], NULL, 16);
		if (bit_offset >= 32 || (value != 0 && value != 1))
			return OTP_USAGE;
		if (mode == OTP_REGION_DATA) {
			if (otp_addr >= 0x800)
				return OTP_USAGE;
		} else {
			if (otp_addr >= 0x20)
				return OTP_USAGE;
		}
	}
	if (value != 0 && value != 1)
		return OTP_USAGE;

	return otp_prog_bit(mode, otp_addr, bit_offset, value, nconfirm);
}

static int do_otpinfo(int argc, char *const argv[])
{
	int view = 0;
	int input;

	if (argc != 2 && argc != 3)
		return OTP_USAGE;

	if (!strcmp(argv[1], "conf")) {
		if (argc == 3) {
			input = strtoul(argv[2], NULL, 16);
			otp_print_conf_info(input);
		} else {
			otp_print_conf_info(-1);
		}
	} else if (!strcmp(argv[1], "strap")) {
		if (argc == 3) {
			if (!strcmp(argv[2], "v")) {
				view = 1;
				/* Drop the view option */
				argc--;
				argv++;
			} else {
				return OTP_USAGE;
			}
		}
		otp_print_strap_info(view);
	} else {
		return OTP_USAGE;
	}

	return OTP_SUCCESS;
}

static int _do_otpprotect(int argc, char *const argv[], int preg)
{
	int input;
	int bit_offset;
	int prog_address;
	int ret = OTP_SUCCESS;
	uint32_t read;
	char info[10];

	if (preg) {
		sprintf(info, "register ");
		prog_address = 28;
	} else {
		info[0] = 0;
		prog_address = 30;
	}

	if (argc == 3) {
		if (strcmp(argv[1], "o"))
			return OTP_USAGE;
		input = strtoul(argv[2], NULL, 16);
	} else if (argc == 2) {
		input = strtoul(argv[1], NULL, 16);
		printf("OTPSTRAP[%d] %swill be protected\n", input, info);
		printf("type \"YES\" (no quotes) to continue:\n");
		if (!confirm_yesno()) {
			printf(" Aborting\n");
			return OTP_FAILURE;
		}
	} else {
		return OTP_USAGE;
	}

	if (input < 30) {
		bit_offset = input + 1;
	} else if (input < 32) {
		printf("OTPSTRAP[%d] is reserved register\n", input);
		return OTP_FAILURE;
	} else if (input < 64) {
		bit_offset = input - 32;
		prog_address++;
	} else {
		return OTP_USAGE;
	}

	otp_read_conf(prog_address, &read);
	if (((read >> bit_offset) & 1) == 1) {
		printf("OTPSTRAP[%d] %salready protected\n", input, info);
		return OTP_SUCCESS;
	}
	
	ret = otp_prog_conf_b(prog_address, bit_offset, 1);

	if (ret == OTP_SUCCESS)
		printf("OTPSTRAP[%d] %sis protected\n", input, info);
	else
		printf("Protect OTPSTRAP[%d] %sfail\n", input, info);

	return ret;
}

static int do_otpprotect(int argc, char *const argv[])
{
	return _do_otpprotect(argc, argv, 0);
}

static int do_otprprotect(int argc, char *const argv[])
{
	if (info_cb.version == OTP_A0) {
		printf("A0 is not support OTPSTRAP register protection\n");
		return OTP_FAILURE;
	}
	return _do_otpprotect(argc, argv, 1);
}

static int do_otpver(char *ver_name)
{
	printf("SOC OTP version: %s\n", ver_name);
	printf("OTP tool version: %s\n", OTP_VER);
	printf("OTP info version: %s\n", OTP_INFO_VER);

	return OTP_SUCCESS;
}

static int do_otpupdate(int argc, char *const argv[])
{
	uint32_t update_num;
	int force = 0;

	if (argc == 3) {
		if (strcmp(argv[1], "o"))
			return OTP_USAGE;
		force = 1;
		update_num = strtoul(argv[2], NULL, 16);
	} else if (argc == 2) {
		update_num = strtoul(argv[1], NULL, 16);
	} else {
		return OTP_USAGE;
	}

	if (update_num > 64)
		return OTP_USAGE;

	return otp_update_rid(update_num, force);
}

static int do_otprid(int argc, char *const argv[])
{
	uint32_t otp_rid[2];
	u32 sw_rid[2];
	int rid_num = 0;
	int sw_rid_num = 0;
	int ret;

	if (argc != 1)
		return OTP_USAGE;

	if (otp_read_conf_buf(0xa, 2, otp_rid))
		return OTP_FAILURE;

	if (sw_revid(sw_rid))
		return OTP_FAILURE;

	rid_num = get_rid_num(otp_rid);
	sw_rid_num = get_rid_num(sw_rid);

	printf("current SW revision ID: 0x%x\n", sw_rid_num);
	if (rid_num >= 0) {
		printf("current OTP revision ID: 0x%x\n", rid_num);
		ret = OTP_SUCCESS;
	} else {
		printf("Currennt OTP revision ID cannot handle by 'otp update',\n"
		       "plase use 'otp pb' command to update it manually\n"
		       "current OTP revision ID\n");
		ret = OTP_FAILURE;
	}
	otp_print_revid(otp_rid);

	return ret;
}

static void usage(void)
{
	printf("otp version\n"
	       "otp read conf|data <otp_dw_offset> <dw_count>\n"
	       "otp read strap <strap_bit_offset> <bit_count>\n"
	       "otp info strap [v]\n"
	       "otp info conf [otp_dw_offset]\n"
	       "otp prog [o] <image_path>\n"
	       "otp pb conf|data [o] <otp_dw_offset> <bit_offset> <value>\n"
	       "otp pb strap [o] <bit_offset> <value>\n"
	       "otp protect [o] <bit_offset>\n"
	       "otp rprotect [o] <bit_offset>\n"
	       "otp update [o] <revision_id>\n"
	       "otp rid\n");
}

int main(int argc, char *argv[])
{
	char *sub_cmd;
	uint32_t ver;
	int ret;
	char ver_name[15];

	if (argc < 2 || argc > 7) {
		usage();
		exit(EXIT_FAILURE);
	}

	info_cb.otp_fd = open("/dev/aspeed-otp", O_RDWR);
	if (info_cb.otp_fd == -1) {
		printf("Can't open /dev/aspeed-otp, please install driver!!\n");
		exit(EXIT_FAILURE);
	}

	sub_cmd = argv[1];

	/* Drop the otp command */
	argc--;
	argv++;

	ver = chip_version();
	ret = 0;
	switch (ver) {
	case OTP_A0:
		info_cb.version = OTP_A0;
		info_cb.conf_info = a0_conf_info;
		info_cb.conf_info_len = ARRAY_SIZE(a0_conf_info);
		info_cb.strap_info = a0_strap_info;
		info_cb.strap_info_len = ARRAY_SIZE(a0_strap_info);
		info_cb.key_info = a0_key_type;
		info_cb.key_info_len = ARRAY_SIZE(a0_key_type);
		sprintf(ver_name, "A0");
		break;
	case OTP_A1:
		info_cb.version = OTP_A1;
		info_cb.conf_info = a1_conf_info;
		info_cb.conf_info_len = ARRAY_SIZE(a1_conf_info);
		info_cb.strap_info = a1_strap_info;
		info_cb.strap_info_len = ARRAY_SIZE(a1_strap_info);
		info_cb.key_info = a1_key_type;
		info_cb.key_info_len = ARRAY_SIZE(a1_key_type);
		sprintf(ver_name, "A1");
		break;
	case OTP_A2:
		info_cb.version = OTP_A2;
		info_cb.conf_info = a2_conf_info;
		info_cb.conf_info_len = ARRAY_SIZE(a2_conf_info);
		info_cb.strap_info = a2_strap_info;
		info_cb.strap_info_len = ARRAY_SIZE(a2_strap_info);
		info_cb.key_info = a2_key_type;
		info_cb.key_info_len = ARRAY_SIZE(a2_key_type);
		sprintf(ver_name, "A2");
		break;
	case OTP_A3:
		info_cb.version = OTP_A3;
		info_cb.conf_info = a3_conf_info;
		info_cb.conf_info_len = ARRAY_SIZE(a3_conf_info);
		info_cb.strap_info = a3_strap_info;
		info_cb.strap_info_len = ARRAY_SIZE(a3_strap_info);
		info_cb.key_info = a3_key_type;
		info_cb.key_info_len = ARRAY_SIZE(a3_key_type);
		sprintf(ver_name, "A3");
		break;
	default:
		sprintf(ver_name, "unrecognized");
		ret = EXIT_FAILURE;
	}

	if (!strcmp(sub_cmd, "version")) {
		do_otpver(ver_name);
		return EXIT_SUCCESS;
	}

	if (ret) {
		printf("SOC is not supported\n");
		return ret;
	}

	if (!strcmp(sub_cmd, "read"))
		ret = do_otpread(argc, argv);
	else if (!strcmp(sub_cmd, "info"))
		ret = do_otpinfo(argc, argv);
	else if (!strcmp(sub_cmd, "pb"))
		ret = do_otppb(argc, argv);
	else if (!strcmp(sub_cmd, "protect"))
		ret = do_otpprotect(argc, argv);
	else if (!strcmp(sub_cmd, "rprotect"))
		ret = do_otprprotect(argc, argv);
	else if (!strcmp(sub_cmd, "prog"))
		ret = do_otpprog(argc, argv);
	else if (!strcmp(sub_cmd, "update"))
		ret = do_otpupdate(argc, argv);
	else if (!strcmp(sub_cmd, "rid"))
		ret = do_otprid(argc, argv);
	else
		ret = OTP_USAGE;

	if (ret == OTP_USAGE) {
		usage();
		return EXIT_FAILURE;
	} else if (ret == OTP_FAILURE) {
		return EXIT_FAILURE;
	} else {
		return EXIT_SUCCESS;
	}
}
