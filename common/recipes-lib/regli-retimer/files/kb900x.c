#include "kb900x.h"
#include "kb900x_addresses.h"
#include "kb900x_comm.h"
#include "kb900x_utils.h"

int kb900x_get_hw_rtssm_log(int handle, kb900x_rtssm_all_logs_t *log) {
  int ret = 0;
  uint32_t operation_status = 0;
  uint32_t dccm_buff_addr;
  uint32_t dccm_buff_length;

  // Read request
  ret = kb900x_read_smbus_command(handle, KB900X_OFFSET_RTSSM_REQ,
                                  &operation_status);
  CHECK_SUCCESS_MSG(
      ret, "Unable to get the hw RTSSM logs (request), err code : %d - %s",
      errno, strerror(errno));
  KANDOU_DEBUG("Request status = %d", operation_status);
  // Read operation status
  uint8_t timeout = 0;
  while (operation_status != KB900X_FEATURE_REQ_STATUS_SUCCESS &&
         timeout < 100) {
    ret = kb900x_read_smbus_command(handle, KB900X_OFFSET_RTSSM_STATUS,
                                    &operation_status);
    CHECK_SUCCESS_MSG(
        ret,
        "Unable to get the hw RTSSM logs (request status), err code : %d - %s",
        errno, strerror(errno));
    KANDOU_DEBUG("Operation status = %d", operation_status);
    timeout++;
  }

  // Check the operation status
  if (operation_status != KB900X_FEATURE_REQ_STATUS_SUCCESS) {
    return KB900X_E_FEATURE_REQ_FAILED;
  }

  // Read DCCM start address
  ret = kb900x_read_smbus_command(handle, KB900X_OFFSET_RTSSM_START_ADDR,
                                  &dccm_buff_addr);
  CHECK_SUCCESS_MSG(ret,
                    "Unable to get the hw RTSSM logs (get buffer address), err "
                    "code : %d - %s",
                    errno, strerror(errno));
  KANDOU_DEBUG("DCCM buff addr=0x%x", dccm_buff_addr);

  // Read DCCM length
  ret = kb900x_read_smbus_command(handle, KB900X_OFFSET_RTSSM_LENGTH,
                                  &dccm_buff_length);
  CHECK_SUCCESS_MSG(
      ret,
      "Unable to get the hw RTSSM logs (get buffer length), err code : %d - %s",
      errno, strerror(errno));
  KANDOU_DEBUG("DCCM buff length=0x%x\n", dccm_buff_length);

  // Read dump
  uint32_t value;
  for (uint32_t idx = 0; idx < dccm_buff_length / 4; idx++) {
    kb900x_read_register(handle, dccm_buff_addr + (4 * idx), &value);
    ((uint32_t *)log)[idx] = value;
  }

  return KB900X_E_OK;
}