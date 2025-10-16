#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include <sys/socket.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <endian.h>

// Vendor information constants
#define VENDORS_BANKS 8
#define VENDORS_ITEMS 128

// MCTP and CCI constants
#define MCTP_MEG_TYPE_CCI 0x8
#define DEFAULT_NET 1
#define MCTP_TAG_OWNER 0x08

// CCI Command Codes
static constexpr uint16_t CCI_GET_FW_INFO = 0x0200;
static constexpr uint16_t CCI_GET_EVENT_RECORDS = 0x0100;
static constexpr uint16_t CCI_GET_SUPPORTED_LOGS = 0x0400;
static constexpr uint16_t CCI_GET_LOG = 0x0401;
static constexpr uint16_t CCI_DIMM_SPD_READ = 50448; // 0xC410
static constexpr uint16_t CCI_DIMM_SLOT_INFO = 0xC520;
static constexpr uint16_t CCI_GET_HEALTH_COUNTERS = 52737; // 0xCE01
static constexpr uint16_t CCI_GET_CXL_MEMBRIDGE_STATS = 0xFB18;

// Event log constants
#define CXL_MAX_RECORDS_TO_DUMP 20
#define CXL_DRAM_EVENT_GUID "601dcbb3-9c06-4eab-b8af-4e9bfb5c9624"
#define CXL_MEM_MODULE_EVENT_GUID "fe927475-dd59-4339-a586-79bab113b774"

// Log UUID constants
#define CEL_UUID "0da9c0b5-bf41-4b78-8f79-96b1623b3f17"
#define VENDOR_LOG_UUID "5e1819d9-11a9-400c-811f-d60719403d86"

// DIMM constants
#define SPD_MODULE_SERIAL_NUMBER_LEN 4

// Endian conversion macros
#define le16_to_cpu(x) le16toh(x)
#define le32_to_cpu(x) le32toh(x)
#define le64_to_cpu(x) le64toh(x)
#define cpu_to_le16(x) htole16(x)
#define cpu_to_le32(x) htole32(x)
#define cpu_to_le64(x) htole64(x)

extern bool g_debug_mode;
#define DEBUG_PRINT(...) \
    do { \
        if (g_debug_mode) { \
            printf("[DEBUG] %s:%d: ", __func__, __LINE__); \
            printf(__VA_ARGS__); \
            printf("\n"); \
            fflush(stdout); \
        } \
    } while(0)

// MCTP socket structures
struct sockaddr_mctp {
    unsigned short smctp_family;
    int smctp_network;
    struct {
        unsigned char s_addr;
    } smctp_addr;
    unsigned char smctp_type;
    unsigned char smctp_tag;
};

struct sockaddr_mctp_ext {
    struct sockaddr_mctp smctp_base;
};

// CCI message structures
struct mctp_cci_hdr {
    uint8_t cci_msg_req_resp;
    uint8_t msg_tag;
    uint8_t cci_rsv;
    uint16_t op;
    int pl_len:21;
    uint8_t rsv:2;
    uint8_t BO:1;
    uint16_t ret;
    uint16_t stat;
} __attribute__((packed));

// Firmware info response structure
struct cci_fw_info_resp {
    mctp_cci_hdr hdr;
    uint8_t fw_slot_supported;
    union {
        uint8_t value;
        struct {
            uint8_t ACTIVE_FW_SLOT:3;
            uint8_t STAGED_FW_SLOT:3;
            uint8_t RESV:2;
        } fields;
    } fw_slot_info;
    uint8_t fw_active_capability;
    uint8_t reserved[13];
    char slot1_fw_revision[16];
    char slot2_fw_revision[16];
    char slot3_fw_revision[16];
    char slot4_fw_revision[16];
} __attribute__((packed));

// Event record structures
struct cxl_dram_event_record {
    uint64_t physical_addr;
    uint8_t memory_event_descriptor;
    uint8_t memory_event_type;
    uint8_t transaction_type;
    uint16_t validity_flags;
    uint8_t channel;
    uint8_t rank;
    uint8_t nibble_mask[3];
    uint8_t bank_group;
    uint8_t bank;
    uint8_t row[3];
    uint16_t column;
    uint8_t correction_mask[0x20];
    uint8_t component_identifier[0x10];
    uint8_t sub_channel;
    uint8_t reserved[0x6];
} __attribute__((packed));

