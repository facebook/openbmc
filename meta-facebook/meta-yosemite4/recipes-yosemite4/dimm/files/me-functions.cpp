// me-functions.cpp (Yosemite4, no libbic; read SPD via pldmtool raw, buffer-style)

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>

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

#ifndef SPD_RAW_LEN
 #ifdef SPD5_DUMP_LEN
  #define SPD_RAW_LEN SPD5_DUMP_LEN
 #else
  #define SPD_RAW_LEN 0x280   // DDR5: 0x000–0x27F
 #endif
#endif

#ifndef PLDM_WF_OEM_TYPE
 #define PLDM_WF_OEM_TYPE       0x3F
#endif
#ifndef PLDM_WF_READ_SPD_CHUNK
 #define PLDM_WF_READ_SPD_CHUNK 0x05
#endif
#ifndef PLDM_WF_IANA0
 #define PLDM_WF_IANA0          0x15
 #define PLDM_WF_IANA1          0xA0
 #define PLDM_WF_IANA2          0x00
#endif
#ifndef PLDM_WF_MAX_CHUNK
 #define PLDM_WF_MAX_CHUNK      0x40   // 64B
#endif

#ifndef PLDM_WF_READ_SPD_COMPACT
#define PLDM_WF_READ_SPD_COMPACT 0x01   // SD-BIC: compact SPD
#endif

static uint8_t g_cxl_page_for_dimm[8] = {0};
static uint8_t g_slot_tier_cache[9] = {0}; // 0=Unknown, 1=T1, 2=T2


static inline uint8_t slot_to_eid(uint8_t slot_id);
static bool run_pldmtool_and_get_rx(const std::string& cmd, std::vector<uint8_t>& out);
static bool is_slot_t2(uint8_t fru);
static std::string detect_slot_tier_via_dbus(uint8_t slot);
static bool active_probe_slot_is_t2(uint8_t fru);


static inline bool is_cxl_dimm_index(uint8_t dimm) 
{ 
  return dimm >= 12 && dimm <= 19; 
}

static inline uint8_t cxl_local_index(uint8_t dimm) 
{ 
  return (uint8_t)(dimm - 12); 
}

static inline uint16_t page_offset_bytes(uint8_t page) 
{ 
  return (uint16_t)page * 0x100; 
}

static inline uint8_t eid_cxl_from_fru(uint8_t fru) 
{ 
  return (uint8_t)(fru * 10 + 2); 
}

static inline uint8_t eid_wf_effective(uint8_t fru) {
  if (const char* s = std::getenv("DIMM_UTIL_WF_EID"))
    return (uint8_t)std::strtoul(s, nullptr, 0);
  return eid_cxl_from_fru(fru);
}

// ---- Yosemite 4 labels ----
extern bool read_block;
const char* get_dimm_label(uint8_t /*cpu*/, uint8_t dimm) {
  static const char* host[12] = {
    "A0","A1","A2","A3","A4","A5","A6","A7","A8","A9","A10","A11"
  };
  static const char* mcio4[4] = { "MCIO4-DIMM_A1", "MCIO4-DIMM_A0", "MCIO4-DIMM_B1", "MCIO4-DIMM_B0" };
  static const char* mcio3[4] = { "MCIO3-DIMM_A1", "MCIO3-DIMM_A0", "MCIO3-DIMM_B1", "MCIO3-DIMM_B0" };

  if (dimm < 12) return host[dimm];
  if (dimm < 16)   return mcio4[dimm - 12];
  if (dimm < 20)   return mcio3[dimm - 16];
  return "--";
}

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

  // Verify the presence of any T2 slots in the system
  bool has_t2_slots = false;
  for (uint8_t slot = 1; slot <= 8; ++slot) {
    if (is_slot_t2(slot)) {
      has_t2_slots = true;
      DBG_PRINT("Found T2 slot: %u\n", slot);
      break;
    }
  }
  
  if (has_t2_slots) {
    num_dimms_per_cpu = 20;  // Host(A0-A11) + CXL(12-19) DIMMs
    DBG_PRINT("System has T2 slots - enabling CXL DIMM support (0-19)\n");
  } else {
    num_dimms_per_cpu = 12;  // Host(A0-A11)
    DBG_PRINT("System has only T1 slots - limiting to host DIMMs (0-11)\n");
  }

  num_cpus          = 1;
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

// Read 0x50 bytes of compact SPD
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

// Read CXL SPD
static int pldm_data_wrapper(uint8_t eid, uint8_t pldm_type, uint8_t pldm_cmd,
                             const uint8_t* tbuf, uint8_t tlen,
                             uint8_t* rbuf, size_t* prlen) {
  if (!tbuf || !rbuf || !prlen) return -1;
  
  std::ostringstream oss;
  oss << "pldmtool raw -m " << unsigned(eid) << " -d"
      << " 0x80"
      << " 0x" << std::hex << std::uppercase << int(pldm_type)
      << " 0x" << int(pldm_cmd);
  
  for (uint8_t i = 0; i < tlen; i++) {
    oss << " 0x" << std::hex << std::uppercase << int(tbuf[i]);
  }

  std::vector<uint8_t> rx;
  if (!run_pldmtool_and_get_rx(oss.str(), rx)) return -1;
  
  size_t copy = (rx.size() < *prlen) ? rx.size() : *prlen;
  memcpy(rbuf, rx.data(), copy);
  *prlen = copy;
  
  return 0;
}

