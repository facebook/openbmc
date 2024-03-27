#include "selformat.hpp"
#include "selexception.hpp"
#include <openbmc/pal.h>
#include <iostream>
#include <ctime>
#include <time.h>
#include <cstdio>
#include <regex>
#include <fstream>

using namespace std::literals;

std::string SELFormat::left_align(const std::string& instr, size_t num) {
  std::string outstr(instr);
  if (instr.length() < num)
    outstr.append(num - instr.length(), ' ');
  return outstr;
}

std::string SELFormat::get_header() {
  return left_align("FRU#", fru_num_left_align) + " " +
      left_align("FRU_NAME", fru_name_left_align) + " " +
      left_align("TIME_STAMP", time_left_align) + " " +
      left_align("APP_NAME", app_left_align) + " " +
      left_align("MESSAGE", msg_left_align);
}

std::string SELFormat::get_fru_name(uint8_t fru_id) {
  std::array<char, 64> fruname{};
  if (pal_get_fru_name(fru_id, fruname.data())) {
    return "";
  }
  return std::string(fruname.data());
}

std::string SELFormat::get_current_time() {
  time_t rawtime;
  struct tm* ts;
  std::array<char, 80> curtime{};

  ::time(&rawtime);
  ts = localtime(&rawtime);
  strftime(curtime.data(), curtime.size(), "%Y %b %d %H:%M:%S", ts);
  return std::string(curtime.data());
}

void SELFormat::set_clear(uint8_t fru) {
  std::string log;
  std::string name;
  if (fru == FRU_ALL) {
    name = "all";
  } else if (fru == FRU_SYS) {
    name = "sys";
  } else {
    name = "FRU: "s + std::to_string(fru);
  }
  log = get_current_time() + " log-util: User cleared " + name + " logs";
  set_raw(std::move(log));
}

// needs to set the timestamp specially
void SELFormat::set_clear_timestamp(uint8_t fru, const std::string& start_time, const std::string& end_time) {
    std::string log;
    std::string name;
    if (fru == FRU_ALL) {
        name = "all";
    } else if (fru == FRU_SYS) {
        name = "sys";
    } else {
        name = "FRU: "s + std::to_string(fru);
    }
    log = get_current_time() + " log-util: User cleared " + name + " logs from " + start_time + " to " + end_time;
    set_raw(std::move(log));
}

void SELFormat::set_raw(std::string&& line) {
  self_log_ = false;
  bare_ = true;
  raw_.assign(line);
  if (raw_.find("log-util") != std::string::npos) {
    self_log_ = true;
    if (raw_.find("all logs") != std::string::npos) {
      fru_num_ = FRU_ALL;
    } else if (raw_.find("sys logs") != std::string::npos) {
      fru_num_ = FRU_SYS;
    }
  } else if (raw_.find(".crit") == std::string::npos) {
    throw SELParserError("Invalid log: " + raw_);
  } else {
    fru_num_ = default_fru_num_;
  }
  static const std::regex find_fru_re(R"(FRU: (\d+))");
  std::smatch find_fru_match;
  if (regex_search(line, find_fru_match, find_fru_re)) {
    fru_num_ = std::stoi(find_fru_match[1]);
  }
  if (fru_num_ == FRU_ALL) {
    fru_ = "all";
  } else if (fru_num_ == FRU_SYS) {
    fru_ = "sys";
    // Do not leak internal choice of magic FRU_SYS.
    fru_num_ = FRU_ALL;
  } else {
    fru_ = get_fru_name(fru_num_);
  }
  static const std::regex log_match(log_fmt.data());
  static const std::regex log_match_legacy(log_fmt_legacy.data());
  static const std::regex log_match_clr(log_fmt_clr.data());

  std::smatch sm;
  if (bool year_fmt = false;
      (year_fmt = std::regex_search(line, sm, log_match)) ||
      std::regex_search(line, sm, log_match_legacy)) {
    std::array<char, 256> curtime;
    struct tm ts;

    if (!year_fmt) {
      strptime(sm[1].str().c_str(), "%b %d %H:%M:%S", &ts);
      strftime(curtime.data(), curtime.size(), "%m-%d %H:%M:%S", &ts);
    } else {
      strptime(sm[1].str().c_str(), "%Y %b %d %H:%M:%S", &ts);
      strftime(curtime.data(), curtime.size(), "%Y-%m-%d %H:%M:%S", &ts);
    }
    time_.assign(std::string(curtime.data()));
    hostname_ = sm[2];
    version_ = sm[3];
    app_ = sm[4];
    msg_ = sm[5];
    bare_ = false;
  } else {
    if (self_log_ && std::regex_search(line, sm, log_match_clr)) {
      std::array<char, 80> curtime;
      struct tm ts;
      strptime(sm[0].str().c_str(), "%Y %b %d %H:%M:%S", &ts);
      strftime(curtime.data(), curtime.size(), "%Y-%m-%d %H:%M:%S", &ts);
      time_.assign(std::string(curtime.data()));
      hostname_ = "";
      version_ = "";
      app_ = sm[1];
      msg_ = sm[2];
    }
  }
}

