// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "test-defs.h"

void test_crc16(struct test_stats *stats)
{
	uint8_t buf[] = {0x0c, 0x03, 0x90, 0x00, 0x00, 0x01};
	uint16_t got = crc16(buf, sizeof(buf));
	uint16_t expected = 0xa817;

	if (got != expected) {
		LOG_DEBUG("crc16 got 0x%04x, expected 0x%04x\n", got, expected);
		stats->num_errors++;
		return;
	}
	stats->num_total++;
}

void test_lrc8(struct test_stats *stats)
{
	uint8_t buf[] = {0x0c, 0x03, 0x90, 0x00, 0x00, 0x01};
	uint8_t got = lrc8(buf, sizeof(buf));
	uint8_t expected = 0x60;

	if (got != expected) {
		LOG_DEBUG("lrc8 got 0x%02x, expected 0x%02x\n", got, expected);
		stats->num_errors++;
		return;
	}
	stats->num_total++;
}