struct cxl_memory_module_record {
    uint8_t dev_event_type;
    uint8_t dev_health_info[0x12];
    uint8_t reserved[0x3d];
} __attribute__((packed));

struct cxl_event_record {
    uuid_t uuid;
    uint8_t event_record_length;
    uint8_t event_record_flags[3];
    uint16_t event_record_handle;
    uint16_t related_event_record_handle;
    uint64_t event_record_ts;
    uint8_t reserved[0x10];
    union {
        struct cxl_dram_event_record dram_event_record;
        struct cxl_memory_module_record memory_module_record;
    } event_record;
} __attribute__((packed));

struct cxl_get_event_record_info {
    uint8_t flags;
    uint8_t reserved1;
    uint16_t overflow_err_cnt;
    uint64_t first_overflow_evt_ts;
    uint64_t last_overflow_evt_ts;
    uint16_t event_record_count;
    uint8_t reserved2[0xa];
} __attribute__((packed));

// Get log structures
struct cxl_mbox_get_log {
    uuid_t uuid;
    uint32_t offset;
    uint32_t length;
} __attribute__((packed));

struct cel_entry {
    uint16_t opcode;
    uint16_t effect;
} __attribute__((packed));

// DIMM structures
struct cxl_mbox_dimm_spd_read_in {
    uint32_t spd_id;
    uint32_t offset;
    uint32_t num_bytes;
} __attribute__((packed));

struct cxl_dimm_slot_info_out {
    uint8_t num_dimm_slots;
    uint8_t rsvd[3];
    uint8_t slot0_spd_i2c_addr;
    uint8_t slot0_channel_id;
    uint8_t slot0_dimm_silk_screen;
    uint8_t slot0_dimm_present;
    uint8_t rsvd1[12];
    uint8_t slot1_spd_i2c_addr;
    uint8_t slot1_channel_id;
    uint8_t slot1_dimm_silk_screen;
    uint8_t slot1_dimm_present;
    uint8_t rsvd2[12];
    uint8_t slot2_spd_i2c_addr;
    uint8_t slot2_channel_id;
    uint8_t slot2_dimm_silk_screen;
    uint8_t slot2_dimm_present;
    uint8_t rsvd3[12];
    uint8_t slot3_spd_i2c_addr;
    uint8_t slot3_channel_id;
    uint8_t slot3_dimm_silk_screen;
    uint8_t slot3_dimm_present;
    uint8_t rsvd4[12];
} __attribute__((packed));

// RAM type enum
typedef enum {
    UNKNOWN = 0,
    DIRECT_RAMBUS = 1,
    RAMBUS = 2,
    FPM_DRAM = 3,
    EDO = 4,
    PIPELINED_NIBBLE = 5,
    SDR_SDRAM = 6,
    MULTIPLEXED_ROM = 7,
    DDR_SGRAM = 8,
    DDR_SDRAM = 9,
    DDR2_SDRAM = 10,
    DDR3_SDRAM = 11,
    DDR4_SDRAM = 12,
    N_RAM_TYPES = 13
} RamType;

// Health counters structure
struct cxl_mbox_health_counters_get_out {
    uint32_t critical_over_temperature_exceeded;
    uint32_t power_on_events;
    uint32_t power_on_hours;
    uint32_t cxl_mem_link_crc_errors;
    uint32_t cxl_io_link_lcrc_errors;
    uint32_t cxl_io_link_ecrc_errors;
    uint32_t num_ddr_correctable_ecc_errors;
    uint32_t num_ddr_uncorrectable_ecc_errors;
    uint32_t link_recovery_events;
    uint32_t time_in_throttled;
    uint32_t over_temperature_warning_level_exceeded;
    uint32_t critical_under_temperature_exceeded;
    uint32_t under_temperature_warning_level_exceeded;
    uint32_t rx_retry_request;
    uint32_t rcmd_qs0_hi_threshold_detect;
    uint32_t rcmd_qs1_hi_threshold_detect;
    uint32_t num_pscan_correctable_ecc_errors;
    uint32_t num_pscan_uncorrectable_ecc_errors;
    uint32_t num_ddr_dimm0_correctable_ecc_errors;
    uint32_t num_ddr_dimm0_uncorrectable_ecc_errors;
    uint32_t num_ddr_dimm1_correctable_ecc_errors;
    uint32_t num_ddr_dimm1_uncorrectable_ecc_errors;
    uint32_t num_ddr_dimm2_correctable_ecc_errors;
    uint32_t num_ddr_dimm2_uncorrectable_ecc_errors;
    uint32_t num_ddr_dimm3_correctable_ecc_errors;
    uint32_t num_ddr_dimm3_uncorrectable_ecc_errors;
} __attribute__((packed));

