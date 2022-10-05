#pragma once
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <string_view>

using fru_set = std::set<uint8_t>;

class SELFormat {
 public:
  static constexpr uint8_t FRU_SYS = 0xFE;
  static constexpr uint8_t FRU_ALL = 0x00;

  explicit SELFormat(uint8_t default_fru_id)
      : default_fru_num_(default_fru_id), fru_num_(default_fru_id) {}
  virtual ~SELFormat() {}

  static std::string left_align(const std::string& instr, size_t num);
  static std::string get_header();

  virtual std::string get_fru_name(uint8_t fru_id);
  virtual std::string get_current_time();

  // Getters.
  bool is_bare() const {
    return bare_;
  }
  bool is_self() const {
    return self_log_;
  }
  uint8_t fru_id() const {
    return fru_num_;
  }
  const std::string& fru_name() const {
    return fru_;
  }
  const std::string& time_stamp() const {
    return time_;
  }
  const std::string& hostname() const {
    return hostname_;
  }
  const std::string& version() const {
    return version_;
  }
  const std::string& app() const {
    return app_;
  }
  const std::string& msg() const {
    return msg_;
  }
  std::string raw() const {
    return raw_;
  }
  std::string str() const;
  void json(nlohmann::json& j) const;

  // Set the raw. This also parses the log.
  void set_raw(std::string&& line);

  // Set a lot indicating of a clear with the current timestamp.
  void set_clear(uint8_t fru);
  void set_clear_timestamp(uint8_t fru, const std::string& start_time, const std::string& end_time);
  void force_bare() {
    bare_ = true;
  }
  bool fru_matches(const fru_set& frus) {
    if (frus.count(FRU_SYS) && fru_ == "sys")
      return true;
    return (frus.count(FRU_ALL) || frus.count(fru_num_));
  }

  bool fits_time_range(const std::string& start_time, const std::string& end_time);

 private:
  bool bare_ = true;
  bool self_log_ = false;
  std::string fru_ = "";
  std::string time_ = "";
  std::string app_ = "";
  std::string msg_ = "";
  std::string hostname_ = "";
  std::string version_ = "";
  int default_fru_num_;
  int fru_num_ = -1;
  std::string raw_ = "";

  static constexpr size_t fru_num_left_align = 4;
  static constexpr size_t fru_name_left_align = 8;
  static constexpr size_t time_left_align = 22;
  static constexpr size_t app_left_align = 16;
  static constexpr size_t msg_left_align = 0;

  // LOG Format:
  // TIME_STAMP HOSTNAME SEVERITY VERSION: APP: MESSAGE
  // Example:
  // MESSAGE: ASSERT: GPIOAA0 - FM_CPU1_SKTOCC_LVT3_N
  // APP:     ipmid
  // VERSION: fbtp-79c9c5e5b7
  // SEVERITY:user.crit
  // TIME_STAMP (Current): 2020 Mar  5 11:21:09
  // TIME_STAMP (Legacy):  Mar  5 11:21:09
  // Note legacy timestamp is missing the year. This was a bug in
  // rsyslogd's configuration and we ended up with the logfile
  // stored in persistent store without a year in the time stamp.
  // This is a hack-workaround to prevent parsing inconsistencies.
  static constexpr std::string_view log_fmt =
      R"(([0-9]{4}\s+\S+\s+\d+\s+\d+:\d+:\d+)\s+(\S+)\s+\S+\s+(\S+):\s+(\S+):\s+(.+)$)";

  static constexpr std::string_view log_fmt_legacy =
      R"((\S+\s+\d+\s+\d+:\d+:\d+)\s+(\S+)\s+\S+\s+(\S+):\s+(\S+):\s+(.+)$)";

  static constexpr std::string_view log_fmt_clr =
      R"(([0-9]{4}\s+\S+\s+\d+\s+\d+:\d+:\d+)\s+(\S+):\s+(.+)$)";

  static constexpr std::string_view time_fmt =
    R"([0-9]{4}-[0-9]{2}-[0-9]{2}\s+[0-9]{2}:[0-9]{2}:[0-9]{2})";

  static constexpr std::string_view time_fmt_legacy =
    R"([0-9]{2}-[0-9]{2}\s+[0-9]{2}:[0-9]{2}:[0-9]{2})";

  static constexpr std::string_view release_fmt =
    R"(OpenBMC Release (\S+))";
};

void to_json(nlohmann::json& j, const SELFormat& sel);
std::istream& operator>>(std::istream& is, SELFormat& s);
std::ostream& operator<<(std::ostream& os, const SELFormat& s);
std::string get_output(const std::string& cmd);
