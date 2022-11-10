#ifndef _SYSTEM_INTF_H_
#define _SYSTEM_INTF_H_
#include <fstream>
#include <iostream>

enum {
  VBOOT_NO_SUPPORT,
  VBOOT_NO_ENFORCE,
  VBOOT_SW_ENFORCE,
  VBOOT_HW_ENFORCE
};

class System {
  public:
    std::ostream &output;
    std::ostream &error;

    System() : output(std::cout), error(std::cerr) {}
    virtual ~System() = default;
    System(std::ostream &out, std::ostream &err): output(out), error(err) {}

    virtual int runcmd(const std::string &cmd);
    virtual int vboot_support_status();
    virtual bool get_mtd_name(std::string name, std::string &dev, size_t& size, size_t& esize);
    virtual bool get_mtd_name(std::string name) {
      std::string unused;
      size_t sz, esz;
      return get_mtd_name(name, unused, sz, esz);
    }
    virtual bool get_mtd_name(std::string name, std::string& dev)
    {
      size_t sz, esz;
      return get_mtd_name(name, dev, sz, esz);
    }

    virtual std::string version();
    std::string name() {
      std::string vers = version();
      std::string mc = vers.substr(0, vers.find("-"));
      std::transform(mc.begin(), mc.end(), mc.begin(), ::tolower);
      return mc;
    }
    virtual std::string& partition_conf();
    virtual uint8_t get_fru_id(std::string &name);
    virtual void set_update_ongoing(uint8_t fru_id, int timeo);
    virtual bool is_update_ongoing(uint8_t fru);
    virtual bool is_sled_cycle_initiated();
    virtual bool is_healthd_running();
    virtual bool is_reboot_ongoing();
    virtual bool is_shutdown_non_executable();
    virtual int wait_shutdown_non_executable(uint8_t timeout_sec);
};

#endif
