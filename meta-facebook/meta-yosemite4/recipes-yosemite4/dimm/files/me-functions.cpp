// me-functions.cpp (Yosemite4, no libbic; read SPD via pldmtool raw)

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

#include <jansson.h>
#include <openbmc/kv.h>
#include <openbmc/ipmi.h>

#include "dimm.h"

#ifdef DEBUG_DIMM_BASE
  #define DBG_PRINT(...) printf(__VA_ARGS__)
#else
  #define DBG_PRINT(...)
#endif

#ifndef SPD_C_OEM_PRESENT
#define SPD_C_OEM_PRESENT 0x4E   // 1 = DIMM present, 0 = no DIMM
#endif
#ifndef SPD_C_OEM_STATUS
#define SPD_C_OEM_STATUS  0x4F   // bit0 = SPD compact ready
#endif
#ifndef SPD_READY_BIT
#define SPD_READY_BIT     0
#endif

// ---- Yosemite 4 plat_init(): 8 FRU × 1 CPU × 12 DIMM ----
extern bool read_block;
const char* get_dimm_label(uint8_t /*cpu*/, uint8_t dimm) {
  static const char* labels[12] = {
    "A0","A1","A2","A3","A4","A5","A6","A7","A8","A9","A10","A11"
  };
  return (dimm < 12) ? labels[dimm] : "--";
}
extern bool read_block;
static inline uint8_t slot_to_eid(uint8_t slot_id);
static bool run_pldmtool_and_get_rx(const std::string& cmd, std::vector<uint8_t>& out);
int plat_init(void)
{
  // === FRU: slot1..slot8 ===
  static const char* yv4_fru_names[] = {
    "slot1","slot2","slot3","slot4","slot5","slot6","slot7","slot8"
  };
  fru_name   = yv4_fru_names;
  num_frus   = 8;
  fru_id_min = 1;
  fru_id_max = 8;
  fru_id_all = 9;


  num_cpus          = 1;
  num_dimms_per_cpu = 12;
  total_dimms       = num_cpus * num_dimms_per_cpu;

  read_block = true;
  return 0;
}

struct Slice { uint16_t off; uint16_t len; };

static const Slice k_ddr5_slices[] = {
  {0x0002, 1},
  {0x0004, 1},
  {0x0006, 1},
  {0x0014, 2},
  {0x00C6, 2},
  {0x00EA, 1},
  {0x00EB, 1},
  {0x00F0, 2},
  {0x0200, 0x27}, // 0x0200..0x0226, 39 bytes
};

static inline bool overlap(uint16_t a0, uint16_t al, uint16_t b0, uint16_t bl,
                           uint16_t* out_off_in_a, uint16_t* out_off_in_b, uint16_t* out_len) {
  uint32_t a1 = (uint32_t)a0 + al;
  uint32_t b1 = (uint32_t)b0 + bl;
  uint32_t s  = (a0 > b0) ? a0 : b0;
  uint32_t e  = (a1 < b1) ? a1 : b1;
  if (e <= s) {
    return false;
  }
  *out_off_in_a = (uint16_t)(s - a0);
  *out_off_in_b = (uint16_t)(s - b0);
  *out_len      = (uint16_t)(e - s);
  return true;
}
// 讀 0x50 bytes 的 Compact SPD
static int read_spd_compact(uint8_t eid, uint8_t dimm_id, uint8_t out[0x50]) {
  if (!out){
    return -1;
  }
  char cmd[256];
  // device_type = 0x03, len = 0x50, offset = 0x0000
  snprintf(cmd, sizeof(cmd),
           "pldmtool raw -m %u -d "
           "0x80 0x3F 0x01 0x15 0xA0 0x00 "
           "0xE0 0xB1 0x15 0xA0 0x00 "
           "0x%02X 0x03 0x%02X 0x%02X 0x%02X",
           eid, dimm_id, 0x50, 0x00, 0x00);

  std::vector<uint8_t> rx;
  if (!run_pldmtool_and_get_rx(cmd, rx))
  {
    return -1;
  }
  const uint8_t pat[] = {0x00, 0x15, 0xA0, 0x00};
  ssize_t idx = -1;
  for (size_t i = 0; i + sizeof(pat) <= rx.size(); ++i) {
    bool ok = true;
    for (size_t j = 0; j < sizeof(pat); ++j) {
      if (rx[i+j] != pat[j]) { ok = false; break; }
    }
    if (ok) idx = (ssize_t)i + (ssize_t)sizeof(pat);
  }
  if (idx < 0 || (rx.size() - (size_t)idx) < 0x50) {
    if (rx.size() < 0x50) return -1;
    idx = (ssize_t)rx.size() - 0x50;
  }

  memcpy(out, rx.data() + idx, 0x50);
  return 0;
}

