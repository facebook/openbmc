#include "fw-util.h"
#include "mezz_nic.hpp"
#include <facebook/bic.h>
#include <openbmc/kv.h>
#include <libpldm/base.h>
#include <libpldm-oem/pldm.h>
#include <openbmc/pal.h>
#include <openbmc/libgpio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <cstdio>
#include <syslog.h>
#include <stdio.h>
#include <termios.h>

using namespace std;

class SwbBicFwComponent : public Component {
  protected:
    uint8_t bus, eid, target;
  public:
    SwbBicFwComponent(const string& fru, const string& comp, uint8_t bus, uint8_t eid, uint8_t target)
        : Component(fru, comp), bus(bus), eid(eid), target(target) {}
    int update(string image) override;
    int fupdate(string image) override;
    int get_version(json& j) override;
};

class SwbBicFwRecoveryComponent : public Component {
  protected:
    uint8_t bus, eid, target;

  public:
    SwbBicFwRecoveryComponent(const string& fru, const string& comp, uint8_t bus, uint8_t eid, uint8_t target)
        :Component(fru, comp), bus(bus), eid(eid), target(target) {}

  int update(string image) override;
  int fupdate(string image) override;
};

class SwbPexFwComponent : public SwbBicFwComponent {
  private:
    uint8_t id;
  public:
    SwbPexFwComponent(const string& fru, const string& comp, uint8_t bus, uint8_t eid, uint8_t target, uint8_t id)
        :SwbBicFwComponent(fru, comp, bus, eid, target), id(id) {}
    int get_version(json& j) override;
};

#define PexImageCount 4

struct PexImage {
  uint32_t offset;
  uint32_t size;
};

class SwbAllPexFwComponent : public Component {
  private:
    uint8_t bus, eid;
    uint8_t target = PEX0_COMP;
  public:
    SwbAllPexFwComponent(const string& fru, const string& comp, uint8_t bus, uint8_t eid)
        :Component(fru, comp), bus(bus), eid(eid) {}
    int update(string image);
    int fupdate(string image);
};

class SwbNicFwComponent : public SwbBicFwComponent {
  private:
    uint8_t id;
  public:
    SwbNicFwComponent(const string& fru, const string& comp, uint8_t bus, uint8_t eid, uint8_t target, uint8_t id)
        :SwbBicFwComponent(fru, comp, bus, eid, target), id(id) {}
    int get_version(json& j) override;
};


#define MD5_SIZE              (16)
#define PLAT_SIG_SIZE         (16)
#define FW_VER_SIZE           (4)
#define IMG_POSTFIX_SIZE      (MD5_SIZE + PLAT_SIG_SIZE + FW_VER_SIZE + 1)
#define BIC_IMG_SIZE          0x50000
#define PKT_SIZE              (64*1024)
#define HEAD_SIZE             (4)

#define MAX_PLDM_IPMI_PAYLOAD (224)

enum {
  SWB_FW_UPDATE_SUCCESS                =  0,
  SWB_FW_UPDATE_FAILED                 = -1,
  SWB_FW_UPDATE_NOT_SUPP_IN_CURR_STATE = -2,
};


struct image_info {
  struct stat stat;
  string tmpfile;
  bool result;
  bool sign;
};

