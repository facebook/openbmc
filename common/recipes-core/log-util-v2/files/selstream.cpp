#include "selstream.hpp"
#include "selexception.hpp"
#include <iostream>

void SELStream::flush(std::ostream& os) {
  if (fmt_ == FORMAT_JSON) {
    nlohmann::json j;
    j["Logs"] = sel_list_;
    os << j.dump(4) << '\n';
  }
  os.flush();
}

std::unique_ptr<SELFormat> SELStream::make_sel(uint8_t default_fru) {
  return std::make_unique<SELFormat>(default_fru);
}

void SELStream::start(
    std::istream& is,
    std::ostream& os,
    const fru_set& filter_fru,
    const std::string& start_time,
    const std::string& end_time,
    const ParserFlag flag) {
  uint8_t default_fru_id = filter_fru.count(SELFormat::FRU_SYS) > 0
      ? SELFormat::FRU_SYS
      : SELFormat::FRU_ALL;
  std::unique_ptr<SELFormat> sel = make_sel(default_fru_id);
  do {
    try {
      if (!(is >> *sel))
        break;
      if (fmt_ == FORMAT_JSON && sel->is_bare()) {
        // RAW is used by clear and we filter out all previous
        // logs injected by this utility.
        // We do not send this as JSON format as well.
        continue;
      }
      bool blacklist = fmt_ == FORMAT_RAW;
      bool timestamp = !(start_time.empty() || end_time.empty());
      if (timestamp) {
          if (!((sel->fru_matches(filter_fru) && sel->fits_time_range(start_time, end_time)) ^ blacklist)) {
            continue;
          }
      } else {
          if (!(sel->fru_matches(filter_fru) ^ blacklist)) {
            continue;
          }
      }
      if (fmt_ == FORMAT_RAW)
        sel->force_bare();
      if (fmt_ == FORMAT_JSON) {
        nlohmann::json j(*sel);
        sel_list_.push_back(j);
      } else {
        os << *sel;
      }
    } catch (SELException &e) {
      if (flag & PARSE_STOP_ON_ERR) {
        std::cerr << "[ERR] " << e.what() << std::endl;
        break;
      }
    }
  } while (!is.eof());
}

void SELStream::log_cleared(std::ostream& os,
        const fru_set& affected_frus,
        const std::string& start_time,
        const std::string& end_time) {
  std::unique_ptr<SELFormat> sel = make_sel(SELFormat::FRU_ALL);
  bool timestamp = start_time.empty() || end_time.empty();
  for (auto& fru : affected_frus) {
    timestamp ? sel->set_clear(fru) : sel->set_clear_timestamp(fru, start_time, end_time);
    os << *sel;
  }
}

