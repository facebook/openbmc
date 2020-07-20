#include "log-util.hpp"
#include <fstream>

void LogUtil::print(const fru_set& frus, bool opt_json, std::ostream& os) {
  std::unique_ptr<SELStream> stream =
      make_stream(opt_json ? FORMAT_JSON : FORMAT_PRINT);
  for (auto& logfile : logfile_list()) {
    try {
      auto fd = std::ifstream(logfile);
      if (!fd.is_open()) {
        throw std::runtime_error(logfile + " open failed");
      }
      stream->start(fd, os, frus);
    } catch (std::exception& e) {
      continue;
    }
  }
  stream->flush(os);
}

void LogUtil::clear(const fru_set& frus) {
  std::unique_ptr<SELStream> stream = make_stream(FORMAT_RAW);
  const std::vector<std::string>& llist = logfile_list();
  for (auto& logfile : llist) {
    auto fd = std::ifstream(logfile);
    if (!fd.is_open()) {
      continue;
    }
    std::string nfile = logfile + ".tmp";
    std::ofstream ofs(nfile);
    if (!ofs.is_open()) {
      throw std::runtime_error(nfile + " creation failed");
    }
    stream->start(fd, ofs, frus);
    // If the last logfile, also add the "CLEARED"
    // log line as a breadcrumb
    if (logfile == llist.back()) {
      stream->log_cleared(ofs, frus);
    }
    stream->flush(ofs);
    ofs.close();
    if (rename(nfile.c_str(), logfile.c_str())) {
      throw std::runtime_error(nfile + " renamed as " + logfile + " failed");
    }
  }
  std::unique_ptr<rsyslogd> rd = make_rsyslogd();
  rd->reload();
}
