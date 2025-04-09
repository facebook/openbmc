/** Driver to communicate with Regli Retimer
 * @file kb900x.h
 * @brief The driver to communicate with Regli Retimer
 * @ingroup retimer-regli
 */
#ifndef __KB900X_H__
#define __KB900X_H__

#include <stdint.h>

typedef enum {
  KB900X_FEATURE_REQ_STATUS_NOT_SET = 0, /* status_not set */
  KB900X_FEATURE_REQ_STATUS_SUCCESS,     /* Request succeeded */
  KB900X_FEATURE_REQ_STATUS_IN_PROGRESS, /* Request in progress */
  KB900X_FEATURE_REQ_STATUS_FAILURE,     /* Request failed */
} kb900x_feature_req_status_t;

/**
 * \brief Type representing a single entry in a log.
 */
typedef union {
  struct {
    /** The RTSSM state */
    uint16_t rtssm : 6;
    /** The data rate in range 0 to 4 (invalid =7, PCIe Gen 5 = 4, PCIe Gen4 =
     * 3, ...)*/
    uint16_t data_rate : 3;
    /** The time delta */
    uint16_t delta : 7;
  };
  uint16_t raw;
} kb900x_rtssm_entry_t;

/**
 * \brief Type representing a log map info.
 */
typedef struct {
  /** The tile ID */
  uint8_t tile_id;
  /** The RPCS ID */
  uint8_t rpcs_id;
  /** The RTSSM config ID */
  uint8_t rtssm_cfg_id;
  /** The current position */
  uint8_t curr_pos;
} kb900x_rtssm_log_map_info_t;

/**
 * \brief Struct representing all RTSSM entries of a single RPCS
 */
typedef struct {
  kb900x_rtssm_log_map_info_t log_map_info;
  kb900x_rtssm_entry_t entries[32];
} kb900x_rtssm_log_t;

/**
 * \brief Struct representing all the RTSSM logs on a device
 */
typedef struct __attribute__((packed, aligned(4))) {
  kb900x_rtssm_log_t logs[8]; // indexed log contents (some may be empty)
} kb900x_rtssm_all_logs_t;

/**
 * \brief Get the HW RTSSM logs.
 *
 * \param[in] handle the I2C handler,
 * \param[out] log a pointer for writing the result
 *
 * \return 0 if no error, else the error code
 */
int kb900x_get_hw_rtssm_log(int handle, kb900x_rtssm_all_logs_t *log);

#endif // __KB900X_H__