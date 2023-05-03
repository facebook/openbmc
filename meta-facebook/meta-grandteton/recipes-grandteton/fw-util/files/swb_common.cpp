#include <openbmc/kv.hpp>
#include <libpldm/base.h>
#include <libpldm-oem/pldm.h>
#include <libpldm-oem/fw_update.hpp>
#include <libpldm-oem/pal_pldm.hpp>
#include <openbmc/libgpio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <iomanip>
#include <vector>
#include <array>
#include <cstdio>
#include <syslog.h>
#include <stdio.h>
#include <termios.h>
#include <algorithm>
#include <fmt/format.h>
#include "swb_common.hpp"

#define PexImageCount 4
#define GTAPeswImageCount 2

struct PexImage {
  uint32_t offset;
  uint32_t size;
};


#define MD5_SIZE              (16)
#define PLAT_SIG_SIZE         (16)
#define FW_VER_SIZE           (4)
#define IMG_POSTFIX_SIZE      (MD5_SIZE + PLAT_SIG_SIZE + FW_VER_SIZE + 1)
#define BIC_IMG_SIZE          0x50000
#define PKT_SIZE              (64*1024)
#define HEAD_SIZE             (4)

#define MAX_PLDM_IPMI_PAYLOAD (224)

// Artemis CXL FW PAYLOAD
#define MAX_CXL_PLDM_IPMI_PAYLOAD (128)
#define MAX_CXL_FW_UPDATE_RETRY   (5)

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


  rc = oem_pldm_ipmi_send_recv(bus, eid,
                               NETFN_OEM_1S_REQ, CMD_OEM_1S_UPDATE_FW,
                               tbuf, tlen,
                               rbuf, &rlen, true);
  return rc;
}

