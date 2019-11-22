#ifndef _FW_UTIL_H_
#define _FW_UTIL_H_
#include <string>
#include <iostream>
#include <algorithm>
#include <map>

#define FW_STATUS_SUCCESS        0
#define FW_STATUS_FAILURE       -1
#define FW_STATUS_NOT_SUPPORTED -2

class Component {
  protected:
    std::string _fru;
    std::string _board;
    std::string _component;

  public:
    static std::map<std::string, std::map<std::string, std::map<std::string, Component *>>> *fru_list;
    static bool is_fru_list_valid();
    static bool is_fru_valid(std::string fru);
    static bool is_board_valid(std::string fru, std::string board);
    static bool is_component_valid(std::string fru, std::string board, std::string comp);
    static Component *get_component(std::string fru, std::string board, std::string comp);
    static void fru_list_setup() {
      if ( !fru_list ) {
        fru_list = new std::map<std::string, std::map<std::string, std::map<std::string, Component *>>> ();
      }
    }

    // Constructor for a real component.
    Component(std::string fru, std::string board, std::string component);
    virtual ~Component();

    std::string &fru(void) { return _fru; }
    std::string &board(void) { return _board; }
    std::string &component(void) { return _component; }
    virtual int update(std::string image) { return FW_STATUS_NOT_SUPPORTED; }
    virtual int fupdate(std::string image) { return FW_STATUS_NOT_SUPPORTED; }
    virtual int dump(std::string image) { return FW_STATUS_NOT_SUPPORTED; }
    virtual int print_version() { return FW_STATUS_NOT_SUPPORTED; }

};

#if 0
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
    virtual int fupdate(std::string image) { return FW_STATUS_NOT_SUPPORTED; }
    virtual int dump(std::string image) { return FW_STATUS_NOT_SUPPORTED; }
    virtual int print_version() { return FW_STATUS_NOT_SUPPORTED; }
};
#endif

#if 0
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
    int fupdate(std::string image);
    int dump(std::string image);
    int print_version();
};
#endif

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
    virtual std::string lock_file(std::string &name);
};

#endif
