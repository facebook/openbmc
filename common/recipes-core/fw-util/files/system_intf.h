#ifndef _SYSTEM_INTF_H_
#define _SYSTEM_INTF_H_

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
    System(std::ostream &out, std::ostream &err): output(out), error(err) {}

    virtual int runcmd(const std::string &cmd);
    virtual int vboot_support_status();
    virtual bool get_mtd_name(std::string name, std::string &dev);
    bool get_mtd_name(std::string name) {
      std::string unused;
      return get_mtd_name(name, unused);
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

};

#endif
