#include "plat_nic_ext.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <syslog.h>

int PlatformNicExtComponent::fupdate(const std::string& image)
{
  std::string cmd = "/usr/local/bin/ncsi-util";
  syslog(LOG_CRIT, "Component %s upgrade initiated\n", _component.c_str() );

  // Pass channel to ncsi-util.
  cmd += " -c " + std::to_string(get_channel());
  // Double quote the image to support paths with space.
  cmd += " -p \"" + image + "\"";
  // Add force update flag.
  cmd += " -f";
  int ret = sys().runcmd(cmd);
  if(ret)
    syslog(LOG_CRIT, "Component %s upgrade failed\n", _component.c_str() );
  else
    syslog(LOG_CRIT, "Component %s upgrade completed\n", _component.c_str() );

  return ret;
}