image_info wrapping_image (const string& image, bool /*force*/)
{
  struct image_info image_stat;
  image_stat.tmpfile = "";
  image_stat.result = false;
  image_stat.sign = false;

  if (stat(image.c_str(), &image_stat.stat) < 0) {
    cerr << "Cannot check " << image << " file information" << endl;
    return image_stat;
  }

  // create a new tmp file
  image_stat.tmpfile = image + "-tmp";

  // open the binary
  int fd_r = open(image.c_str(), O_RDONLY);
  if (fd_r < 0) {
    cerr << "Cannot open " << image << " for reading" << endl;
    return image_stat;
  }

  // create a tmp file for writing.
  int fd_w = open(image_stat.tmpfile.c_str(), O_WRONLY | O_CREAT, 0666);
  if (fd_w < 0) {
    cerr << "Cannot write to " << image_stat.tmpfile << endl;
    close(fd_r);
    return image_stat;
  }

  uint8_t *memblock = new uint8_t [image_stat.stat.st_size + HEAD_SIZE];
  *memblock = image_stat.stat.st_size;
  *(memblock+1) = image_stat.stat.st_size >> 8;
  *(memblock+2) = image_stat.stat.st_size >> 16;
  *(memblock+3) = image_stat.stat.st_size >> 24;

  size_t r_size = read(fd_r, memblock+HEAD_SIZE, image_stat.stat.st_size);
  r_size += HEAD_SIZE;
  size_t w_size = write(fd_w, memblock, r_size);

    //check size
  if ( r_size != w_size ) {
    cerr << "Cannot create the tmp file - " << image_stat.tmpfile << endl;
    cerr << "Read: " << r_size << " Write: " << w_size << endl;
    goto exit;
  }

  image_stat.result = true;

exit:
  close(fd_r);
  close(fd_w);
  delete[] memblock;
  return image_stat;
}

int send_update_packet (int bus, int eid, uint8_t* buf, ssize_t bufsize,
                        uint8_t target, uint32_t offset) {
  uint8_t tbuf[255] = {0};
  uint8_t rbuf[255] = {0};
  uint8_t tlen=0;
  size_t rlen = 0;
  int rc;

  tbuf[tlen++] = target;
  tbuf[tlen++] = (offset) & 0xFF;
  tbuf[tlen++] = (offset >> 8 ) & 0xFF;
  tbuf[tlen++] = (offset >> 16) & 0xFF;
  tbuf[tlen++] = (offset >> 24) & 0xFF;
  tbuf[tlen++] = (bufsize) & 0xFF;
  tbuf[tlen++] = (bufsize >> 8) & 0xFF;
  // FW Image Data
  memcpy(tbuf+tlen, buf, bufsize);
  tlen += bufsize;


  rc = pldm_oem_ipmi_send_recv(bus, eid,
                               NETFN_OEM_1S_REQ, CMD_OEM_1S_UPDATE_FW,
                               tbuf, tlen,
                               rbuf, &rlen);
  return rc;
}

int swb_fw_update (uint8_t bus, uint8_t eid, uint8_t target, uint8_t* file_data, uint32_t file_size)
{
  int ret = SWB_FW_UPDATE_FAILED;

  // start update
  cout << "updating fw on bus " << +bus << endl;
  struct timeval start, end;
  uint8_t buf[256] = {0};
  uint32_t offset = 0;
  uint32_t boundary = PKT_SIZE;
  uint16_t read_count;
  ssize_t count;
  // Write chunks of binary data in a loop
  uint32_t dsize = file_size/10;
  uint32_t last_offset = 0;

  gettimeofday(&start, NULL);

  while (1)
  {
    // send packets in blocks of 64K
    read_count = ((offset + MAX_PLDM_IPMI_PAYLOAD) < boundary) ?
                  MAX_PLDM_IPMI_PAYLOAD : boundary - offset;

    if(offset >= file_size)
      break;


    if (file_size - offset > read_count) {
      count = read_count;
    } else {
      // last packet
      count = file_size - offset;
      target |= 0x80;
    }
//    if (offset + count >= file_size)

    memcpy(buf, file_data+offset, count);
    // Send data to Bridge-IC
    if (send_update_packet(bus, eid, buf, count, target, offset))
       goto exit;

    // Update counter
    offset += count;
    if (offset >= boundary) {
      boundary += PKT_SIZE;
    }

    if (last_offset + dsize <= offset) {
      pal_set_fw_update_ongoing(FRU_SWB, 60);
      printf("\rupdated: %d %%", offset/dsize*10);
      fflush(stdout);
      last_offset += dsize;
    }
  }

  ret = SWB_FW_UPDATE_SUCCESS;
  gettimeofday(&end, NULL);
  printf("\n");
  printf("Elapsed time:  %d   sec.\n", (int)(end.tv_sec - start.tv_sec));

exit:
  return ret;
}

