#include "../kb900x_comm.h"
#include "../kb900x_utils.h"
#include "../kb900x_log.h"

#ifdef CONFIG_BIC_COMMUNICATION
#include <facebook/bic_xfer.h> // Not supported on our platform
#else
// Mocked function for test purpose
int bic_data_wrapper(uint8_t slot_id, uint8_t netfn, uint8_t cmd,
                     uint8_t *txbuf, uint16_t txlen, uint8_t *rxbuf, uint8_t *rxlen)
{
  KANDOU_INFO("bic_data_wrapper called with slot_id: %d, netfn: %d, cmd: %d, txbuf: %02x %02x %02x %02x, txlen: %d",
              slot_id, netfn, cmd, txbuf[0], txbuf[1], txbuf[2], txbuf[3], txlen);
  return KB900X_E_OK;
}
#define NETFN_APP_REQ (0x06)
#define CMD_APP_MASTER_WRITE_READ (0x0C)
#endif

// Constants
// Command code as per supplemental spec Table 6-3.
// (Slave SMBus Command Code Fields)
#define CCODE_START_READ_FUNC0 (0x82)
#define CCODE_END_READ_FUNC0 (0x81)

#define CCODE_START_END_WRITE_FUNC1 (0x87)

#define CCODE_START_READ_FUNC2 (0x8A)
#define CCODE_END_READ_FUNC2 (0x89)

#define CCODE_START_END_WRITE_FUNC3 (0x8F)

static uint8_t calculate_pec(uint8_t *data, size_t len)
{
  uint8_t crc = 0x00;
  for (size_t i = 0; i < len; i++)
  {
    crc ^= data[i];
    for (int j = 0; j < 8; j++)
    {                                                      // NOLINT
      crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1); // NOLINT
    }
  }
  return crc;
}

static int get_smbus_command_code(uint8_t address_size, uint8_t *command_code_start,
                                  uint8_t *command_code_stop)
{
  if (address_size == 4)
  {
    *command_code_start = CCODE_START_READ_FUNC2;
    *command_code_stop = CCODE_END_READ_FUNC2;
  }
  else if (address_size == 2)
  {
    *command_code_start = CCODE_START_READ_FUNC0;
    *command_code_stop = CCODE_END_READ_FUNC0;
  }
  else
  {
    KANDOU_ERR("Address size not supported. Must be 2 or 4 bytes.");
    return -EINVAL;
  }
  return KB900X_E_OK;
}