int swb_fw_update (uint8_t fruid, uint8_t bus, uint8_t eid, uint8_t target, uint8_t* file_data, uint32_t file_size, const string& comp)
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
  int retry = MAX_CXL_FW_UPDATE_RETRY;
  uint8_t txbuf[MAX_TXBUF_SIZE] = {0};
  uint8_t rxbuf[MAX_RXBUF_SIZE] = {0};
  uint8_t txlen = 0;
  size_t rxlen = 0;


  gettimeofday(&start, NULL);

  while (1)
  {
    // CXL send packets in blocks of 180 bytes
    if (comp.compare(0, 3, "cxl") == 0) {
      read_count = ((offset + MAX_CXL_PLDM_IPMI_PAYLOAD) < boundary) ?
                    MAX_CXL_PLDM_IPMI_PAYLOAD : boundary - offset;
    } else {
    // send packets in blocks of 64K
      read_count = ((offset + MAX_PLDM_IPMI_PAYLOAD) < boundary) ?
                  MAX_PLDM_IPMI_PAYLOAD : boundary - offset;
    }

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

    if (send_update_packet(bus, eid, buf, count, target, offset)) {
      retry --;
      if (retry >= 0) {
        sleep(1);
        continue;
      } else {
        cout << endl << "Retry reached MAX, update failed!" << endl;
        goto exit;
      }
    }

    retry = MAX_CXL_FW_UPDATE_RETRY;
    // Update counter
    offset += count;
    if (offset >= boundary) {
      boundary += PKT_SIZE;
    }

    if (last_offset + dsize <= offset) {
      pal_set_fw_update_ongoing(fruid, 120);
      printf("\rupdated: %d %%",(int)(offset/dsize*10));
      fflush(stdout);
      last_offset += dsize;
    }
  }

  // Artemis CXL needs activation
  if (comp.compare(0, 3, "cxl") == 0) {
    cout << endl << "Update success, sleep 10 secs and active " << comp << endl;
    sleep(10);
    txbuf[0] = target;
    txlen += 1;
    ret = oem_pldm_ipmi_send_recv(bus, eid,
                                NETFN_OEM_1S_REQ, CMD_OEM_1S_ACTIVE_CXL,
                                txbuf, txlen,
                                rxbuf, &rxlen, true);
    if (ret < 0) {
      cout << endl << "Active " << comp << " Failed!" << endl;
      goto exit;
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
                    uint8_t target, const string& comp, const string& fru)
{
  struct stat st;
  uint8_t fruid = 0;

  pal_get_fru_id((char *)fru.c_str(), &fruid);

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

    int ret = swb_fw_update(fruid, bus, eid, target, memblock, r_size, comp);
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
  return fw_update_proc(image, false, bus, eid, target, this->alias_component(), this->alias_fru());
}

int SwbBicFwComponent::fupdate(string image)
{
  return fw_update_proc(image, true, bus, eid, target, this->alias_component(), this->alias_fru());
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

  ret = fw_update_proc(image, false, bus, eid, target, this->alias_component(), this->alias_fru());

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

  return fw_update_proc(image, false, bus, eid, target, this->alias_component(), this->alias_fru());
}

//Print Version
int get_swb_version (uint8_t bus, uint8_t eid, uint8_t target, vector<uint8_t> &data) {
  uint8_t tbuf[255] = {0};
  uint8_t rbuf[255] = {0};
  uint8_t tlen=0;
  size_t rlen = 0;
  int rc;

  tbuf[tlen++] = target;

  rc = oem_pldm_ipmi_send_recv(bus, eid,
                               NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_FW_VER,
                               tbuf, tlen,
                               rbuf, &rlen, true);
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
    char ver_char = '0';
    if (pal_is_artemis()) {
      ver_char = ver[6];
      ver_str << ver_char;
      ver_char = ver[7];
      ver_str << ver_char;
    }
    ver_str << std::hex << std::setfill('0')
      << std::setw(2) << +ver[2]
      << std::setw(2) << +ver[3] << '.'
      << std::setw(2) << +ver[4] << '.'
      << std::setw(2) << +ver[5];
    j["VERSION"] = ver_str.str();
  } else {
    j["VERSION"] = "Format not supported";
  }


  stringstream comp_str;
  comp_str << this->alias_fru()
           << ' '
           << this->alias_component();

  string rw_str = comp_str.str();
  transform(rw_str.begin(),rw_str.end(), rw_str.begin(),::toupper);

  j["PRETTY_COMPONENT"] = rw_str;
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

  stringstream comp_str;
  comp_str << this->alias_fru()
           << ' '
           << this->alias_component();

  string rw_str = comp_str.str();
  transform(rw_str.begin(),rw_str.end(), rw_str.begin(),::toupper);

  j["PRETTY_COMPONENT"] = rw_str;

  return FW_STATUS_SUCCESS;
}