struct MapCR { uint16_t raw_off; uint16_t comp_off; uint16_t len; };
static const MapCR k_compact_to_raw[] = {
  {0x0002, 0x00, 1},        // Type (DDR5=0x12)
  {0x0004, 0x01, 1},        // First SDRAM Density and Package
  {0x0006, 0x02, 1},        // First SDRAM I/O Width
  {0x0014, 0x03, 2},        // Speed (tCKmin)
  {0x00C6, 0x05, 2},        // PMIC Vendor
  {0x00EA, 0x09, 1},        // pkg/rank
  {0x00EB, 0x0A, 1},        // bus_width/ch
  {0x00F0, 0x07, 2},        // Register Vendor
  {0x0200, 0x0B, 2},        // JEDEC MFG ID
  {0x0202, 0x0D, 1},        // MFG Location
  {0x0203, 0x0E, 1},        // Year
  {0x0204, 0x0F, 1},        // Week
  {0x0205, 0x10, 4},        // Serial Number
  {0x0209, 0x14, 30},       // Part Number (ASCII, padded)
};

static int fill_sparse_window(uint8_t fru_id, uint8_t /*cpu*/, uint8_t dimm,
                              uint16_t off, uint8_t len, uint8_t* rxbuf)
{
  if (!rxbuf || !len) return -1;
  memset(rxbuf, 0x00, len);

  uint8_t comp[0x50];
  if (read_spd_compact(slot_to_eid(fru_id), dimm, comp) != 0) {
    return (int)len;
  }

  for (const auto& m : k_compact_to_raw) {
    uint32_t a0 = off,     a1 = (uint32_t)off + len;
    uint32_t b0 = m.raw_off, b1 = (uint32_t)m.raw_off + m.len;
    uint32_t s  = (a0 > b0) ? a0 : b0;
    uint32_t e  = (a1 < b1) ? a1 : b1;
    if (e <= s) continue;

    uint32_t ol = e - s;
    uint32_t w_off_in_dst = s - a0;
    uint32_t off_in_map   = s - b0;

    memcpy(rxbuf + w_off_in_dst, comp + m.comp_off + off_in_map, ol);
  }

  return (int)len;
}

static inline uint8_t slot_to_eid(uint8_t slot_id) {
  return (uint8_t)(slot_id * 10);
}

static bool run_pldmtool_and_get_rx(const std::string& cmd, std::vector<uint8_t>& out)
{
  FILE* fp = popen(cmd.c_str(), "r");
  if (!fp) {
    DBG_PRINT("popen failed for cmd: %s\n", cmd.c_str());
    return false;
  }

  char line[1024] = {0};
  std::string last_rx_line;
  while (fgets(line, sizeof(line), fp)) {
    std::string s(line);
    if (s.find("Rx:") != std::string::npos) {
      last_rx_line = s; // Take the last line containing "Rx:"
    }
  }
  pclose(fp);

  if (last_rx_line.empty()) {
    DBG_PRINT("No 'Rx:' found for cmd: %s\n", cmd.c_str());
    return false;
  }

  // Decode the hex string that follows "Rx:"
  size_t pos = last_rx_line.find("Rx:");
  if (pos == std::string::npos) {
    return false;
  }
  std::string hex = last_rx_line.substr(pos + 3); // skip "Rx:"
  std::istringstream iss(hex);
  std::string tok;
  out.clear();
  while (iss >> tok) {
    tok.erase(std::remove_if(tok.begin(), tok.end(),
                             [](char c){ return c==',' || c==':'; }), tok.end());
    char* endp = nullptr;
    long v = strtol(tok.c_str(), &endp, 16);
    if (endp != tok.c_str() && *endp == '\0' && v >= 0 && v <= 0xFF) {
      out.push_back((uint8_t)v);
    }
  }
  if (out.empty()) {
    DBG_PRINT("Parsed Rx is empty. line: %s\n", last_rx_line.c_str());
    return false;
  }
  return true;
}

static bool probe_dimm_present_ready(uint8_t fru_id, uint8_t dimm, bool* ready_out) {
  if (ready_out) *ready_out = false;

  uint8_t comp[0x50];
  if (read_spd_compact(slot_to_eid(fru_id), dimm, comp) != 0) {
    return false;
  }

  bool present = (comp[SPD_C_OEM_PRESENT] == 1);
  bool ready   = (comp[SPD_C_OEM_STATUS] & (1u << SPD_READY_BIT)) != 0;

  if (ready_out) *ready_out = ready;
  return present;
}

int util_read_spd_byte(uint8_t fru_id, uint8_t cpu, uint8_t dimm, uint8_t offset) {
  uint8_t b = 0;
  int r = util_read_spd(fru_id, cpu, dimm, offset, 1, &b);
  return (r == 1) ? b : -1;
}
int util_read_spd(uint8_t fru_id, uint8_t cpu, uint8_t dimm,
                  uint16_t offset, uint8_t len, uint8_t* rxbuf)
{
  bool ready = false;
  if (!probe_dimm_present_ready(fru_id, dimm, &ready)) {
    return -1;
  }
  if (!ready){
    return -1;
  }
  return fill_sparse_window(fru_id, cpu, dimm, offset, len, rxbuf);
}

int util_set_EE_page(uint8_t /*slot_id*/, uint8_t /*cpu*/, uint8_t /*dimm*/, uint8_t /*page_num*/)
{
  return 0;
}

int util_check_me_status(uint8_t /*slot_id*/)
{
  return 0;
}