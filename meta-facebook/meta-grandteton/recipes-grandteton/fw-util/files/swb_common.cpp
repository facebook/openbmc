#include <openbmc/kv.hpp>
#include <libpldm/base.h>
#include <libpldm-oem/pldm.h>
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
      if (comp.compare(0, 3, "cxl") == 0) {
        retry--;
        if (retry >= 0) {
          sleep(1);
          continue;
        } else {
          cout << endl << "Retry reached MAX, update failed!" << endl;
          goto exit;
        }
      }
      goto exit;
    }

    retry = MAX_CXL_FW_UPDATE_RETRY;
    // Update counter
    offset += count;
    if (offset >= boundary) {
      boundary += PKT_SIZE;
    }

    if (last_offset + dsize <= offset) {
      pal_set_fw_update_ongoing(fruid, 60);
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

  if (count != PexImageCount) {
    fprintf(stderr, "There should be %d images not %d.\n", PexImageCount, (int)count);
    return -1;
  }

  struct PexImage images[PexImageCount];
  for (int i = 0; i < PexImageCount; ++i) {
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

void GTPldmComponent::store_device_id_record(
                          pldm_firmware_device_id_record& /*id_record*/,
                                                uint16_t& descriper_type,
                                          variable_field& descriper_data)
{
  if (descriper_type == PLDM_FWUP_VENDOR_DEFINED) {
    string type = (const char*)descriper_data.ptr+2;
    string data = (const char*)descriper_data.ptr+2+descriper_data.ptr[1];
    type.resize(descriper_data.ptr[1]);
    data.resize(descriper_data.length-descriper_data.ptr[1]-2);
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

void GTPldmComponent::store_comp_img_info(
                          pldm_component_image_information& comp_info,
                                            variable_field& comp_verstr)
{
  // comp id
  img_info.component_id = comp_info.comp_identifier;

  // comp vendor
  string vendor, verstr = (const char *)comp_verstr.ptr;
  verstr.resize(comp_verstr.length);
  stringstream input_stringstream(verstr);
  if (getline(input_stringstream, vendor, '_')) {
    auto& map = pldm_signed_info::vendor_map;
    img_info.vendor_id = (map.find(vendor)!=map.end()) ? map.at(vendor):0xFF;
  }
}

void GTPldmComponent::store_firmware_parameter(
                 pldm_get_firmware_parameters_resp& /*fwParams*/,
                                    variable_field& activeCompImageSetVerStr,
                                    variable_field& /*pendingCompImageSetVerStr*/,
                    pldm_component_parameter_entry& compEntry,
                                    variable_field& activeCompVerStr,
                                    variable_field& /*pendingCompVerStr*/)
{
  string bic_ver = (const char*)activeCompImageSetVerStr.ptr;
  string bic_key = "swb_bic_active_ver";
  string comp_ver = (const char*)activeCompVerStr.ptr;
  string comp_key = fmt::format("swb_{}_active_ver", pldm_signed_info::comp_str_t.at(compEntry.comp_identifier));

  bic_ver.resize(activeCompImageSetVerStr.length-1);
  comp_ver.resize(activeCompVerStr.length);

  kv::set(bic_key,  bic_ver);
  kv::set(comp_key, comp_ver);
}

int GTPldmComponent::gt_get_version(json& j, const string& fru, const string& comp, uint8_t target)
{
  string comp_key = fmt::format("swb_{}_active_ver", pldm_signed_info::comp_str_t.at(target));
  string comp_ver;

  try {
    comp_ver = kv::get(comp_key, kv::region::temp);
    j["VERSION"] = comp_ver;
  } catch (...) {
    get_firmware_parameter();
    comp_ver = kv::get(comp_key, kv::region::temp);
    if (!comp_ver.empty()) {
      j["VERSION"] = comp_ver;
    } else {
      j["VERSION"] = "NA";
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
  int ret = try_pldm_update(image, false);
  if (ret == 0)
    get_firmware_parameter();
  return ret;
}

int GTSwbBicFwComponent::fupdate(string image)
{
  int ret = try_pldm_update(image, true);
  if (ret == 0)
    get_firmware_parameter();
  return ret;
}

// int GTSwbBicFwComponent::get_version(json& j) {
//   return gt_get_version(j, this->alias_fru(), this->alias_component(), target);
// }

int GTSwbPexFwComponent::update(string image)
{
  int ret = try_pldm_update(image, false);
  if (ret == 0)
    get_firmware_parameter();
  return ret;
}

int GTSwbPexFwComponent::fupdate(string image)
{
  int ret = try_pldm_update(image, true);
  if (ret == 0)
    get_firmware_parameter();
  return ret;
}

// int GTSwbPexFwComponent::get_version(json& j) {
//   return gt_get_version(j, this->alias_fru(), this->alias_component(), target);
// }

int GTSwbVrComponent::update(string image)
{
  int ret = try_pldm_update(image, false);
  if (ret == 0)
    get_firmware_parameter();
  return ret;
}

int GTSwbVrComponent::fupdate(string image)
{
  int ret = try_pldm_update(image, true);
  if (ret == 0)
    get_firmware_parameter();
  return ret;
}

int GTSwbVrComponent::get_version(json& j) {
  return gt_get_version(j, this->alias_fru(), this->alias_component(), target);
}

int GTSwbCpldComponent::update(string image)
{
  int ret = try_pldm_update(image, false);
  if (ret == 0)
    get_firmware_parameter();
  return ret;
}

int GTSwbCpldComponent::fupdate(string image)
{
  int ret = try_pldm_update(image, true);
  if (ret == 0)
    get_firmware_parameter();
  return ret;
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