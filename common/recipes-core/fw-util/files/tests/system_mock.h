#ifndef _SYSTEM_MOCK_H_
#define _SYSTEM_MOCK_H_
#include "system_intf.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>

class SystemMock : public System {
  public:
  SystemMock() : System() {}
  SystemMock(std::ostream &out, std::ostream &err): System(out, err) {}

  MOCK_METHOD1(runcmd, int(const std::string &cmd));
  MOCK_METHOD0(vboot_hardware_enforce, bool());
  MOCK_METHOD2(get_mtd_name, bool(const std::string name, std::string &dev));
  MOCK_METHOD0(name, std::string());
  MOCK_METHOD0(version, std::string());
  MOCK_METHOD0(partition_conf, std::string&());
  MOCK_METHOD1(get_fru_id, uint8_t(std::string &name));
  MOCK_METHOD2(set_update_ongoing, void(uint8_t fruid, int timeo));
  MOCK_METHOD1(lock_file, std::string(std::string name));
  int copy_file(std::string cmd) {
    size_t pos = cmd.find("flashcp -v ");
    if (pos == std::string::npos) {
      return -1;
    }
    std::string params = cmd.substr(pos + strlen("flashcp -v "));
    pos = params.find(" ");
    std::string filesrc = params.substr(0, pos);
    std::string filedst = params.substr(pos + 1);
    std::ofstream dst(filedst);
    dst << file_contents(filesrc);
    dst.close();
    return 0;
  }
  static std::string file_contents(std::string name)
  {
    std::ifstream in(name);
    std::string ret;
    in >> ret;
    in.close();
    return ret;
  }
};

#endif