// Memory bridge stats structure
struct cxl_cmd_membridge_stats_out {
    uint64_t m2s_req_count;
    uint64_t m2s_rwd_count;
    uint64_t s2m_drs_count;
    uint64_t s2m_ndr_count;
    uint64_t rwd_first_poison_hpa_log;
    uint64_t rwd_latest_poison_hpa_log;
    uint64_t req_first_hpa_log;
    uint64_t rwd_first_hpa_log;
    uint32_t mst_m2s_req_corr_err_count;
    uint32_t mst_m2s_rwd_corr_err_count;
    uint32_t fifo_full_status;
    uint32_t fifo_empty_status;
    uint8_t m2s_rwd_credit_count;
    uint8_t m2s_req_credit_count;
    uint8_t s2m_ndr_credit_count;
    uint8_t s2m_drc_credit_count;
    uint8_t rx_fsm_status_rx_deinit;
    uint8_t rx_fsm_status_m2s_req;
    uint8_t rx_fsm_status_m2s_rwd;
    uint8_t rx_fsm_status_ddr0_ar_req;
    uint8_t rx_fsm_status_ddr0_aw_req;
    uint8_t rx_fsm_status_ddr0_w_req;
    uint8_t rx_fsm_status_ddr1_ar_req;
    uint8_t rx_fsm_status_ddr1_aw_req;
    uint8_t rx_fsm_status_ddr1_w_req;
    uint8_t tx_fsm_status_tx_deinit;
    uint8_t tx_fsm_status_s2m_ndr;
    uint8_t tx_fsm_status_s2m_drc;
    uint8_t stat_qos_tel_dev_load_read;
    uint8_t stat_qos_tel_dev_load_type2_read;
    uint8_t stat_qos_tel_dev_load_write;
    uint8_t resvd;
} __attribute__((packed));

// Function declarations
int send_cci_command(uint8_t eid, uint16_t opcode, const void* payload, size_t payload_len, 
                     std::vector<uint8_t>& response);

std::string find_argument(const std::vector<std::string>& params, const std::string& flag);

// Helper functions
void int_to_string(uint8_t *string, uint8_t *integer, uint8_t sizeInByte);
const char* decode_ddr4_module_type(uint8_t *bytes);
float ddr4_mtb_ftb_calc(unsigned char b1, signed char b2);
int decode_ddr4_module_speed(uint8_t *bytes);
int decode_ddr4_module_size(uint8_t *bytes);
const char* decode_ddr4_manufacturer(uint8_t *bytes);
int decode_ram_type(uint8_t *bytes);

// CCI functions
int get_fw_info(uint8_t eid);
int get_event_records(uint8_t eid, uint8_t event_log_type);
int get_supported_logs(uint8_t eid);
int get_log(uint8_t eid, const std::string& uuid, uint32_t data_size);
int dimm_spd_read(uint8_t eid, uint32_t spd_id, uint32_t offset, uint32_t num_bytes);
int dimm_slot_info(uint8_t eid);
int get_health_counters(uint8_t eid);
int get_cxl_membridge_stats(uint8_t eid);

// Wrapper functions
int get_fw_info_wrapper(uint8_t eid, const std::vector<std::string>& params);
int get_event_records_wrapper(uint8_t eid, const std::vector<std::string>& params);
int get_supported_logs_wrapper(uint8_t eid, const std::vector<std::string>& params);
int get_log_wrapper(uint8_t eid, const std::vector<std::string>& params);
int dimm_spd_read_wrapper(uint8_t eid, const std::vector<std::string>& params);
int dimm_slot_info_wrapper(uint8_t eid, const std::vector<std::string>& params);
int get_health_counters_wrapper(uint8_t eid, const std::vector<std::string>& params);
int get_cxl_membridge_stats_wrapper(uint8_t eid, const std::vector<std::string>& params);