static int extract_spd_data_from_rx(const std::vector<uint8_t>& rx,
                                    std::vector<uint8_t>& data_out) {
  data_out.clear();
  
  for (size_t i = 0; i + 7 < rx.size(); ++i) {
    if (rx[i] == PLDM_WF_OEM_TYPE && rx[i+1] == PLDM_WF_READ_SPD_CHUNK) {
      uint8_t cc = rx[i+2];
      if (cc != 0x00) return -10;
      
      if (rx[i+3] != PLDM_WF_IANA0 || rx[i+4] != PLDM_WF_IANA1 || rx[i+5] != PLDM_WF_IANA2) 
        return -11;
      
      uint8_t dlen = rx[i+6];
      if (i + 7 + dlen > rx.size()) return -12;
      
      data_out.assign(rx.begin() + i + 7, rx.begin() + i + 7 + dlen);
      return 0;
    }
  }
  return -13;
}

static int yv4_read_cxl_spd_chunk(uint8_t fru_id, uint8_t cxl_idx, uint8_t dimm_idx,
                                  uint16_t offset, uint8_t len,
                                  uint8_t* out, uint8_t* got) {
  if (!out || !got) return -1;
  if (len == 0 || len > PLDM_WF_MAX_CHUNK) return -2;
  if ((uint32_t)offset + len > SPD_RAW_LEN) return -3;

  *got = 0;

  // Tx payload: IANA[3], cxl_idx, dimm_idx, off_lo, off_hi, len
  uint8_t tbuf[8];
  tbuf[0] = PLDM_WF_IANA0; 
  tbuf[1] = PLDM_WF_IANA1; 
  tbuf[2] = PLDM_WF_IANA2;
  tbuf[3] = cxl_idx; 
  tbuf[4] = dimm_idx;
  tbuf[5] = (uint8_t)(offset & 0xFF);
  tbuf[6] = (uint8_t)((offset >> 8) & 0xFF);
  tbuf[7] = len;

  uint8_t rbuf[128]; 
  size_t rlen = sizeof(rbuf);
  uint8_t eid = eid_wf_effective(fru_id);
  
  int rc = pldm_data_wrapper(eid, PLDM_WF_OEM_TYPE, PLDM_WF_READ_SPD_CHUNK,
                             tbuf, sizeof(tbuf), rbuf, &rlen);
  if (rc != 0) {
    DBG_PRINT("pldm_data_wrapper failed: fru=%u cxl=%u dimm=%u rc=%d\n", 
              fru_id, cxl_idx, dimm_idx, rc);
    return rc;
  }

  std::vector<uint8_t> rx(rbuf, rbuf + rlen), data;
  rc = extract_spd_data_from_rx(rx, data);
  if (rc != 0) {
    DBG_PRINT("extract_spd_data_from_rx failed: rc=%d\n", rc);
    return rc;
  }

  if (data.size() > len) return -4;
  
  memcpy(out, data.data(), data.size());
  *got = (uint8_t)data.size();
  return 0;
}

int util_read_spd_byte(uint8_t fru_id, uint8_t cpu, uint8_t dimm, uint8_t offset) {
  uint8_t b = 0;
  int r = util_read_spd(fru_id, cpu, dimm, offset, 1, &b);
  return (r == 1) ? b : -1;
}

int util_read_spd(uint8_t fru_id, uint8_t cpu, uint8_t dimm,
                  uint16_t offset, uint8_t len, uint8_t* rxbuf)
{
  // CXL DIMM: index 12..19
  if (is_cxl_dimm_index(dimm)) {
    if (!is_slot_t2(fru_id)) {
      DBG_PRINT("Slot %u is T1, CXL DIMM %u not available\n", fru_id, dimm);
      return -1;
    }
    
    uint8_t local = cxl_local_index(dimm);
    uint8_t cxl_idx = (local < 4) ? 0 : 1;  // 0=MCIO4, 1=MCIO3
    uint8_t cxl_dimm = (local & 0x3);       // 0..3

    uint8_t cur_page = g_cxl_page_for_dimm[local];
    uint16_t cur_off = (uint16_t)(offset + page_offset_bytes(cur_page));
    uint8_t* p = rxbuf;
    uint8_t left = len;

    while (left > 0) {
      uint8_t chunk = (left > PLDM_WF_MAX_CHUNK) ? PLDM_WF_MAX_CHUNK : left;
      if ((uint32_t)cur_off + chunk > SPD_RAW_LEN) return -1;

      uint8_t got = 0;
      int rc = yv4_read_cxl_spd_chunk(fru_id, cxl_idx, cxl_dimm, cur_off, chunk, p, &got);
      if (rc != 0 || got == 0) return -1;

      cur_off += got;
      p += got;
      left -= got;
    }
    return len;
  }

  // Host DIMM: index 0..11
  bool ready = false;
  if (!probe_dimm_present_ready(fru_id, dimm, &ready)) {
    return -1;
  }
  if (!ready){
    return -1;
  }
  return fill_sparse_window(fru_id, cpu, dimm, offset, len, rxbuf);
}

