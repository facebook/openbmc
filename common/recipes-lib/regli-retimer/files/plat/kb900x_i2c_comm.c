#include "../kb900x_comm.h"
#include "../kb900x_utils.h"
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>

// Constants
// Command code as per supplemental spec Table 6-3.
// (Slave SMBus Command Code Fields)
#define CCODE_START_READ_FUNC0 (0x82)
#define CCODE_END_READ_FUNC0 (0x81)

#define CCODE_START_END_WRITE_FUNC1 (0x87)

#define CCODE_START_READ_FUNC2 (0x8A)
#define CCODE_END_READ_FUNC2 (0x89)

#define CCODE_START_END_WRITE_FUNC3 (0x8F)

uint8_t kb900x_i2c_slave_addr = 0x00;

static uint8_t calculate_pec(uint8_t *data, size_t len) {
  uint8_t crc = 0x00;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {                          // NOLINT
      crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1); // NOLINT
    }
  }
  return crc;
}

static int get_smbus_command_code(uint8_t address_size, uint8_t *command_code_start,
                                  uint8_t *command_code_stop) {
  if (address_size == 4) {
    *command_code_start = CCODE_START_READ_FUNC2;
    *command_code_stop = CCODE_END_READ_FUNC2;
  } else if (address_size == 2) {
    *command_code_start = CCODE_START_READ_FUNC0;
    *command_code_stop = CCODE_END_READ_FUNC0;
  } else {
    KANDOU_ERR("Address size not supported. Must be 2 or 4 bytes.");
    return -EINVAL;
  }
  return KB900X_E_OK;
}

static int smbus_pec(int handle, bool enabled) {
  // FIXME check driver functionalities?
  // int ret = smbus_check_supported_func(handle, I2C_FUNC_SMBUS_PEC);
  // if (ret == 0)
  // {
  //     // Not supported
  //     KANDOU_ERR("PEC not supported");
  //     return -E_PEC_NOT_SUPPORTED;
  // }
  // else if (ret < 0)
  // {
  // IOCTL errno - error
  // return ret;
  // }
  int ret = ioctl(handle, I2C_PEC, enabled);
  CHECK_IOCTL_MSG(ret, "Unable to enable SMBus PEC, err code : %d - %s", errno,
                  strerror(errno));
  return KB900X_E_OK;
}

int kb900x_i2c_open(int i2c_bus, uint8_t slave_address) {
  const uint8_t filename_max_length = 20;
  char filename[filename_max_length];
  int handle = 0;
  // Try to open /dev/i2c/{i2c_bus}
  snprintf(filename, filename_max_length, "/dev/i2c/%d", i2c_bus);
  handle = open(filename, O_RDWR);
  if (handle < 0 && (errno == ENOENT || errno == ENOTDIR)) {
    // Try to open /dev/i2c-{i2c_bus}
    snprintf(filename, filename_max_length, "/dev/i2c-%d", i2c_bus); // NOLINT
    handle = open(filename, O_RDWR);
  }
  // If the connection failed
  if (handle < 0) {
    if (errno == ENOENT) {
      KANDOU_ERR("Error: Could not open file "
                 "`/dev/i2c-%d' or `/dev/i2c/%d': %s\n",
                 i2c_bus, i2c_bus, strerror(ENOENT));
    } else {
      KANDOU_ERR("Error: Could not open file "
                 "`%s': %s\n",
                 filename, strerror(errno));
      if (errno == EACCES) {
        KANDOU_ERR("Run as root?");
      }
    }
    // Forward error above
    return -errno;
  }
  // Set slave address
  int ret = kb900x_i2c_select_slave_addr(handle, slave_address);
  CHECK_SUCCESS(ret);
  // Enable PEC
  smbus_pec(handle, true);
  return handle;
}

int kb900x_i2c_select_slave_addr(int handle, uint8_t slave_address) {
  kb900x_i2c_slave_addr = slave_address;
  int ret = ioctl(handle, I2C_SLAVE, slave_address);
  CHECK_IOCTL_MSG(ret,
                  "Unable to select I2C slave address %d, err code: %d - %s",
                  slave_address, errno, strerror(errno));
  return KB900X_E_OK;
}