static
int fw_update_proc (const string& image, bool /*force*/,
                    uint8_t bus, uint8_t eid,
                    uint8_t target, const string& comp)
{
  struct stat st;

  // open the binary
  int fd_r = open(image.c_str(), O_RDONLY);
  if (fd_r < 0) {
    cerr << "Cannot open " << image << " for reading" << endl;
    return FW_STATUS_FAILURE;
  }

  if (stat(image.c_str(), &st) < 0) {
    cerr << "Cannot check " << image << " file information" << endl;
    return FW_STATUS_FAILURE;
  }

  uint8_t *memblock = new uint8_t [st.st_size];
  size_t r_size = read(fd_r, memblock, st.st_size);

  //Check Signed image

  syslog(LOG_CRIT, "Component %s upgrade initiated\n", comp.c_str() );

  try {
    int ret = swb_fw_update(bus, eid, target, memblock, r_size);
    delete[] memblock;
    close(fd_r);
    if (ret != SWB_FW_UPDATE_SUCCESS) {
      switch(ret)
      {
        case SWB_FW_UPDATE_FAILED:
          cerr << comp << ": update process failed" << endl;
          break;
        case SWB_FW_UPDATE_NOT_SUPP_IN_CURR_STATE:
          cerr << comp << ": firmware update not supported in current state." << endl;
          break;
        default:
          cerr << comp << ": unknow error (ret: " << ret << ")" << endl;
          break;
      }
      return FW_STATUS_FAILURE;
    }

  } catch (string& err) {
    cerr << err << endl;
    return FW_STATUS_NOT_SUPPORTED;
  }
  syslog(LOG_CRIT, "Component %s upgrade completed\n", comp.c_str() );
  return 0;
}

int SwbBicFwComponent::update(string image)
{
  return fw_update_proc(image, false, bus, eid, target, this->alias_component());
}

int SwbBicFwComponent::fupdate(string image)
{
  return fw_update_proc(image, true, bus, eid, target, this->alias_component());
}

int
bic_recovery_init(string image, bool /*force*/) {

  if (gpio_set_value_by_shadow("BIC_FWSPICK", GPIO_VALUE_HIGH)) {
     syslog(LOG_WARNING, "[%s] Set BIC_FWSPICK High failed\n", __func__ );
     return -1;
  }

  if (gpio_set_value_by_shadow("BIC_UART_BMC_SEL", GPIO_VALUE_LOW)) {
     syslog(LOG_WARNING, "[%s] Set BIC_UART_BMC_SEL Low failed\n", __func__ );
     return -1;
  }

  if (gpio_set_value_by_shadow("RST_SWB_BIC_N", GPIO_VALUE_LOW)) {
     syslog(LOG_WARNING, "[%s] Set RST_SWB_BIC_N Low failed\n", __func__ );
     return -1;
  }
  sleep(1);

  if (gpio_set_value_by_shadow("RST_SWB_BIC_N", GPIO_VALUE_HIGH)) {
     syslog(LOG_WARNING, "[%s] Set RST_SWB_BIC_N High failed\n", __func__);
     return -1;
  }
  sleep(2);

  char cmd[64] = {0};
  snprintf(cmd, sizeof(cmd), "/bin/stty -F /dev/ttyS%d 115200", SWB_UART_ID);
  if (system(cmd) != 0) {
    syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
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
           head[0], head[1], head[2], head[3], SWB_UART_ID);
  if (system(cmd) != 0) {
    syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
    return -1;
  }

  snprintf(cmd, sizeof(cmd), "cat %s > /dev/ttyS%d", image.c_str(), SWB_UART_ID);
  if (system(cmd) != 0) {
    syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
    return -1;
  }

  snprintf(cmd, sizeof(cmd), "/bin/stty -F /dev/ttyS%d 57600", SWB_UART_ID);
  if (system(cmd) != 0) {
    syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
    return -1;
  }
  sleep(2);
  return 0;
}

