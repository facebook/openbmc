#ifndef _FW_UTIL_H_
#define _FW_UTIL_H_
#include <string>
#include <iostream>
#include <map>

#define FW_STATUS_SUCCESS        0
#define FW_STATUS_FAILURE       -1
#define FW_STATUS_NOT_SUPPORTED -2

#define MAX_COMPONENTS  32
#define MAX_STRING_SIZE 32

class Component;

struct ComponentList {
  char fru[MAX_STRING_SIZE];
  char comp[MAX_STRING_SIZE];
  Component *obj;
};

struct ComponentAliasList {
  char fru[MAX_STRING_SIZE];
  char comp[MAX_STRING_SIZE];
  char real_fru[MAX_STRING_SIZE];
  char real_comp[MAX_STRING_SIZE];
};

class Component {
  protected:
    std::string _fru;
    std::string _component;
    // Since order of object initialization is not guaranteed, 
    // use a simple statically initialized data structure instead
    // of a more robust data structure.
    static ComponentList registered_components[MAX_COMPONENTS];
    static ComponentAliasList registered_aliases[MAX_COMPONENTS];

  public:
    static std::map<std::string, std::map<std::string, Component *>> fru_list;

    // Constructor for a real component.
    Component(std::string fru, std::string component);

    // Constructor for an alias.
    Component(std::string fru, std::string component, 
        std::string real_fru, std::string real_component);

    static void populateFruList();

    std::string &component(void) { return _component; }
    std::string &fru(void) { return _fru; }

    virtual int update(std::string image) { return FW_STATUS_NOT_SUPPORTED; }
    virtual int dump(std::string image) { return FW_STATUS_NOT_SUPPORTED; }
    virtual int print_version() { return FW_STATUS_NOT_SUPPORTED; }
};

#endif