int kb900x_write(int handle, const uint8_t *address, const uint8_t address_size,
                 const uint8_t *payload, const uint8_t payload_size) {
  // We only use 4 bytes addresses with vendor defined SMBus write register
  if (address_size != 4) {
    KANDOU_ERR("Address size must be 4 bytes");
    return -EINVAL;
  }
  struct i2c_smbus_ioctl_data blk;
  union i2c_smbus_data i2c_data;
  // Populate payload
  const uint8_t pec_size = 1;
  const uint8_t bytecnt_size = 1;

  i2c_data.block[0] = bytecnt_size + address_size + payload_size + pec_size;
  uint8_t bytecnt = address_size + payload_size;
  i2c_data.block[1] = bytecnt;
  // Copy the 4-byte address to the beginning of the buffer
  for (size_t i = 0; i < address_size; i++) {
    // As SMBus expect address in little endian
    // We reverse the address to match the expected format
    i2c_data.block[i + 2] = address[address_size - 1 - i];
  }

  // Copy the payload after the address
  for (size_t i = 0; i < payload_size; i++) {
    // As SMBus expect payload in little endian
    // We reverse the payload to match the expected format
    i2c_data.block[i + 2 + address_size] = payload[payload_size - i - 1];
  }
  // Add the PEC
  uint8_t data_to_sign[address_size + payload_size + 3];
  data_to_sign[0] = kb900x_i2c_slave_addr << 1; // Write
  data_to_sign[1] = CCODE_START_END_WRITE_FUNC3;
  for (int i = 0; i < bytecnt_size + address_size + payload_size; i++) {
    data_to_sign[i + 2] = i2c_data.block[i + 1];
  }
  i2c_data.block[bytecnt_size + address_size + payload_size + 1] =
      calculate_pec(data_to_sign, address_size + payload_size + 3);

  blk.read_write = I2C_SMBUS_WRITE;
  blk.command = CCODE_START_END_WRITE_FUNC3;
  blk.size = I2C_SMBUS_I2C_BLOCK_DATA;
  blk.data = &i2c_data;
  // FIXME Is it possible to use ioctl directly or should we use the obmc-i2c driver?
  int ret = ioctl(handle, I2C_SMBUS, &blk);
  CHECK_IOCTL_MSG(ret, "Unable to write I2C data, err code : %d - %s", errno,
                  strerror(errno));
  return KB900X_E_OK;
}

