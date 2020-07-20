#pragma once
#include "rsyslogd.hpp"
#include "selstream.hpp"

class LogUtil {
  std::vector<std::string> logfile_list_{"/mnt/data/logfile.0",
                                         "/mnt/data/logfile"};

 public:
  LogUtil() {}
  virtual ~LogUtil() {}
  virtual std::unique_ptr<SELStream> make_stream(OutputFormat fmt) {
    return std::make_unique<SELStream>(fmt);
  }
  virtual std::unique_ptr<rsyslogd> make_rsyslogd() {
    return std::make_unique<rsyslogd>();
  }
  virtual const std::vector<std::string>& logfile_list() {
    return logfile_list_;
  }
  void print(const fru_set& frus, bool opt_json, std::ostream& os = std::cout);
  void clear(const fru_set& frus);
};