// FIXME Trying to SMBus write to the retimer through the BIC 
int __attribute__((weak)) kb900x_write(int handle, const uint8_t *address, const uint8_t address_size,
                                      const uint8_t *payload, const uint8_t payload_size)
{
    (void)handle; // FIXME find a way to avoid unused parameters
    // We only use 4 bytes addresses with vendor defined SMBus write register
    if (address_size != 4)
    {
      KANDOU_ERR("Address size must be 4 bytes");
      return -EINVAL;
    }
    uint8_t tbuf[64] = {0x00};
    uint8_t rbuf[64] = {0x00};
    uint8_t tlen = 0;
    uint8_t rlen = 0;
  
    const uint8_t bus = 1; // FIXME what is the correct bus value? Is 1 representing I2C1 on the BIC? Or is it representing the slave address of the BIC?
    const uint8_t addr = 0x50; // FIXME what is the correct address value? Is it the slave address of the Retimer on the I2C bus between the BIC and the retimer?
    const uint8_t smbus_tx_length = payload_size + address_size + 2; // FIXME Write length = ByteCount + payload size + address size + PEC
    tbuf[0] = (bus << 1) + 1;
    tbuf[1] = addr;
    tbuf[2] = 0x00; // Read count = 0
    tbuf[3] = CCODE_START_END_WRITE_FUNC3; // FIXME Is it the SMBus Command Code
    tbuf[4] = smbus_tx_length;
  
    // Copy address to the beginning of the buffer
    for (size_t i = 0; i < address_size; i++)
    {
      // As SMBus expect address in little endian
      // We reverse the address to match the expected format
      tbuf[i + 5] = address[address_size - 1 - i];
    }
    // Copy the payload after the address
    for (size_t i = 0; i < payload_size; i++)
    {
      // As SMBus expect payload in little endian
      // We reverse the payload to match the expected format
      tbuf[i + 5 + address_size] = payload[payload_size - i - 1];
    }
    // Add the PEC
    uint8_t data_to_sign[smbus_tx_length + 1]; // FIXME +1 for the slave address
    data_to_sign[0] = addr << 1; // Write
    data_to_sign[1] = CCODE_START_END_WRITE_FUNC3;
    for (int i = 0; i < smbus_tx_length - 1; i++) // FIXME -1 for the PEC
    {
      data_to_sign[i + 2] = tbuf[i + 5];
    }
    tbuf[smbus_tx_length + 3] = calculate_pec(data_to_sign, smbus_tx_length + 1);
    tlen = smbus_tx_length + 4; // FIXME Is the size correct? What should be the format of the tbuf passed to the bic_data_wrapper function?
    const uint8_t slot_id = 0; // FIXME what is the correct slot id? Is it the BIC id?
    int ret = bic_data_wrapper(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
    if(ret != 0) {
      KANDOU_ERR("bic_data_wrapper failed with error code: %d", ret);
    }
    return ret;
}

// FIXME Trying to read from the retimer through the BIC
int __attribute__((weak)) kb900x_read(int handle, const uint8_t *address, const uint8_t address_size,
                uint8_t *result, uint8_t result_size) {
  (void) handle; // FIXME find a way to avoid unused parameters
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t tbuf[64] = {0x00};
  uint8_t rbuf[64] = {0x00};

  // Get SMBus Command Code
  uint8_t command_code_start;
  uint8_t command_code_stop;
  int ret = get_smbus_command_code(address_size, &command_code_start,
                                   &command_code_stop);
  CHECK_SUCCESS(ret);
  // First we need to write the address to the retimer
  const uint8_t bus = 1;                                           // FIXME what is the correct bus value? Is 1 representing I2C1 on the BIC? Or is it representing the slave address of the BIC?
  const uint8_t addr = 0x50;                                       // FIXME what is the correct address value? Is it the slave address of the Retimer on the I2C bus between the BIC and the retimer?
  const uint8_t smbus_tx_length = address_size + 2; // FIXME Write length = ByteCount + address size + PEC
  tbuf[0] = (bus << 1) + 1;  // FIXME what is the correct bus value? Is 1 representing I2C1 on the BIC? Or is it representing the slave address of the BIC?
  tbuf[1] = addr;            // FIXME what is the correct address value? Is it the slave address of the Retimer on the I2C bus between the BIC and the retimer?
  tbuf[2] = 0x00;                        
  tbuf[3] = command_code_start; // FIXME Is it the SMBus Command Code
  tbuf[4] = smbus_tx_length;
  
  // Copy address to the beginning of the buffer
  for (size_t i = 0; i < address_size; i++)
  {
    // As SMBus expect address in little endian
    // We reverse the address to match the expected format
    tbuf[i + 5] = address[address_size - 1 - i];
  }
  // Add the PEC
  uint8_t data_to_sign[smbus_tx_length + 1]; // FIXME +1 for the slave address
  data_to_sign[0] = addr << 1;               // Write
  data_to_sign[1] = command_code_start;
  for (int i = 0; i < smbus_tx_length - 1; i++) // FIXME -1 for the PEC
  {
    data_to_sign[i + 2] = tbuf[i + 5];
  }
  tbuf[smbus_tx_length + 3] = calculate_pec(data_to_sign, smbus_tx_length + 1);
  tlen = smbus_tx_length + 4; // FIXME Is the size correct? What should be the format of the tbuf passed to the bic_data_wrapper function?
  const uint8_t slot_id = 0;  // FIXME what is the correct slot id? Is it the BIC id?
  ret = bic_data_wrapper(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if (ret != 0)
  {
    KANDOU_ERR("bic_data_wrapper failed with error code: %d", ret);
  }
  // Then we read the result from the retimer
  memset(tbuf, 0, sizeof(tbuf));
  memset(rbuf, 0, sizeof(rbuf));
  tlen = 0;
  rlen = 0;
  
  tbuf[0] = (bus << 1) + 1; // FIXME what is the correct bus value? Is 1 representing I2C1 on the BIC? Or is it representing the slave address of the BIC?
  tbuf[1] = addr;           // FIXME what is the correct address value? Is it the slave address of the Retimer on the I2C bus between the BIC and the retimer?
  tbuf[2] = 0x04;           // Bytecnt = 4, A read operation will always return 4 bytes
  tbuf[3] = command_code_stop; // FIXME Is it the SMBus Command Code
  tlen = 4;
  ret = bic_data_wrapper(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if (ret != 0)
  {
    KANDOU_ERR("bic_data_wrapper failed with error code: %d", ret);
  }else{
    // Copy the result to the result buffer
    for (size_t i = 0; i < result_size; i++)
    {
      // As SMBus expect payload in little endian
      // We reverse the payload to match the expected format
      result[i] = rbuf[i + 2]; // FIXME +2 for the ByteCount and command code
    }
  }

  return ret;
}
