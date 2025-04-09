#include "kb900x_utils.h"
#include "kb900x.h"
#include <stdio.h>
#include <stdlib.h>

int read_file(const char *filename, uint8_t **buffer, size_t *buffer_size) {
  FILE *file = fopen(filename, "rb"); // Open file in binary read mode
  if (file == NULL) {
    perror("Failed to open file");
    return KB900X_E_ERR;
  }
  // Move to the end of the file to determine the file size
  fseek(file, 0, SEEK_END);
  *buffer_size =
      ftell(file); // Get the current position in the file, which is the size
  fseek(file, 0,
        SEEK_SET); // Reset the file position indicator to the start of the file

  if (*buffer_size <= 0) {
    perror("Failed to determine file size or file is empty");
    fclose(file);
    return KB900X_E_ERR;
  }

  // Allocate memory for the array
  *buffer = (uint8_t *)malloc(*buffer_size * sizeof(uint8_t));
  if (buffer == NULL) {
    perror("Failed to allocate memory");
    fclose(file);
    return KB900X_E_ERR;
  }

  // Read the file contents into the buffer
  size_t read_size = fread(*buffer, sizeof(uint8_t), *buffer_size, file);
  if (read_size != *buffer_size) {
    perror("Failed to read the entire file");
    free(*buffer);
    *buffer = NULL;
    fclose(file);
    return KB900X_E_ERR;
  }

  // Clean up
  fclose(file);

  return KB900X_E_OK;
}

const char *rc_to_string(int rc) {
  if (rc < 0) {
    rc = -rc;
  }
  switch (rc) {
  case KB900X_E_OK:
    return "Success";
  case KB900X_E_ERR:
    return "Generic error";
  case KB900X_E_PEC_NOT_SUPPORTED:
    return "PEC not supported";
  case KB900X_E_TX_ABORT:
    return "TX abort detected";
  case KB900X_E_BOOT_STATUS:
    return "Boot status error";
  case KB900X_E_NOT_IMPLEMENTED:
    return "Not implemented";
  case KB900X_E_UNKNOWN_REVID:
    return "Unknown or unsupported regli revision ID";
  case KB900X_E_FEATURE_REQ_FAILED:
    return "Feature request failed";
  default:
    return strerror(rc);
  }
}

int decode_u32(const uint8_t *bytes, uint32_t *dst, bool is_big_endian) {
  if (!bytes || !dst) {
    return -EINVAL;
  }

  if (is_big_endian) {
    *dst = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) |
           bytes[3]; // NOLINT
  } else {
    *dst = (bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) |
           bytes[0]; // NOLINT
  }

  return KB900X_E_OK;
}

int encode_u32(uint32_t value, uint8_t *dst, bool is_big_endian) {
  if (!dst) {
    return -EINVAL;
  }

  if (is_big_endian) {
    dst[0] = value >> 24;          // NOLINT
    dst[1] = (value >> 16) & 0xFF; // NOLINT
    dst[2] = (value >> 8) & 0xFF;  // NOLINT
    dst[3] = value & 0xFF;         // NOLINT
  } else {
    dst[3] = value >> 24;          // NOLINT
    dst[2] = (value >> 16) & 0xFF; // NOLINT
    dst[1] = (value >> 8) & 0xFF;  // NOLINT
    dst[0] = value & 0xFF;         // NOLINT
  }

  return KB900X_E_OK;
}
