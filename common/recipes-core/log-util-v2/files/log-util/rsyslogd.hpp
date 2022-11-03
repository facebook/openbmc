#pragma once
#include <string>

class rsyslogd {
 private:
  int pid = -1;

  virtual void runcmd(const std::string& cmd);
  virtual void runcmd(const std::string& cmd, std::string& out);

 public:
  virtual int getpid(void);
  rsyslogd() {}
  virtual ~rsyslogd() {}
  virtual void reload();
};