int MebCxlFwComponent::get_version(json& j) {
  vector<uint8_t> ver = {};
  int ret = 0;

  // Check if post complete
  if (pal_bios_completed(FRU_MB)) {
    // Get Meb Cxl Version
    ret = get_swb_version(bus, eid, target, ver);

    if (ret != 0 || ver.empty()) {
      j["VERSION"] = "NA";
    } else if (ver[1] == MEB_CXL_FW_RESP_LEN) {
      stringstream ver_str;
      // ver[0] = component id, ver[1] = fw version length
      for(std::size_t i = 2; i <= MEB_CXL_FW_RESP_LEN; i ++) {
        ver_str << std::hex << ver[i];
      }
      j["VERSION"] = ver_str.str();
    } else {
      j["VERSION"] = "Format not supported";
    }
  } else {
    j["VERSION"] = "NA";
  }

  stringstream comp_str;
  comp_str << this->alias_fru()
           << ' '
           << this->alias_component();

  string rw_str = comp_str.str();
  transform(rw_str.begin(),rw_str.end(), rw_str.begin(),::toupper);

  j["PRETTY_COMPONENT"] = rw_str;
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

int SwbPexFwComponent::fupdate(string image) {
  return update(image);
}

int SwbPexFwComponent::update(string image) {

  uint8_t fruid = 0;
  uint8_t image_count = 0;

  if (pal_is_artemis()) {
    image_count = GTAPeswImageCount;
  } else {
    image_count = PexImageCount;
  }

  pal_get_fru_id((char *)this->alias_fru().c_str(), &fruid);

  // open the binary
  int fd_r = open(image.c_str(), O_RDONLY);
  if (fd_r < 0) {
    cerr << "Cannot open " << image << " for reading" << endl;
    return -1;
  }

  uint32_t count = 0;
  if (uint32_t_read(fd_r, count) < 0)
    return -1;

  if (count != image_count) {
    fprintf(stderr, "There should be %d images not %d.\n", image_count, (int)count);
    return -1;
  }

  struct PexImage images[PexImageCount];
  for (int i = 0; i < image_count; ++i) {
    if (uint32_t_read(fd_r, images[i].offset) < 0) return -1;
    if (uint32_t_read(fd_r, images[i].size)   < 0) return -1;
  }

  int num = target - PEX0_COMP;

  syslog(LOG_CRIT, "Component %s upgrade initiated\n", this->alias_component().c_str() );
  int ret = 0;

  try {
    if (lseek(fd_r, images[num].offset, SEEK_SET) != (off_t)images[num].offset) {
      printf("%d\n", -errno);
      cerr << "Cannot seek " << image << endl;
      ret = -1;
      goto exit;
    }
    uint8_t *memblock = new uint8_t [images[num].size];
    if (read(fd_r, memblock, images[num].size) != (ssize_t)images[num].size) {
      cerr << "Cannot read " << image << endl;
      ret = -1;
      goto exit;
    }
    ret = swb_fw_update(fruid, bus, eid, target, memblock, images[num].size, this->alias_component());
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

exit:
  syslog(LOG_CRIT, "Component %s upgrade completed\n", this->alias_component().c_str() );
  close(fd_r);
  return ret;
}

static
int set_swb_snr_polling (uint8_t status) {
  uint8_t tbuf[255] = {0};
  uint8_t rbuf[255] = {0};
  uint8_t tlen=0;
  size_t rlen = 0;
  int rc;

  tbuf[tlen++] = status;

  rc = oem_pldm_ipmi_send_recv(SWB_BUS_ID, SWB_BIC_EID,
                               NETFN_OEM_1S_REQ,
                               CMD_OEM_1S_DISABLE_SEN_MON,
                               tbuf, tlen,
                               rbuf, &rlen, true);

  cout << __func__ << " rc = " << rc << endl;
  return rc;
}

int SwbVrComponent::update_proc(string image, bool force) {
  int ret;
  string comp = this->component();

  if (vr_probe() < 0) {
    cout << "VR probe failed!" << endl;
    return -1;
  }

  syslog(LOG_CRIT, "Component %s upgrade initiated", comp.c_str());
  ret = vr_fw_update(name.c_str(), (char *)image.c_str(), force);
  if (ret < 0) {
    cout << "ERROR: VR Firmware update failed!" << endl;
  } else {
    syslog(LOG_CRIT, "Component %s %s completed", comp.c_str(), force? "force upgrade": "upgrade");
  }

  vr_remove();
  return ret;
}

int SwbVrComponent::update(string image) {
  int ret;

  ret = set_swb_snr_polling(0x00);
  if (ret)
   return ret;

  ret = update_proc(image, 0);

  if(set_swb_snr_polling(0x01))
    cout << "set snr polling start fail." << endl;

  return ret;
}

int SwbVrComponent::fupdate(string image) {
  int ret;

  ret = set_swb_snr_polling(0x00);
  if (ret)
   return ret;

  ret = update_proc(image, 1);

  if(set_swb_snr_polling(0x01))
    cout << "set snr polling start fail." << endl;

  return ret;
}

int GTPldmComponent::is_pldm_info_valid()
{
  signed_header_t img_info{};
  auto dev_records = oem_get_pkg_device_record();
  auto comp_infos = oem_get_pkg_comp_img_info();

  // right now, device record should be only one
  for(auto&descriper:dev_records[0].recordDescriptors) {
    if (descriper.type == PLDM_FWUP_VENDOR_DEFINED) {
      string type = descriper.data.c_str()+2;
      string data = descriper.data.c_str()+2+descriper.data.at(1);
      type.resize(descriper.data.at(1));
      data.resize(descriper.data.size()-descriper.data.at(1)-2);
      if (type == "Platform") {
        img_info.project_name = data;
      } else if (type == "BoardID") {
        auto& map = pldm_signed_info::board_map;
        img_info.board_id = (map.find(data)!=map.end()) ? map.at(data):0xFF;
      } else if (type == "Stage") {
        auto& map = pldm_signed_info::stage_map;
        img_info.stage_id = (map.find(data)!=map.end()) ? map.at(data):0xFF;
      }
    }
  }

  if (comp_info.component_id != COMPONENT_VERIFY_SKIPPED) {
    for(auto&comp:comp_infos) {
      if (comp.compImageInfo.comp_identifier == comp_info.component_id) {
        string vendor;
        comp_verify_str = comp.compVersion; // comp_verify()
        stringstream input_stringstream(comp.compVersion);
        if (getline(input_stringstream, vendor, ' ')) {
          auto& map = pldm_signed_info::vendor_map;
          img_info.vendor_id = (map.find(vendor)!=map.end()) ? map.at(vendor):0xFF;
        }
        img_info.component_id = comp.compImageInfo.comp_identifier;
      }
    }
  } else {
    // multi comp image should have the same vendor
    string vendor;
    stringstream input_stringstream(comp_infos[0].compVersion);
    if (getline(input_stringstream, vendor, ' ')) {
      auto& map = pldm_signed_info::vendor_map;
      img_info.vendor_id = (map.find(vendor)!=map.end()) ? map.at(vendor):0xFF;
    }
  }

  return check_header_info(img_info) || comp_verify();
}

int GTPldmComponent::try_pldm_update(const string& image, bool force, uint8_t specified_comp)
{
  int isValidImage = oem_parse_pldm_package(image.c_str());

  // Legacy way
  if (is_pldm_supported(bus, eid) < 0) {
    if (force) {
      return comp_fupdate(image);
    } else {
      // PLDM image
      if (isValidImage == 0) {
        // Try to discard PLDM header
        // Pex not support.
        if (component.find("pex") != std::string::npos) {
          cerr << "Pex not support PLDM image transfer" << endl;
          return -1;
        } else {
          string raw_image{};
          if (get_raw_image(image, raw_image) == FORMAT_ERR::SUCCESS) {
            int ret = comp_fupdate(raw_image);
            del_raw_image();
            return ret;
          } else {
            cerr << "Can not do PLDM image transfer" << endl;
            return -1;
          }
        }
      // Raw image
      } else {
        return comp_update(image);
      }
    }
  // PLDM way
  } else {
    if (force) {
      if (isValidImage == 0) {
        cout << "PLDM image detected." << endl;
        return pldm_update(image, specified_comp);
      } else {
        cout << "Raw image detected." << endl;
        return comp_update(image);
      }
    } else {
      if (isValidImage == 0) {
        if (is_pldm_info_valid() == 0) {
          return pldm_update(image, specified_comp);
        } else {
          cerr << "Non-valid package info." << endl;
          return -1;
        }
      } else {
        cerr << "Non-valid image header." << endl;
        return -1;
      }
    }
  }
}

int GTPldmComponent::gt_get_version(json& j, const string& fru, const string& comp, uint8_t target)
{
  string active_key = fmt::format("swb_{}_active_ver",  pldm_signed_info::comp_str_t.at(target));
  string active_ver;

  // TODO: will remove ipmi get version while pldm get version is supported
  if (pal_is_artemis()) {
    return comp_version(j);
  }

  try {
    active_ver = kv::get(active_key, kv::region::temp);
    j["VERSION"] = active_ver;
  } catch (...) {
    if (pal_pldm_get_firmware_parameter(bus, eid) < 0) {
      return comp_version(j);
    } else {
      active_ver = kv::get(active_key, kv::region::temp);
      j["VERSION"] = (active_ver.empty()) ? "NA" : active_ver;
    }
  }

  stringstream comp_str;
  comp_str << fru << ' ' << comp;
  string rw_str = comp_str.str();
  transform(rw_str.begin(),rw_str.end(), rw_str.begin(),::toupper);
  j["PRETTY_COMPONENT"] = rw_str;

  return FW_STATUS_SUCCESS;
}

int GTSwbBicFwComponent::update(string image)
{
  return try_pldm_update(image, false);
}

int GTSwbBicFwComponent::fupdate(string image)
{
  return try_pldm_update(image, true);
}

int GTSwbBicFwComponent::get_version(json& j) {
  return gt_get_version(j, this->alias_fru(), this->alias_component(), target);
}

int GTSwbPexFwComponent::update(string image)
{
  return try_pldm_update(image, false, target);
}

int GTSwbPexFwComponent::fupdate(string image)
{
  return try_pldm_update(image, true, target);
}

int GTSwbPexFwComponent::get_version(json& j) {
  return gt_get_version(j, this->alias_fru(), this->alias_component(), target);
}

int GTSwbVrComponent::update(string image)
{
  return try_pldm_update(image, false);
}

int GTSwbVrComponent::fupdate(string image)
{
  return try_pldm_update(image, true);
}

int GTSwbVrComponent::get_version(json& j) {
  return gt_get_version(j, this->alias_fru(), this->alias_component(), target);
}

int GTSwbVrComponent::comp_verify()
{
  // MPS no need to check CRC
  if (comp_info.vendor_id == pldm_signed_info::MPS)
    return 0;

  // in case that cache not yet established.
  auto img_comp_info = oem_get_pkg_comp_img_info();
  json json_ver;
  gt_get_version(json_ver, this->alias_fru(), this->alias_component(), target);

  // variable for parsing
  string ver_str = json_ver["VERSION"];
  vector<string> words{};
  string ver_checksum{};
  string img_checksum{};
  string token{};
  size_t pos;

  // get current checksum
  while ((pos = ver_str.find(' ')) != std::string::npos) {
    token = ver_str.substr(0, pos);
    words.push_back(token);
    ver_str.erase(0, pos + 1);
  }
  words.push_back(ver_str);
  if (words.size() < 2) {
    cerr << "Can not get current version checksum." << endl;
    return -1;
  } else if (words[1].size() < 8) {
    cerr << "Can not get valid current version checksum." << endl;
    return -1;
  } else {
    ver_checksum = words[1];
    ver_checksum.resize(8);
  }

  // get image checksum
  words.clear();
  while ((pos = comp_verify_str.find(' ')) != std::string::npos) {
    token = comp_verify_str.substr(0, pos);
    words.push_back(token);
    comp_verify_str.erase(0, pos + 1);
  }
  words.push_back(comp_verify_str);
  if (words.size() < 2) {
    cerr << "Can not get image version checksum." << endl;
    return -1;
  } else if (words[1].size() < 8) {
    cerr << "Can not get valid image version checksum." << endl;
    return -1;
  } else {
    img_checksum = words[1];
    img_checksum.resize(8);
  }

  if (ver_checksum == img_checksum) {
    cerr << "Detected the same CRC, please use --force." << endl;
    return -1;
  } else
    return 0;
}

int GTSwbCpldComponent::update(string image)
{
  return try_pldm_update(image, false);
}

int GTSwbCpldComponent::fupdate(string image)
{
  return try_pldm_update(image, true);
}

int GTSwbCpldComponent::get_version(json& j) {
  return gt_get_version(j, this->alias_fru(), this->alias_component(), target);
}

int AcbPeswFwComponent::get_version(json &j) {
  vector<uint8_t> ver = {};

  // Get PESW Version
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

  stringstream comp_str;
  comp_str << this->alias_fru()
           << ' '
           << this->alias_component();

  string rw_str = comp_str.str();
  transform(rw_str.begin(),rw_str.end(), rw_str.begin(),::toupper);

  j["PRETTY_COMPONENT"] = rw_str;

  return FW_STATUS_SUCCESS;
}

static int
bic_sensor_polling_enabled(uint8_t sensor_num, bool enable)
{
  uint8_t tbuf[255] = {0};
  uint8_t rbuf[255] = {0};
  uint8_t tlen=0;
  size_t rlen = 0;
  int rc;

  tbuf[tlen++] = 0x01;
  tbuf[tlen++] = (enable) ? 0x01 : 0x00;
  tbuf[tlen++] = sensor_num;

  rc = oem_pldm_ipmi_send_recv(SWB_BUS_ID, SWB_BIC_EID,
                               NETFN_OEM_1S_REQ, CMD_OEM_1S_SET_DELAY_ACTIVATE_SYSFW,
                               tbuf, tlen,
                               rbuf, &rlen, true);
  return rc;
}

int SwbPLDMNicComponent::update(string image)
{
  int ret = 0;
  auto& map = pldm_signed_info::swb_nic_t;

  if (map.find(_ver_key) == map.end()) {
    std::cerr << "Nic card key error, unable to disable bic polling." << std::endl;
    ret = -1;
  } else {
    bic_sensor_polling_enabled(map.at(_ver_key), false);
    ret = PLDMNicComponent::update(image);
    bic_sensor_polling_enabled(map.at(_ver_key), true);
  }

  return ret;
}

namespace pldm_signed_info
{
  class swb_fw_common_config {
    public:
      swb_fw_common_config() {
        if (!pal_is_artemis()) {
          static GTSwbBicFwComponent bic("swb", "bic", SWB_BUS_ID, SWB_BIC_EID, BIC_COMP,
                        signed_header_t(gt_swb_comps, BIC_COMP, ASPEED));

          static GTSwbPexFwComponent swb_pex0("swb", "pex0", SWB_BUS_ID, SWB_BIC_EID, PEX0_COMP,
                            signed_header_t(gt_swb_comps, COMPONENT_VERIFY_SKIPPED, BROADCOM));
          static GTSwbPexFwComponent swb_pex1("swb", "pex1", SWB_BUS_ID, SWB_BIC_EID, PEX1_COMP,
                            signed_header_t(gt_swb_comps, COMPONENT_VERIFY_SKIPPED, BROADCOM));
          static GTSwbPexFwComponent swb_pex2("swb", "pex2", SWB_BUS_ID, SWB_BIC_EID, PEX2_COMP,
                            signed_header_t(gt_swb_comps, COMPONENT_VERIFY_SKIPPED, BROADCOM));
          static GTSwbPexFwComponent swb_pex3("swb", "pex3", SWB_BUS_ID, SWB_BIC_EID, PEX3_COMP,
                            signed_header_t(gt_swb_comps, COMPONENT_VERIFY_SKIPPED, BROADCOM));

          static GTSwbVrComponent vr_pex0_vcc("swb", "pex01_vcc", "VR_PEX01_VCC", SWB_BUS_ID, SWB_BIC_EID, VR0_COMP,
                            signed_header_t(gt_swb_comps, VR0_COMP));
          static GTSwbVrComponent vr_pex1_vcc("swb", "pex23_vcc", "VR_PEX23_VCC", SWB_BUS_ID, SWB_BIC_EID, VR1_COMP,
                            signed_header_t(gt_swb_comps, VR1_COMP));

          static GTSwbCpldComponent swb_cpld("swb", "swb_cpld", LCMXO3_9400C, SWB_BUS_ID, 0x40, &cpld_pldm_wr,
                            SWB_BIC_EID, CPLD_COMP, signed_header_t(gt_swb_comps, CPLD_COMP, LATTICE));
        }
      }
  };
  swb_fw_common_config _swb_fw_common_config;
}
