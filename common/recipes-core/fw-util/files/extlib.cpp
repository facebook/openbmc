#include <iostream>
#include <dlfcn.h>
#include "extlib.h"

using namespace std;

static int update_unsupported(const char * /*image*/, const char * /*info*/)
{
  return FW_STATUS_NOT_SUPPORTED;
}
static int print_version_unsupported(const char * /*info*/)
{
  return FW_STATUS_NOT_SUPPORTED;
}

ExtlibComponent::ExtlibComponent(string fru, string component,
                            string libpath, string update_func,
                            string vers_func, string _info) :
  Component(fru, component), info(_info),
  update_fn(update_unsupported), print_version_fn(print_version_unsupported)
{
  void *ptr;
  lib = dlopen(libpath.c_str(), RTLD_NOW);
  if (!lib) {
    cerr << "DLOPEN of " << libpath << " failed" << endl;
    return;
  }
  if (update_func != "") {
    ptr = dlsym(lib, update_func.c_str());
    if (!ptr) {
      cerr << "Getting symbol for " << update_func << " failed" << endl;
    } else {
      update_fn = (int(*)(const char *image, const char *info))ptr;
    }
  }
  if (vers_func != "") {
    ptr = dlsym(lib, vers_func.c_str());
    if (!ptr) {
      cerr << "Getting symbol for " << vers_func << " failed" << endl;
    } else {
      print_version_fn = (int(*)(const char *info))ptr;
    }
  }
}

int ExtlibComponent::update(string image)
{
  return update_fn(image.c_str(), info.c_str());
}

int ExtlibComponent::print_version()
{
  return print_version_fn(info.c_str());
}