//Recovery BIC Image
int SwbBicFwRecoveryComponent::update(string image)
{
  int ret;

  ret = bic_recovery_init(image, false);
  if (ret)
    return ret;

  ret = fw_update_proc(image, false, bus, eid, target, this->alias_component());

  sleep (2);
  if (gpio_set_value_by_shadow("BIC_FWSPICK", GPIO_VALUE_LOW)) {
     syslog(LOG_WARNING, "[%s] Set BIC_FWSPICK Low failed\n", __func__ );
     return -1;
  }

  if (gpio_set_value_by_shadow("RST_SWB_BIC_N", GPIO_VALUE_LOW)) {
     syslog(LOG_WARNING, "[%s] Set RST_SWB_BIC_N Low failed\n", __func__ );
     return -1;
  }
  sleep(1);

  if (gpio_set_value_by_shadow("RST_SWB_BIC_N", GPIO_VALUE_HIGH)) {
     syslog(LOG_WARNING, "[%s] Set RST_SWB_BIC_N High failed\n", __func__);
     return -1;
  }
  return ret;
}

int SwbBicFwRecoveryComponent::fupdate(string image)
{
  int ret;

  ret = bic_recovery_init(image, true);
  if (ret)
    return ret;

  return fw_update_proc(image, false, bus, eid, target, this->alias_component());
}

//Print Version
int get_swb_version (uint8_t bus, uint8_t eid, uint8_t target, vector<uint8_t> &data) {
  uint8_t tbuf[255] = {0};
  uint8_t rbuf[255] = {0};
  uint8_t tlen=0;
  size_t rlen = 0;
  int rc;

  tbuf[tlen++] = target;

  rc = pldm_oem_ipmi_send_recv(bus, eid,
                               NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_FW_VER,
                               tbuf, tlen,
                               rbuf, &rlen);
  if (rc == 0)
    data = vector<uint8_t>(rbuf, rbuf + rlen);
  return rc;
}


int SwbBicFwComponent::get_version(json& j) {
  vector<uint8_t> ver = {};

  // Get Bridge-IC Version
  int ret = get_swb_version(bus, eid, target, ver);

  if (ret != 0 || ver.empty()) {
    j["VERSION"] = "NA";
  } else if (ver[1] == 7){
    stringstream ver_str;
    ver_str << std::hex << std::setfill('0')
      << std::setw(2) << +ver[2]
      << std::setw(2) << +ver[3] << '.'
      << std::setw(2) << +ver[4] << '.'
      << std::setw(2) << +ver[5];
    j["VERSION"] = ver_str.str();
  } else {
    j["VERSION"] = "Format not supported";
  }
  if (pal_is_artemis()) {
    if (bus == ACB_BIC_BUS) {
      j["PRETTY_COMPONENT"] = "ACB BIC";
    } else {
      j["PRETTY_COMPONENT"] = "MEB BIC";
    }
  } else {
    j["PRETTY_COMPONENT"] = "SWB BIC";
  }
  return FW_STATUS_SUCCESS;
}

int SwbPexFwComponent::get_version(json& j) {
  vector<uint8_t> ver = {};

  // Get PEX Version
  int ret = get_swb_version(bus, eid, target, ver);
  if (ret != 0 || ver.empty()) {
    j["VERSION"] = "NA";
  } else if (ver[1] == 4){
    stringstream ver_str;
    ver_str << std::hex << std::setfill('0')
      << std::setw(2) << +ver[5] << '.'
      << std::setw(2) << +ver[4] << '.'
      << std::setw(2) << +ver[3] << '.'
      << std::setw(2) << +ver[2];
    j["VERSION"] = ver_str.str();
  } else {
    j["VERSION"] = "Format not supported";
  }
  if (pal_is_artemis()) {
    j["PRETTY_COMPONENT"] = string("ACB PESW") + std::to_string(+id);
  } else {
    j["PRETTY_COMPONENT"] = string("SWB PEX") + std::to_string(+id);
  }
  return FW_STATUS_SUCCESS;
}

static ssize_t
uint32_t_read (int fd, uint32_t& target)
{
  ssize_t ret = 0;
  uint8_t temp[PexImageCount];
  ret = read(fd, temp, sizeof(temp));
  if (ret >= 0) {
    target = 0;
    for (int i = 0; i < 4; ++i)
      target = (target << 8) + temp[i];
  }

  return ret;
}

int SwbAllPexFwComponent::fupdate(string image) {
  return update(image);
}

