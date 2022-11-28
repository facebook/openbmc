#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include <syslog.h>
#include "server.h"
#include <openbmc/pal.h>
#include <facebook/bic.h>
#include "ast_bic.h"
#include <sys/stat.h>
#include <unistd.h>

int retry_system(const char *cmd) {
  const int retry = 5;
  int ret = -1;

  for (int curr_try = 0; curr_try < retry; ++curr_try) {
   ret = system(cmd);
   if (ret == 0) {
     return ret;
   }
  }

  return ret;
}

int
bic_recovery_init(uint8_t slot_id, string image, bool /*force*/) {
  char cmd[64] = {0};

  // SET IO-expander OUTPUT value
  snprintf(cmd, sizeof(cmd), "i2cset -y %d 0x49 0x01 0x04", slot_id + 3);
  if (retry_system(cmd) != 0) {
    syslog(LOG_WARNING, "[%s] %s failed", __func__, cmd);
    return -1;
  }

  // SET BIC_FWSPICK_IO_EXP to high
  snprintf(cmd, sizeof(cmd), "i2cset -y %d 0x49 0x03 0xFB", slot_id + 3);
  if (retry_system(cmd) != 0) {
    syslog(LOG_WARNING, "[%s] %s failed", __func__, cmd);
    return -1;
  }

  // SET UART MUX to 1OU
  snprintf(cmd, sizeof(cmd), "i2cset -y %d 0x0F 0x01 0x01", slot_id + 3);
  if (retry_system(cmd) != 0) {
    syslog(LOG_WARNING, "[%s] %s failed", __func__, cmd);
    return -1;
  }

  sleep(1);
  // SET BIC_SRST_N_IO_EXP_R to low
  snprintf(cmd, sizeof(cmd), "i2cset -y %d 0x49 0x03 0xFA", slot_id + 3);
  if (retry_system(cmd) != 0) {
    syslog(LOG_WARNING, "[%s] %s failed", __func__, cmd);
    return -1;
  }
  sleep(1);
  // SET BIC_SRST_N_IO_EXP_R to high
  snprintf(cmd, sizeof(cmd), "i2cset -y %d 0x49 0x03 0xFB", slot_id + 3);
  if (retry_system(cmd) != 0) {
    syslog(LOG_WARNING, "[%s] %s failed", __func__, cmd);
    return -1;
  }
  sleep(2);

  snprintf(cmd, sizeof(cmd), "/bin/stty -F /dev/ttyS%d 115200", slot_id);
  if (retry_system(cmd) != 0) {
    syslog(LOG_WARNING, "[%s] %s failed", __func__, cmd);
    return -1;
  }

  struct stat st;
  if (stat(image.c_str(), &st) < 0) {
    cerr << "Cannot check " << image << " file information" << endl;
    return FW_STATUS_FAILURE;
  }

  uint8_t head[4];
  head[0] = st.st_size;
  head[1] = st.st_size >> 8;
  head[2] = st.st_size >> 16;
  head[3] = st.st_size >> 24;

  snprintf(cmd, sizeof(cmd), "echo -n -e '\\x%x\\x%x\\x%x\\x%x' > /dev/ttyS%d",
           head[0], head[1], head[2], head[3], slot_id);
  if (retry_system(cmd) != 0) {
    syslog(LOG_WARNING, "[%s] %s failed", __func__, cmd);
    return -1;
  }

  cout << "Start transmission over UART" << endl;
  snprintf(cmd, sizeof(cmd), "cat %s > /dev/ttyS%d", image.c_str(), slot_id);
  if (retry_system(cmd) != 0) {
    syslog(LOG_WARNING, "[%s] %s failed", __func__, cmd);
    return -1;
  }
  cout << "Transmission finish" << endl;

  snprintf(cmd, sizeof(cmd), "/bin/stty -F /dev/ttyS%d 57600", slot_id);
  if (retry_system(cmd) != 0) {
    syslog(LOG_WARNING, "[%s] %s failed", __func__, cmd);
    return -1;
  }
  sleep(2);
  return 0;
}

int AstBicFwRecoveryComponent::ast_bic_recovery(string image, bool /*force*/) {
  char cmd[64] = {0};
  int ret = 0;

  try {
    server.ready();
    expansion.ready();

    ret = bic_recovery_init(slot_id, image, false);
    if (ret) {
      cerr << this->alias_component() << ": bic_recovery_init process failed" << endl;
      return ret;
    }

    ret = bic_update_fw(slot_id, fw_comp, (char *)image.c_str(), FORCE_UPDATE_SET);
    if (ret) {
      cerr << this->alias_component() << ": bic_update_fw process failed" << endl;
      return ret;
    }
    sleep (2);
    // SET BIC_FWSPICK_IO_EXP to low
    snprintf(cmd, sizeof(cmd), "i2cset -y %d 0x49 0x03 0xFF", slot_id + 3);
    if (retry_system(cmd) != 0) {
      syslog(LOG_WARNING, "[%s] %s failed", __func__, cmd);
      return -1;
    }

    // SET BIC_SRST_N_IO_EXP_R to low
    snprintf(cmd, sizeof(cmd), "i2cset -y %d 0x49 0x03 0xFE", slot_id + 3);
    if (retry_system(cmd) != 0) {
      syslog(LOG_WARNING, "[%s] %s failed", __func__, cmd);
      return -1;
    }
    sleep(1);
    // SET BIC_SRST_N_IO_EXP_R to high
    snprintf(cmd, sizeof(cmd), "i2cset -y %d 0x49 0x03 0xFF", slot_id + 3);
    if (retry_system(cmd) != 0) {
      syslog(LOG_WARNING, "[%s] %s failed", __func__, cmd);
      return -1;
    }

  } catch (string& err) {
    printf("%s \n", err.c_str());
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

//Recovery BIC Image
int AstBicFwRecoveryComponent::update(string image)
{
  return ast_bic_recovery(image, FORCE_UPDATE_UNSET);
}

int AstBicFwRecoveryComponent::fupdate(string image)
{
  return ast_bic_recovery(image, FORCE_UPDATE_SET);
}
