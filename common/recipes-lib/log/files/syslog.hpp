#ifndef _SYSLOG_HPP_
#define _SYSLOG_HPP_
#include <syslog.h>
#include <iostream>

namespace openbmc {

class SysLogBuf : public std::basic_streambuf<char, std::char_traits<char>> {
 public:
  explicit SysLogBuf(int prio) : priority_(prio) {}

 protected:
  int sync() {
    if (buffer_.length()) {
      syslog(priority_, "%s", buffer_.c_str());
      buffer_.erase();
    }
    return 0;
  }
  int overflow(int c) {
    if (c != EOF) {
      buffer_ += static_cast<char>(c);
    } else {
      sync();
    }
    return c;
  }

 private:
  std::string buffer_;
  int priority_;
};

class Logger : public std::ostream {
  SysLogBuf logbuf;

 public:
  explicit Logger(int prio) : std::ostream(), logbuf(prio) {
    rdbuf(&logbuf);
  }
  Logger() : std::ostream(), logbuf(LOG_INFO) {
    rdbuf(&logbuf);
  }
};

static thread_local Logger syslog_crit(LOG_CRIT);
static thread_local Logger syslog_error(LOG_ERR);
static thread_local Logger syslog_warn(LOG_WARNING);
static thread_local Logger syslog_notice(LOG_NOTICE);
static thread_local Logger syslog_debug(LOG_DEBUG);

}; // namespace openbmc

#endif