int util_set_EE_page(uint8_t slot_id, uint8_t /*cpu*/, uint8_t dimm, uint8_t page_num)
{
  (void)slot_id;
  if (!is_cxl_dimm_index(dimm)) return 0;
  if (page_num > 1) page_num = 1;
  
  uint8_t local = cxl_local_index(dimm);
  if (local >= 8) return -1;
  
  g_cxl_page_for_dimm[local] = page_num;
  return 0;
}

int util_check_me_status(uint8_t /*slot_id*/)
{
  return 0;
}

// Check T1/T2
static bool run_cmd_capture(const std::string& cmd, std::string& out) {
  FILE* fp = popen((cmd + " 2>/dev/null").c_str(), "r");
  if (!fp) return false;
  char buf[512];
  out.clear();
  while (fgets(buf, sizeof(buf), fp)) out += buf;
  pclose(fp);
  return !out.empty();
}

static std::string detect_slot_tier_via_dbus(uint8_t slot) {
  std::ostringstream oss;
  oss << "busctl call xyz.openbmc_project.ObjectMapper "
      << "/xyz/openbmc_project/object_mapper "
      << "xyz.openbmc_project.ObjectMapper GetSubTreePaths sias "
      << "\"/xyz/openbmc_project/inventory/system/board\" 1 1 "
      << "\"xyz.openbmc_project.Inventory.Item.Board.Motherboard\" | "
      << "grep -Eo \"Yosemite_4_Sentinel_Dome_T[12]_(with_Retimer_)?Slot_" 
      << (int)slot << "\"";
  
  std::string result;
  if (!run_cmd_capture(oss.str(), result)) {
    DBG_PRINT("Failed to detect slot type for slot %u\n", slot);
    return "UNKNOWN";
  }
  
  // Remove newline characters and whitespace
  result.erase(std::remove_if(result.begin(), result.end(), 
                             [](unsigned char c) { return std::isspace(c); }), 
               result.end());
  
  DBG_PRINT("Detected slot type for slot %u: '%s'\n", slot, result.c_str());
  
  if (result.find("_T1_") != std::string::npos) {
    return "T1";
  } else if (result.find("_T2_") != std::string::npos) {
    return "T2";
  }
  
  return "UNKNOWN";
}

static bool active_probe_slot_is_t2(uint8_t fru) {
  const uint8_t eid = eid_wf_effective(fru);
  
  for (uint8_t cxl_idx = 0; cxl_idx < 2; ++cxl_idx) {
    uint8_t tbuf[8] = { 
      PLDM_WF_IANA0, PLDM_WF_IANA1, PLDM_WF_IANA2,
      cxl_idx, 0x00, 0x00, 0x00, 0x01 
    };
    
    uint8_t rbuf[128]; 
    size_t rlen = sizeof(rbuf);
    
    if (pldm_data_wrapper(eid, PLDM_WF_OEM_TYPE, PLDM_WF_READ_SPD_CHUNK,
                          tbuf, sizeof(tbuf), rbuf, &rlen) == 0) {
      std::vector<uint8_t> rx(rbuf, rbuf + rlen), data;
      if (extract_spd_data_from_rx(rx, data) == 0 && !data.empty()) {
        DBG_PRINT("Active probe: slot %u is T2 (CXL response received)\n", fru);
        return true;
      }
    }
  }
  
  DBG_PRINT("Active probe: slot %u is T1 (no CXL response)\n", fru);
  return false;
}

static bool is_slot_t2(uint8_t fru) {
  if (fru == 0 || fru > 8) return false;
  
  if (const char* s = std::getenv("DIMM_UTIL_FORCE_TIER")) {
    bool forced_t2 = (strncmp(s, "T2", 2) == 0);
    DBG_PRINT("Forced tier for slot %u: %s\n", fru, forced_t2 ? "T2" : "T1");
    return forced_t2;
  }
  
  if (g_slot_tier_cache[fru] != 0) {
    DBG_PRINT("Using cached tier for slot %u: %s\n", fru,
              (g_slot_tier_cache[fru] == 2) ? "T2" : "T1");
    return (g_slot_tier_cache[fru] == 2);
  }
  
  std::string tier = detect_slot_tier_via_dbus(fru);
  if (tier == "T1") {
    g_slot_tier_cache[fru] = 1;
    DBG_PRINT("D-Bus detected slot %u as T1\n", fru);
    return false;
  } else if (tier == "T2") {
    g_slot_tier_cache[fru] = 2;
    DBG_PRINT("D-Bus detected slot %u as T2\n", fru);
    return true;
  }
  
  DBG_PRINT("D-Bus detection failed for slot %u, using active probe\n", fru);
  bool is_t2 = active_probe_slot_is_t2(fru);
  g_slot_tier_cache[fru] = is_t2 ? 2 : 1;
  
  return is_t2;
}