#include "fw-util.h"
#include <openbmc/pal.h>

using namespace std;

class PAXComponent : public Component {
  uint8_t _paxid;
  std::string _mtd_name;
  public:
    PAXComponent(string fru, string comp, uint8_t paxid, std::string mtd)
      : Component(fru, comp), _paxid(paxid), _mtd_name(mtd) {}
    int print_version();
    int update(string image);
    int fupdate(string image);
};

int PAXComponent::print_version()
{
  int ret;
  char ver[64] = {0};

  cout << "PAX" << (int)_paxid << " IMG Version: ";
  ret = pal_get_pax_version(_paxid, ver);
  if (ret < 0)
    cout << "NA" << endl;
  else
    cout << string(ver) << endl;

  return ret;
}

int PAXComponent::update(string image)
{
  return pal_pax_fw_update(_paxid, image.c_str());
}

int PAXComponent::fupdate(string image)
{
  int ret;
  int mtdno, found = 0;
  char line[128], name[32];
  FILE *fp;

  // TODO:
  // 	Check fwimg before force update
  //
  ret = pal_fw_update_prepare(FRU_MB, _component.c_str());
  if (ret) {
    return -1;
  }

  fp = fopen("/proc/mtd", "r");
  if (fp) {
    while (fgets(line, sizeof(line), fp)) {
      if (sscanf(line, "mtd%d: %*x %*x %s", &mtdno, name) == 2) {
        if (!strcmp(_mtd_name.c_str(), name)) {
          found = 1;
          break;
        }
      }
    }
    fclose(fp);
  }

  if (found) {
    snprintf(line, sizeof(line), "flashcp -v %s /dev/mtd%d", image.c_str(), mtdno);
    ret = system(line);
    if (WIFEXITED(ret) && (WEXITSTATUS(ret) == 0))
      ret = FW_STATUS_SUCCESS;
    else
      ret = FW_STATUS_FAILURE;

  } else {
    ret = FW_STATUS_FAILURE;
  }

  ret = pal_fw_update_finished(FRU_MB, _component.c_str(), ret);

  return ret;
}

PAXComponent pax0("mb", "pax0", 0, "\"switch0\"");
PAXComponent pax1("mb", "pax1", 1, "\"switch0\"");
PAXComponent pax2("mb", "pax2", 2, "\"switch0\"");
PAXComponent pax3("mb", "pax3", 3, "\"switch0\"");
