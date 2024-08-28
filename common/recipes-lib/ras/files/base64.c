#include "base64.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

static const uint8_t encode_table[65] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 *
 * Caller is responsible for freeing the returned buffer.
 */
char *base64_encode(const uint8_t *src, int32_t len, int32_t *out_len)
{
	char *out;
	char *out_pos;
	const uint8_t *src_end;
	const uint8_t *in_pos;

	if (!out_len) {
		return NULL;
	}

	// 3 byte blocks to 4 byte blocks plus up to 2 bytes of padding
	*out_len = 4 * ((len + 2) / 3);

	// Handle overflows
	if (*out_len < len) {
		return NULL;
	}

	out = malloc(*out_len);
	if (out == NULL) {
		return NULL;
	}

	src_end = src + len;
	in_pos = src;
	out_pos = out;
	while (src_end - in_pos >= 3) {
		*out_pos++ = encode_table[in_pos[0] >> 2];
		*out_pos++ = encode_table[((in_pos[0] & 0x03) << 4) |
					  (in_pos[1] >> 4)];
		*out_pos++ = encode_table[((in_pos[1] & 0x0f) << 2) |
					  (in_pos[2] >> 6)];
		*out_pos++ = encode_table[in_pos[2] & 0x3f];
		in_pos += 3;
	}

	if (src_end - in_pos) {
		*out_pos++ = encode_table[in_pos[0] >> 2];
		if (src_end - in_pos == 1) {
			*out_pos++ = encode_table[(in_pos[0] & 0x03) << 4];
			*out_pos++ = '=';
		} else {
			*out_pos++ = encode_table[((in_pos[0] & 0x03) << 4) |
						  (in_pos[1] >> 4)];
			*out_pos++ = encode_table[(in_pos[1] & 0x0f) << 2];
		}
		*out_pos++ = '=';
	}

	return out;
}

// Base64 decode table.  Invalid values are specified with 0x80.
uint8_t decode_table[256] =
	"\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80"
	"\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80"
	"\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x3e\x80\x80\x80\x3f"
	"\x34\x35\x36\x37\x38\x39\x3a\x3b\x3c\x3d\x80\x80\x80\x00\x80\x80"
	"\x80\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e"
	"\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x80\x80\x80\x80\x80"
	"\x80\x1a\x1b\x1c\x1d\x1e\x1f\x20\x21\x22\x23\x24\x25\x26\x27\x28"
	"\x29\x2a\x2b\x2c\x2d\x2e\x2f\x30\x31\x32\x33\x80\x80\x80\x80\x80"
	"\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80"
	"\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80"
	"\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80"
	"\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80"
	"\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80"
	"\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80"
	"\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80"
	"\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80";

/**
 *
 * Caller is responsible for freeing the returned buffer.
 */
uint8_t *base64_decode(const char *src, int32_t len, int32_t *out_len)
{
	uint8_t *out;
	uint8_t *pos;
	uint8_t block[4];
	uint8_t tmp;
	int32_t block_index;
	int32_t src_index;
	uint32_t pad_count = 0;

	if (!out_len) {
		return NULL;
	}

	// Malloc might be up to 2 larger dependent on padding
	*out_len = len / 4 * 3;
	pos = out = malloc(*out_len);
	if (out == NULL) {
		return NULL;
	}

	block_index = 0;
	for (src_index = 0; src_index < len; src_index++) {
		tmp = decode_table[(uint8_t)src[src_index]];
		if (tmp == 0x80) {
			free(out);
			return NULL;
		}

		if (src[src_index] == '=') {
			pad_count++;
		}

		block[block_index] = tmp;
		block_index++;
		if (block_index == 4) {
			*pos++ = (block[0] << 2) | (block[1] >> 4);
			*pos++ = (block[1] << 4) | (block[2] >> 2);
			*pos++ = (block[2] << 6) | block[3];
			if (pad_count > 0) {
				if (pad_count == 1) {
					pos--;
				} else if (pad_count == 2) {
					pos -= 2;
				} else {
					/* Invalid pad_counting */
					free(out);
					return NULL;
				}
				break;
			}
			block_index = 0;
		}
	}

	*out_len = pos - out;
	return out;
}
