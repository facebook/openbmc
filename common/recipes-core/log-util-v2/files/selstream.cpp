#include "selstream.hpp"

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
    const fru_set& filter_fru) {
  uint8_t default_fru_id = filter_fru.count(SELFormat::FRU_SYS) > 0
      ? SELFormat::FRU_SYS
      : SELFormat::FRU_ALL;
  std::unique_ptr<SELFormat> sel = make_sel(default_fru_id);
  while (is >> *sel) {
    if ((fmt_ == FORMAT_RAW || fmt_ == FORMAT_JSON) && sel->is_self()) {
      // RAW is used by clear and we filter out all previous
      // logs injected by this utility.
      // We do not send this as JSON format as well.
      continue;
    }
    bool blacklist = fmt_ == FORMAT_RAW;
    if (!(sel->fru_matches(filter_fru) ^ blacklist))
      continue;
    if (fmt_ == FORMAT_RAW)
      sel->force_bare();
    if (fmt_ == FORMAT_JSON) {
      nlohmann::json j(*sel);
      sel_list_.push_back(j);
    } else {
      os << *sel;
    }
  }
}

void SELStream::log_cleared(std::ostream& os, const fru_set& frus) {
  std::unique_ptr<SELFormat> sel = make_sel(SELFormat::FRU_ALL);
  for (auto& fru : frus) {
    sel->set_clear(fru);
    os << *sel;
  }
}
