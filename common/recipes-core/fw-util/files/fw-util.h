#ifndef _FW_UTIL_H_
#define _FW_UTIL_H_
#include <string>
#include <iostream>
#include <algorithm>
#include <map>
#include <atomic>
#include "system_intf.h"

#define FW_STATUS_SUCCESS        0
#define FW_STATUS_FAILURE       -1
#define FW_STATUS_NOT_SUPPORTED -2

class Component {
  protected:
    std::string _fru;
    std::string _component;
    System sys;
    bool update_initiated;
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

    virtual void set_update_ongoing(int timeout) {
      if (timeout > 0)
        update_initiated = true;
      sys.set_update_ongoing(sys.get_fru_id(_fru), timeout);
    }
    virtual bool is_update_ongoing() {
      return sys.is_update_ongoing(sys.get_fru_id(_fru));
    }
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
    int fupdate(std::string image);
    int dump(std::string image);
    int print_version();

    void set_update_ongoing(int timeout);
    bool is_update_ongoing();
};

#endif
