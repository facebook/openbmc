#include "kb900x_comm.h"
#include "kb900x_utils.h"

const uint8_t BITS_IN_BYTE = 8;

int kb900x_i2c_init(int i2c_bus, uint8_t slave_address) {
#ifdef CONFIG_BIC_COMMUNICATION
  return 0;
#else
  return kb900x_i2c_open(i2c_bus, slave_address);
#endif
}

int kb900x_write_register(int handle, const uint32_t address,
                          const uint32_t payload) {
  int ret = 0;
  // Convert uint32 address into an array of 4 bytes big-endian
  uint8_t address_array[KB900X_REGLI_REGISTER_ADDR_SIZE] = {0x00};
  const uint8_t mask = 0xFF;
  for (uint8_t i = 0; i < KB900X_REGLI_REGISTER_ADDR_SIZE; i++) {
    address_array[i] = (address >> ((KB900X_REGLI_REGISTER_ADDR_SIZE - 1 - i) *
                                    BITS_IN_BYTE)) &
                       mask;
  }

  // Splitting payload into bytes
  uint8_t payload_array[KB900X_REGLI_REGISTER_SIZE];
  payload_array[0] = (payload >> 24) & mask; // NOLINT
  payload_array[1] = (payload >> 16) & mask; // NOLINT
  payload_array[2] = (payload >> 8) & mask;  // NOLINT
  payload_array[3] = payload & mask;         // NOLINT

  ret = kb900x_write(handle, address_array, KB900X_REGLI_REGISTER_ADDR_SIZE,
                     payload_array, KB900X_REGLI_REGISTER_SIZE);
  CHECK_SUCCESS_MSG(ret, "Unable to write register, err code : %d - %s", errno,
                    strerror(errno));
  return KB900X_E_OK;
}

int kb900x_read_register(int handle, const uint32_t address, uint32_t *result) {
  int ret = 0;
  const size_t buffer_size = KB900X_REGLI_REGISTER_SIZE;
  uint8_t rx_buf[buffer_size];
  for (size_t i = 0; i < buffer_size; i++) {
    rx_buf[i] = 0;
  }
  uint8_t address_array[KB900X_REGLI_REGISTER_ADDR_SIZE] = {0x00};
  const uint8_t mask = 0xFF;
  // Convert uint32 address into an array of 4 bytes big-endian
  for (size_t i = 0; i < KB900X_REGLI_REGISTER_ADDR_SIZE; i++) {
    address_array[i] = (address >> ((KB900X_REGLI_REGISTER_ADDR_SIZE - 1 - i) *
                                    BITS_IN_BYTE)) &
                       mask;
  }
  ret = kb900x_read(handle, address_array, KB900X_REGLI_REGISTER_ADDR_SIZE,
                    rx_buf, KB900X_REGLI_REGISTER_SIZE);
  CHECK_SUCCESS_MSG(ret, "Unable to read register, err code : %d - %s", errno,
                    strerror(errno));
  *result = (rx_buf[0] << 24) | (rx_buf[1] << 16) | (rx_buf[2] << 8) |
            rx_buf[3]; // NOLINT
  return KB900X_E_OK;
}

int kb900x_read_smbus_command(int handle, uint16_t offsets, uint32_t *result) {
  int ret = 0;
  const size_t buffer_size = KB900X_SMBUS_COMMAND_SIZE;
  uint8_t rx_buf[buffer_size];
  for (size_t i = 0; i < buffer_size; i++) {
    rx_buf[i] = 0;
  }
  uint8_t address_array[KB900X_SMBUS_COMMAND_ADDR_SIZE] = {0x00};
  const uint8_t mask = 0xFF;
  // Convert uint32 address into an array of 4 bytes big-endian
  for (size_t i = 0; i < KB900X_SMBUS_COMMAND_ADDR_SIZE; i++) {
    address_array[i] =
        (offsets >> ((KB900X_SMBUS_COMMAND_ADDR_SIZE - 1 - i) * BITS_IN_BYTE)) &
        mask;
  }
  ret = kb900x_read(handle, address_array, KB900X_SMBUS_COMMAND_ADDR_SIZE,
                    rx_buf, KB900X_SMBUS_COMMAND_SIZE);
  CHECK_SUCCESS_MSG(ret, "Unable to read SMBus command, err code : %d - %s",
                    errno, strerror(errno));
  *result = (rx_buf[0] << 24) | (rx_buf[1] << 16) | (rx_buf[2] << 8) |
            rx_buf[3]; // NOLINT
  return KB900X_E_OK;
}