bool SELFormat::fits_time_range(const std::string& start_time, const std::string& end_time) {
  time_t time_s, time_e, time_c;
  std::smatch sm;

  // construct the regexes
  static const std::regex time_match(time_fmt.data());
  static const std::regex time_match_legacy(time_fmt_legacy.data());

  // not expecting both of these strings to be in the same format just in case
  if (std::regex_search(start_time, sm, time_match)) {
    std::tm ts{};
    strptime(sm[0].str().c_str(), "%Y-%m-%d %H:%M:%S", &ts);
    time_s = std::mktime(&ts);
  } else if (std::regex_search(start_time, sm, time_match_legacy)) {
    std::tm ts{};
    strptime(sm[0].str().c_str(), "%m-%d %H:%M:%S", &ts);
    time_s = std::mktime(&ts);
  } else {
    return false;
  }

  if (std::regex_search(end_time, sm, time_match)) {
    std::tm ts{};
    strptime(sm[0].str().c_str(), "%Y-%m-%d %H:%M:%S", &ts);
    time_e = std::mktime(&ts);
  } else if (std::regex_search(end_time, sm, time_match_legacy)) {
    std::tm ts{};
    strptime(sm[0].str().c_str(), "%m-%d %H:%M:%S", &ts);
    time_e = std::mktime(&ts);
  } else {
    return false;
  }

  // time_ is stored in a different format than displayed from what I can tell
  if (std::regex_search(time_, sm, time_match)) {
    std::tm ts{};
    strptime(sm[0].str().c_str(), "%Y-%m-%d %H:%M:%S", &ts);
    time_c = std::mktime(&ts);
  } else if (std::regex_search(time_, sm, time_match_legacy)) {
    std::tm ts{};
    strptime(sm[0].str().c_str(), "%m-%d %H:%M:%S", &ts);
    time_c = std::mktime(&ts);
  } else {
    return false;
  }

  // check if it is too old
  if (difftime(time_s, time_c) > 0) {
    return false;
  }
  if (difftime(time_c, time_e) > 0) {
    return false;
  }
  return true;
}

std::string SELFormat::str() const {
  if (bare_)
    return raw_;
  return left_align(std::to_string(fru_num_), fru_num_left_align) + " " +
      left_align(fru_, fru_name_left_align) + " " +
      left_align(time_, time_left_align) + " " +
      left_align(app_, app_left_align) + " " + msg_;
}

void SELFormat::json(nlohmann::json& j) const {
  j["FRU_NAME"] = fru_;
  j["FRU#"] = std::to_string(fru_num_);
  j["TIME_STAMP"] = time_;
  j["APP_NAME"] = app_;
  j["MESSAGE"] = msg_;
}

void to_json(nlohmann::json& j, const SELFormat& sel) {
  sel.json(j);
}

std::istream& operator>>(std::istream& is, SELFormat& s) {
  std::string line;
  std::istream& ret = getline(is, line);
  if (ret.fail())
    return is;
  line.erase(std::remove(line.begin(), line.end(), '\0'), line.end());
  s.set_raw(std::move(line));
  return ret;
}

std::ostream& operator<<(std::ostream& os, const SELFormat& s) {
  os << s.str() << '\n';
  return os;
}

std::string get_output(const std::string& cmd) {
  char buffer[128];
  std::string output;
  auto pipe_close = [](auto fd) { (void)pclose(fd); };
  std::unique_ptr<FILE, decltype(pipe_close)> pipe(popen(cmd.c_str(), "r"), pipe_close);
  if (!pipe) {
    std::cerr << "Failed to open a pipe" << std::endl;
    return "Error";
  }
  while (fgets(buffer, 128, pipe.get()) != NULL) {
    output += buffer;
  }
  return output;
}