int SwbAllPexFwComponent::update(string image) {

  // open the binary
  int fd_r = open(image.c_str(), O_RDONLY);
  if (fd_r < 0) {
    cerr << "Cannot open " << image << " for reading" << endl;
    return -1;
  }

  uint32_t count = 0;
  if (uint32_t_read(fd_r, count) < 0)
    return -1;

  if (count != PexImageCount) {
    fprintf(stderr, "There should be %d images not %d.\n", PexImageCount, count);
    return -1;
  }

  struct PexImage images[PexImageCount];
  for (int i = 0; i < PexImageCount; ++i) {
    if (uint32_t_read(fd_r, images[i].offset) < 0) return -1;
    if (uint32_t_read(fd_r, images[i].size)   < 0) return -1;
  }

  syslog(LOG_CRIT, "Component %s upgrade initiated\n", this->alias_component().c_str() );
  int ret = 0;

  for (int i = 0; i < PexImageCount; ++i) {
    try {
      if (lseek(fd_r, images[i].offset, SEEK_SET) != (off_t)images[i].offset) {
        printf("%d\n", -errno);
        cerr << "Cannot seek " << image << endl;
        ret = -1;
        goto exit;
      }
      uint8_t *memblock = new uint8_t [images[i].size];
      if (read(fd_r, memblock, images[i].size) != (ssize_t)images[i].size) {
        cerr << "Cannot read " << image << endl;
        ret = -1;
        goto exit;
      }
      ret = swb_fw_update(bus, eid, PEX0_COMP+i, memblock, images[i].size);
      delete[] memblock;
      if (ret != SWB_FW_UPDATE_SUCCESS) {
        switch(ret)
        {
          case SWB_FW_UPDATE_FAILED:
            cerr << this->alias_component() << ": update process failed" << endl;
            break;
          case SWB_FW_UPDATE_NOT_SUPP_IN_CURR_STATE:
            cerr << this->alias_component() << ": firmware update not supported in current state." << endl;
            break;
          default:
            cerr << this->alias_component() << ": unknow error (ret: " << ret << ")" << endl;
            break;
        }
        ret = FW_STATUS_FAILURE;
      }

    } catch (string& err) {
      printf("%s\n", err.c_str());
      ret = FW_STATUS_NOT_SUPPORTED;
    }
  }

exit:
  syslog(LOG_CRIT, "Component %s upgrade completed\n", this->alias_component().c_str() );
  close(fd_r);
  return ret;
}

class fw_bic_config {
  public:
    fw_bic_config() {
      if (pal_is_artemis()) {
        static SwbBicFwComponent acb_bic("acb", "bic", ACB_BIC_BUS, ACB_BIC_EID, BIC_COMP);
        static SwbBicFwComponent meb_bic("meb", "bic", MEB_BIC_BUS, MEB_BIC_EID, BIC_COMP);
        static SwbPexFwComponent acb_pesw0("acb", "pesw0", ACB_BIC_BUS, ACB_BIC_EID, PEX0_COMP, 0);
        static SwbPexFwComponent acb_pesw1("acb", "pesw1", ACB_BIC_BUS, ACB_BIC_EID, PEX1_COMP, 1);
      } else {
        static SwbBicFwComponent bic("swb", "bic", 3, 0x0A, BIC_COMP);
        static SwbBicFwRecoveryComponent bic_recovery("swb", "bic_recovery", 3, 0x0A, BIC_COMP);
        static SwbPexFwComponent swb_pex0("swb", "pex0", 3, 0x0A, PEX0_COMP, 0);
        static SwbPexFwComponent swb_pex1("swb", "pex1", 3, 0x0A, PEX1_COMP, 1);
        static SwbPexFwComponent swb_pex2("swb", "pex2", 3, 0x0A, PEX2_COMP, 2);
        static SwbPexFwComponent swb_pex3("swb", "pex3", 3, 0x0A, PEX3_COMP, 3);
        static SwbAllPexFwComponent swb_pex("swb", "pex_all", 3, 0x0A);
      }
    }
};

fw_bic_config _fw_bic_config;
