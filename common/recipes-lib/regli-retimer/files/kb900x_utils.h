#ifndef _KB_UTILS_H
#define _KB_UTILS_H

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "kb900x_log.h"

// Custom Error numbers
#define KB900X_E_OK (0) /* Success */

#define KB900X_E_ERR (1000)                /* generic error */
#define KB900X_E_PEC_NOT_SUPPORTED (1001)  /* PEC not supported */
#define KB900X_E_TX_ABORT (1002)           /* TX abort detected */
#define KB900X_E_BOOT_STATUS (1003)        /* Boot status error */
#define KB900X_E_NOT_IMPLEMENTED (1004)    /* Not implemented */
#define KB900X_E_UNKNOWN_REVID (1005)      /* Unknown chip rev id */
#define KB900X_E_FEATURE_REQ_FAILED (1006) /* Feature request failed */

// Macros for error handling
// Macro for ioctl return codes
#define CHECK_IOCTL(rc)                                                        \
  {                                                                            \
    if (rc < 0) {                                                              \
      KANDOU_ERR("Error: unexpected return code: %d - %s", rc,                 \
                 strerror(errno));                                             \
      return -errno;                                                           \
    }                                                                          \
  }
// Macro for ioctl return codes with custom message
#define CHECK_IOCTL_MSG(rc, ...)                                               \
  {                                                                            \
    if (rc < 0) {                                                              \
      KANDOU_ERR(__VA_ARGS__);                                                 \
      return -errno;                                                           \
    }                                                                          \
  }

// Macro for the case when no message is provided
#define CHECK_SUCCESS(rc)                                                      \
  {                                                                            \
    if (rc < 0) {                                                              \
      KANDOU_ERR("Error: unexpected return code %d - %s", rc,                  \
                 rc_to_string(rc));                                            \
      return rc;                                                               \
    }                                                                          \
  }

// Macro for the case when a message is provided
#define CHECK_SUCCESS_MSG(rc, ...)                                             \
  {                                                                            \
    if ((rc) < 0) {                                                            \
      KANDOU_ERR(__VA_ARGS__);                                                 \
      return (rc);                                                             \
    }                                                                          \
  }

/**
 * \brief Read a binary file and store it in a buffer.
 *
 * \param[in] filename the name (path) of the file to read
 * \param[out] buffer the pointer to the buffer to store the file content
 * \param[out] buffer_size the size of the buffer
 *
 * \note The buffer is allocated in this function and must be freed by the
 * caller
 *
 * \return 0 if no error, else the error code
 */
// int read_file(const char *filename, uint8_t **buffer, size_t *buffer_size);

/** \brief Convert an error code to a string
 *
 * \note This function compares the absolute value of the error code.
 *
 * \param[in] rc the error code.
 */
const char *rc_to_string(int rc);

#endif // _KB_UTILS_H
