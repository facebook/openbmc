#ifndef _FW_UTIL_H_
#define _FW_UTIL_H_
#include <string>
#include <iostream>
#include <map>

#define FW_STATUS_SUCCESS        0
#define FW_STATUS_FAILURE       -1
#define FW_STATUS_NOT_SUPPORTED -2

class Component {
  protected:
    std::string _fru;
    std::string _component;

  public:
    static std::map<std::string, std::map<std::string, Component *>> *fru_list;
    static Component *find_component(std::string fru, std::string comp);
    static void fru_list_setup() {
      if (!fru_list) {
        fru_list = new std::map<std::string, std::map<std::string, Component *>>();
      }
    }
    // Constructor for a real component.
    Component(std::string fru, std::string component);
    virtual ~Component();

    std::string &component(void) { return _component; }
    std::string &fru(void) { return _fru; }
    virtual bool is_alias(void) { return false; }
    virtual std::string &alias_component(void) { return _component; }
    virtual std::string &alias_fru(void) { return _fru; }
    virtual int update(std::string image) { return FW_STATUS_NOT_SUPPORTED; }
    virtual int dump(std::string image) { return FW_STATUS_NOT_SUPPORTED; }
    virtual int print_version() { return FW_STATUS_NOT_SUPPORTED; }
};

class AliasComponent : public Component {
  std::string _target_fru;
  std::string _target_comp_name;
  Component *_target_comp;
  bool setup();
  public:
    AliasComponent(std::string fru, std::string comp, std::string t_fru, std::string t_comp);
    bool is_alias(void) { return true; }
    std::string &alias_component(void) { return _target_comp_name; }
    std::string &alias_fru(void) { return _target_fru; }
    int update(std::string image);
    int dump(std::string image);
    int print_version();
};

class System {
  public:
    static int runcmd(const std::string &cmd);
    static bool vboot_hardware_enforce();
    static bool get_mtd_name(const char *name, char *dev);
    static std::string& name();
    static std::string& partition_conf();
    static uint8_t get_fru_id(std::string &name);
    static void set_update_ongoing(uint8_t fru_id, int timeo);
    static std::string& lock_file(std::string &name);
};

#endif