int kb900x_read(int handle, const uint8_t *address, const uint8_t address_size,
                uint8_t *result, uint8_t result_size) {
  struct i2c_smbus_ioctl_data blk;
  union i2c_smbus_data i2c_data;

  const uint8_t data_offset = 2;
  uint8_t command_code_start;
  uint8_t command_code_stop;
  uint8_t bytecnt;
  int ret;
  int retries = 0;
  const int max_retries = 3;
  while (retries < max_retries) {
    retries++;
    // WRITE (prepare read)
    // Populate payload
    const uint8_t pec_size = 1;
    const uint8_t bytecnt_size = 1;

    i2c_data.block[0] =
        bytecnt_size + address_size + pec_size; // Total data to send
    bytecnt = address_size;
    i2c_data.block[1] = bytecnt;
    for (size_t i = 0; i < address_size; i++) {
      // As SMBus expect address in little endian
      // We reverse the address to match the expected format
      i2c_data.block[i + 2] = address[address_size - 1 - i];
    }
    ret = get_smbus_command_code(address_size, &command_code_start,
                                 &command_code_stop);
    CHECK_SUCCESS(ret);

    // Add the PEC
    uint8_t data_to_sign[address_size + 3];
    data_to_sign[0] = kb900x_i2c_slave_addr << 1; // Write
    data_to_sign[1] = command_code_start;
    for (int i = 0; i < bytecnt_size + address_size; i++) {
      data_to_sign[i + 2] = i2c_data.block[i + 1];
    }
    i2c_data.block[bytecnt_size + address_size + 1] =
        calculate_pec(data_to_sign, address_size + 3);

    blk.read_write = I2C_SMBUS_WRITE;
    blk.command = command_code_start;
    blk.size = I2C_SMBUS_I2C_BLOCK_DATA;
    blk.data = &i2c_data;
    // Send write
    ret = ioctl(handle, I2C_SMBUS, &blk);
    if (ret < 0) {
      continue;
    }
    // READ
    blk.read_write = I2C_SMBUS_READ;
    blk.command = command_code_stop;
    blk.size = I2C_SMBUS_I2C_BLOCK_DATA;
    const int nb_data_to_read =
        result_size + address_size + bytecnt_size + pec_size;
    i2c_data.block[0] = nb_data_to_read;
    blk.data = &i2c_data;
    // Send read
    ret = ioctl(handle, I2C_SMBUS, &blk);
    if (ret < 0) {
      continue;
    }
    // Check PEC
    const uint8_t max_expected_size = 8;
    const uint8_t min_expected_size = 6;
    bytecnt = i2c_data.block[1];
    if (bytecnt != min_expected_size && bytecnt != max_expected_size) {
      continue;
    }
    uint8_t pec = i2c_data.block[data_offset + address_size + result_size];
    uint8_t data_to_check[address_size + result_size + 4];
    data_to_check[0] = (kb900x_i2c_slave_addr << 1) | 0; // Write
    data_to_check[1] = command_code_stop;
    data_to_check[2] = (kb900x_i2c_slave_addr << 1) | 1; // Read
    data_to_check[3] = bytecnt;
    for (int i = 0; i < bytecnt; i++) {
      data_to_check[i + 4] = i2c_data.block[data_offset + i];
    }
    if (pec != calculate_pec(data_to_check, address_size + result_size + 4)) {
      for (int i = 0; i < address_size + result_size + 4; i++) {
        KANDOU_DEBUG("data_to_check[%d] = %02x", i, data_to_check[i]);
      }
      KANDOU_ERR("PEC mismatch - received %02x expected %02x", pec,
                 calculate_pec(data_to_check, address_size + result_size + 4));
      continue;
    }
    break;
  }
  CHECK_IOCTL_MSG(ret, "Unable to read SMBus data");
  if (retries >= max_retries) {
    KANDOU_ERR("Unable to read SMBus data");
    return -ECOMM;
  }
  // Parse response
  // Check result buffer size FIXME
  if (result_size < bytecnt - address_size) {
    KANDOU_ERR("Buffer size too small: received %d expected %d",
               bytecnt - address_size, result_size);
    return -EINVAL;
  }

  // Check if the read register address is invalid (0xFFFFFFFF) and data is
  // invalid (0xDEADBEEF)
  if (address_size == 4) {
    const uint8_t INVALID_ADDR_BYTES[] = {0xFF, 0xFF, 0xFF, 0xFF};
    const uint8_t INVALID_DATA_BYTES[] = {0xEF, 0xBE, 0xAD,
                                          0xDE}; // Little endian
    bool is_invalid_addr =
        !memcmp(&i2c_data.block[data_offset], INVALID_ADDR_BYTES, address_size);
    bool is_invalid_data = !memcmp(&i2c_data.block[data_offset + address_size],
                                   INVALID_DATA_BYTES, address_size);
    if (is_invalid_addr && is_invalid_data) {
      char buf[50]; // Sufficient buffer size
      snprintf(buf, sizeof(buf), "Invalid register address: 0x"); // NOLINT

      // Append the address bytes as hex
      for (unsigned i = 0; i < address_size; i++) {
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%02x",
                 address[i]); // NOLINT
      }

      KANDOU_ERR("%s", buf);
      return -EINVAL;
    }
  }

  for (size_t i = 0; i < result_size; i++) {
    // result is little endian -> reverse
    result[i] =
        i2c_data.block[data_offset + address_size + (result_size - 1 - i)];
  }

  return ret;
}
