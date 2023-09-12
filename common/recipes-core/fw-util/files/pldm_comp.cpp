#include "pldm_comp.hpp"
#include <fcntl.h>    // for open()
#include <syslog.h>   // for syslog
#include <unistd.h>   // for close()
#include <libpldm-oem/fw_update.hpp>
#include <libpldm-oem/oem_pldm.hpp>
#include <cstdio>

using namespace std;

int PldmComponent::find_image_index(uint8_t target_id) const
{
  if (comp_info.component_id == COMPONENT_VERIFY_SKIPPED)
    return -1;

  auto comp_infos = oem_get_pkg_comp_img_info();
  auto it = std::find_if(comp_infos.begin(), comp_infos.end(),
    [target_id](const component_image_info_t& comp) {
      return comp.compImageInfo.comp_identifier == target_id;
    }
  );

  if (it != comp_infos.end())
    return it-comp_infos.begin();

  return -1;
}

int PldmComponent::get_raw_image(const string& image, string& raw_image)
{
  int ret = FORMAT_ERR::SUCCESS;
  int pldm_fd = open(image.c_str(), O_RDONLY);
  if (pldm_fd < 0) {
    cerr << "Cannot open " << image << endl;
    return FORMAT_ERR::INVALID_FILE;
  }

  raw_image = "/tmp/"+fru+"_"+component+"_pldm_raw_image.bin";
  int raw_fd = open(raw_image.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (raw_fd < 0) {
    close(pldm_fd);
    cerr << "Cannot open " << raw_image << endl;
    return FORMAT_ERR::INVALID_FILE;
  }

  auto comp_infos = oem_get_pkg_comp_img_info();
  auto index = find_image_index(comp_info.component_id);
  if (index < 0) {
    ret = FORMAT_ERR::INVALID_SIGNATURE;
    cerr << "Cannot find " << comp_info.component_id << endl;
    goto exit;
  } else {
    uint32_t size = comp_infos[index].compImageInfo.comp_size;
    uint32_t offset = comp_infos[index].compImageInfo.comp_location_offset;
    vector<uint8_t> raw_data(comp_infos[index].compImageInfo.comp_size);

    // read raw image part from PLDM image
    if (lseek(pldm_fd, offset, SEEK_SET) != offset) {
      cerr << __func__ << " lseek failed." << endl;
      ret = FORMAT_ERR::INVALID_FILE;
      goto exit;
    } else {
      if (read(pldm_fd, raw_data.data(), size) < 0) {
        cerr << __func__ << " read failed." << endl;
        ret = FORMAT_ERR::INVALID_FILE;
      goto exit;
    }
    }

    // write to temp file
    if (write(raw_fd, raw_data.data(), raw_data.size()) != (int)size) {
      cerr << __func__ << " write failed." << endl;
      ret = FORMAT_ERR::INVALID_FILE;
      goto exit;
    }
  }

exit:
  close(pldm_fd);
  close(raw_fd);
  return ret;
}

int PldmComponent::del_raw_image() const
{
  string raw_image = "/tmp/"+fru+"_"+component+"_pldm_raw_image.bin";
  return remove(raw_image.c_str());
}

int PldmComponent::pldm_update(const string& image, uint8_t specified_comp) {

  int ret;

  syslog(LOG_CRIT, "FRU %s Component %s upgrade initiated", fru.c_str(), component.c_str());

  ret = oem_pldm_fw_update(bus, eid, (char *)image.c_str(), specified_comp);

  if (ret)
    syslog(LOG_CRIT, "FRU %s Component %s upgrade fail", fru.c_str(), component.c_str());
  else
    syslog(LOG_CRIT, "FRU %s Component %s upgrade completed", fru.c_str(), component.c_str());

  return ret;
}
