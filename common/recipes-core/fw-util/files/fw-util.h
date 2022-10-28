#ifndef _FW_UTIL_H_
#define _FW_UTIL_H_
#include <string>
#include <iostream>
#include <algorithm>
#include <map>
#include <atomic>
#include <regex>
#include "system_intf.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

#define FW_STATUS_SUCCESS        0
#define FW_STATUS_FAILURE       -1
#define FW_STATUS_NOT_SUPPORTED -2

// If we are comparing two strings which end in a numeric, example "device10" and "device4",
// then compare the integer parts of the strings rather than a pure lexicographical comparison.
// tl;dr return device4 < device10 instead of the other way around.
class partialLexCompare {
  std::regex rx;
  // Split string into numeric and string part.
  void split_string(const std::string &in, std::string &out_s, int &out_i) const {
    std::smatch sm;
    if (std::regex_match(in, sm, rx)) {
      out_s.assign(sm[1]);
      out_i = std::stoi(sm[2]);
      return;
    }
    out_s.assign(in);
    out_i = -1;
  }
  public:
    partialLexCompare() : rx(R"((.+)(\d+)$)") {}
    bool operator()(const std::string& a, const std::string& b) const {
      std::string aa, bb;
      int ai, bi;
      split_string(a, aa, ai);
      split_string(b, bb, bi);
      if (aa == bb && ai >= 0 && bi >= 0) {
        return ai < bi;
      }
      // Ensure that we are doing a lexicographical comparision against the same string
      // no matter what tuple are being picked to compare.
      // This ensures that when we sorting "bmc", "device4", "device10", we are not using
      // "device" when comparing "device4" and "device10" while picking "device10" when
      // comparing "device10" and "bmc" which can mess sorting ordering due to inconsistent
      // comparision.
      return aa < bb;
    }
};

class Component {
  protected:
    std::string _fru;
    std::string _component;
    System _sys;
    bool update_initiated;
  public:
    static std::map<std::string, std::map<std::string, Component *, partialLexCompare>, partialLexCompare> *fru_list;
    static Component *find_component(std::string fru, std::string comp);
    static void fru_list_setup() {
      if (!fru_list) {
        fru_list = new std::map<std::string, std::map<std::string, Component *, partialLexCompare>, partialLexCompare>();
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
    virtual int update(int fd, bool force) { return FW_STATUS_NOT_SUPPORTED; }
    virtual int fupdate(std::string image) { return FW_STATUS_NOT_SUPPORTED; }
    virtual int update_finish(void) { return FW_STATUS_NOT_SUPPORTED; }
    virtual int dump(std::string image) { return FW_STATUS_NOT_SUPPORTED; }
    virtual int print_version() { return FW_STATUS_NOT_SUPPORTED; }
    virtual int get_version(json &j) { return FW_STATUS_NOT_SUPPORTED; }
    virtual System& sys() {
      return _sys;
    }
    virtual void set_update_ongoing(int timeout) {
      if (timeout > 0)
        update_initiated = true;
      sys().set_update_ongoing(sys().get_fru_id(_fru), timeout);
    }
    virtual bool is_update_ongoing() {
      return sys().is_update_ongoing(sys().get_fru_id(_fru));
    }
    virtual bool is_sled_cycle_initiated() {
      return sys().is_sled_cycle_initiated();
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
