#include <stdlib.h>
#include <stdint.h>
#include "dimm-util.h"


static const uint16_t die_capacity_mb[] =
{
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


int get_die_capacity(uint8_t data) {
  if ((data < 0) ||
      (data >= ARRAY_SIZE(die_capacity_mb)) ||
      (die_capacity_mb[data] == 0)) {
    return -1;
  } else {
    return die_capacity_mb[data];
  }
}


int get_bus_width_bits(uint8_t data) {
  if ((data < 0) ||
      (data >= ARRAY_SIZE(bus_width_bits)) ||
      (bus_width_bits[data] == 0)) {
    return -1;
  } else {
    return bus_width_bits[data];
  }
}


int get_device_width_bits(uint8_t data) {
  if ((data < 0) ||
      (data >= ARRAY_SIZE(device_width)) ||
      (device_width[data] == 0)) {
    return -1;
  } else {
    return device_width[data];
  }
}

int get_package_rank(uint8_t data) {
  if (data < 0) {
    return -1;
  } else {
    return data + 1;
  }
}
