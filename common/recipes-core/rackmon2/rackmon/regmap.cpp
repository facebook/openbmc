#include "regmap.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>

#if (__GNUC__ < 8)
#include <experimental/filesystem>
namespace std {
namespace filesystem = experimental::filesystem;
}
#else
#include <filesystem>
#endif

using nlohmann::json;

bool addr_range::operator<(const addr_range& rhs) const {
  return range.second < rhs.range.first;
}

bool addr_range::contains(uint8_t addr) const {
  return addr >= range.first && addr <= range.second;
}

void from_json(const json& j, addr_range& a) {
  a.range = j;
}
void to_json(json& j, const addr_range& a) {
  j = a.range;
}

NLOHMANN_JSON_SERIALIZE_ENUM(
    RegisterFormatType,
    {
        {RegisterFormatType::HEX, "hex"},
        {RegisterFormatType::ASCII, "ascii"},
        {RegisterFormatType::DECIMAL, "decimal"},
        {RegisterFormatType::FIXED_POINT, "fixedpoint"},
        {RegisterFormatType::TABLE, "table"},
    })

void from_json(const json& j, RegisterDescriptor& i) {
  j.at("begin").get_to(i.begin);
  j.at("length").get_to(i.length);
  j.at("name").get_to(i.name);
  i.keep = j.value("keep", 1);
  i.changes_only = j.value("changes_only", false);
  i.format = j.value("format", RegisterFormatType::HEX);
  if (i.format == RegisterFormatType::FIXED_POINT) {
    j.at("precision").get_to(i.precision);
  } else if (i.format == RegisterFormatType::TABLE) {
    j.at("table").get_to(i.table);
  }
}
void to_json(json& j, const RegisterDescriptor& i) {
  j["begin"] = i.begin;
  j["length"] = i.length;
  j["name"] = i.name;
  j["keep"] = i.keep;
  j["changes_only"] = i.changes_only;
  j["format"] = i.format;
  if (i.format == RegisterFormatType::FIXED_POINT) {
    j["precision"] = i.precision;
  } else if (i.format == RegisterFormatType::TABLE) {
    j["table"] = i.table;
  }
}

void to_json(json& j, const RegisterValue& m) {
  j["time"] = m.timestamp;
  std::stringstream ss;
  for (auto& d : m.value) {
    uint8_t l = d & 0xff, h = (d >> 8) & 0xff;
    ss << std::setfill('0') << std::setw(2) << std::right << std::hex << int(l);
    ss << std::setfill('0') << std::setw(2) << std::right << std::hex << int(h);
  }
  j["data"] = ss.str();
}

void to_json(json& j, const RegisterValueStore& m) {
  j["begin"] = m.reg_addr;
  j["readings"] = m.history;
}

void from_json(const json& j, RegisterMap& m) {
  j.at("address_range").get_to(m.applicable_addresses);
  j.at("probe_register").get_to(m.probe_register);
  j.at("name").get_to(m.name);
  j.at("preferred_baudrate").get_to(m.preferred_baudrate);
  j.at("default_baudrate").get_to(m.default_baudrate);
  std::vector<RegisterDescriptor> tmp;
  j.at("registers").get_to(tmp);
  for (auto& i : tmp) {
    m.register_descriptors[i.begin] = i;
  }
}
void to_json(json& j, const RegisterMap& m) {
  j["address_range"] = m.applicable_addresses;
  j["probe_register"] = m.probe_register;
  j["name"] = m.name;
  j["preferred_baudrate"] = m.preferred_baudrate;
  j["default_baudrate"] = m.preferred_baudrate;
  j["registers"] = {};
  std::transform(
      m.register_descriptors.begin(),
      m.register_descriptors.end(),
      std::back_inserter(j["registers"]),
      [](const auto& kv) { return kv.second; });
}

RegisterMap& RegisterMapDatabase::at(uint16_t addr) {
  RegisterMap& ret = regmaps.at(addr_range(addr, addr));
  if (!ret.applicable_addresses.contains(addr)) {
    throw std::out_of_range("not found: " + std::to_string(addr));
  }
  return ret;
}

void RegisterMapDatabase::load(const std::string& dir_s) {
  std::filesystem::path dir(dir_s);
  for (auto const& dir_entry : std::filesystem::directory_iterator{dir}) {
    std::string path = dir_entry.path().string();
    std::ifstream ifs(path);
    json j;
    ifs >> j;
    addr_range range;
    j.at("address_range").get_to(range);
    regmaps[range] = j;
  }
}

void RegisterMapDatabase::print(std::ostream& os) {
  json j = {};
  std::transform(
      regmaps.begin(),
      regmaps.end(),
      std::back_inserter(j),
      [](const auto& kv) { return kv.second; });
  os << j.dump(4);
}
