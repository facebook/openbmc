#pragma once
#include <iostream>
#include <memory>
#include "selformat.hpp"

enum OutputFormat { FORMAT_PRINT, FORMAT_RAW, FORMAT_JSON };

class SELStream {
  nlohmann::json sel_list_ = nlohmann::json::array();
  OutputFormat fmt_;

 public:
  SELStream(OutputFormat fmt) : fmt_(fmt) {}
  virtual ~SELStream() {}
  void flush(std::ostream& os);
  virtual std::unique_ptr<SELFormat> make_sel(uint8_t default_fru);
  void start(std::istream& is, std::ostream& os, const fru_set& filter_fru);
  void log_cleared(std::ostream& os, const fru_set& affected_frus);
};
