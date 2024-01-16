#ifndef _EXTLIB_H_
#define _EXTLIB_H_
#include "fw-util.h"

class ExtlibComponent : public Component {
  void *lib;
  std::string info;
  int (*update_fn)(const char *, const char *);
  int (*get_version_fn)(const char *, char *, size_t n);
  public:
    ExtlibComponent(std::string fru, std::string component, 
        std::string libpath, std::string update_func, std::string vers_func,
        std::string info);
    int update(std::string image) override;
    int get_version(json&) override;
};

#endif
