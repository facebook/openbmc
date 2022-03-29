#include <stdio.h>
#include <stdint.h>
#include "dimm-util.h"

#define CHANNEL_NUM(x) (x + 1)

static const uint16_t die_capacity_mb[] = {
  256,
  512,
  1024,
  2048,
  4096,
  8192,
  16384,
  32768,
  12288,  // 12 Gb
  24576,  // 24 Gb
};

static const uint16_t bus_width_bits[] = {
  8,
  16,
  32,
  64,
};

static const uint16_t device_width[] = {
  4,
  8,
  16,
  32,
};

static const uint16_t sdram_size_gb[] = {
  0,
  4,
  8,
  12,
  16,
  24,
  32,
  48,
  64,
};

static const uint16_t logical_rank[] = {
  1,
  1,
  2,
  4,
  8,
  16,
};

typedef struct _max_dimm_speed {
  uint16_t min_cycle;
  uint16_t speed;
} max_dimm_speed;
static max_dimm_speed spd5_dimm_speed[] = {
  { 0x271, 3200 },
  { 0x22b, 3600 },
  { 0x1f4, 4000 },
  { 0x1c6, 4400 },
  { 0x1a0, 4800 },
  { 0x180, 5200 },
  { 0x165, 5600 },
  { 0x14d, 6000 },
  { 0x138, 6400 },
  { 0x126, 6800 },
  { 0x115, 7200 },
  { 0x107, 7600 },
  { 0x0fa, 8000 },
  { 0x0ee, 8400 },
};

int get_die_capacity(uint8_t data) {
  if ((data >= ARRAY_SIZE(die_capacity_mb)) || (die_capacity_mb[data] == 0)) {
    return -1;
  } else {
    return die_capacity_mb[data];
  }
}

int get_bus_width_bits(uint8_t data) {
  if ((data >= ARRAY_SIZE(bus_width_bits)) || (bus_width_bits[data] == 0)) {
    return -1;
  } else {
    return bus_width_bits[data];
  }
}

int get_device_width_bits(uint8_t data) {
  if ((data >= ARRAY_SIZE(device_width)) || (device_width[data] == 0)) {
    return -1;
  } else {
    return device_width[data];
  }
}

static int get_sdram_size_gb(uint8_t data) {
  if ((data >= ARRAY_SIZE(sdram_size_gb)) || (data == 0)) {
    return -1;
  }
  return sdram_size_gb[data];
}

static int get_logical_rank(uint8_t data) {
  if (data >= ARRAY_SIZE(logical_rank)) {
    return -1;
  }
  return logical_rank[data];
}

int
get_spd5_dimm_size(uint8_t fru_id, uint8_t cpu, uint8_t dimm, char *size_str) {
  uint8_t size_gb_raw;
  uint8_t ch_num_raw;
  uint8_t log_rank_raw, pack_rank_raw;
  uint8_t bus_width_raw, dev_width_raw;
  uint8_t buf[16] = {0}, dimm_present = 0;
  int dimm_size = 0;

  util_read_spd_with_retry(fru_id, cpu, dimm, 0x4, 3, 0, buf, &dimm_present);
  if (!dimm_present) {
    snprintf(size_str, LEN_SIZE_STRING, "Unknown");
    return -1;
  }
  size_gb_raw = buf[0] & 0xF;           // byte4[3:0]
  log_rank_raw = (buf[0] >> 5) & 0x7;   // byte4[7:5]
  dev_width_raw = (buf[2] >> 5) & 0x7;  // byte6[7:5]

  util_read_spd_with_retry(fru_id, cpu, dimm, 0xea, 2, 0, buf, &dimm_present);
  if (!dimm_present) {
    snprintf(size_str, LEN_SIZE_STRING, "Unknown");
    return -1;
  }
  pack_rank_raw = (buf[0] >> 3) & 0x7;  // byte234[5:3]
  bus_width_raw = buf[1] & 0x7;         // byte235[2:0]
  ch_num_raw = (buf[1] >> 5) & 0x3;     // byte235[6:5]

  int size_gb   = get_sdram_size_gb(size_gb_raw);
  int bus_width = get_bus_width_bits(bus_width_raw);
  int dev_width = get_device_width_bits(dev_width_raw);
  int ranks     = get_logical_rank(log_rank_raw) * PKG_RANK(pack_rank_raw);
  int channels  = CHANNEL_NUM(ch_num_raw);

  dimm_size = (size_gb * ranks * channels * bus_width * 1024) / (8 * dev_width);
  snprintf(size_str, LEN_SIZE_STRING, "%d MB", dimm_size);

  return 0;
}

int
get_spd5_dimm_speed(uint8_t fru_id, uint8_t cpu, uint8_t dimm, char *speed_str) {
  uint8_t buf[16] = {0}, dimm_present = 0, i;
  uint16_t min_cycle = 0;

  util_read_spd_with_retry(fru_id, cpu, dimm, 0x14, 2, 0, buf, &dimm_present);
  if (!dimm_present) {
    snprintf(speed_str, LEN_SPEED_STRING, "Unknown");
    return -1;
  }
  min_cycle = (buf[1] << 8) | buf[0];  // (byte21 << 8) | byte20

  for (i = 0; i < ARRAY_SIZE(spd5_dimm_speed); i++) {
    if (min_cycle >= spd5_dimm_speed[i].min_cycle) {
      snprintf(speed_str, LEN_SPEED_STRING, "%u MT/s", spd5_dimm_speed[i].speed);
      break;
    }
  }
  if (i >= ARRAY_SIZE(spd5_dimm_speed)) {
    snprintf(speed_str, LEN_SPEED_STRING, "Unknown");
    return -1;
  }

  return 0;
}
