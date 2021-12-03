#include "regmap.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>

#if (__GNUC__ < 8)
#include <experimental/filesystem>
namespace std {
namespace filesystem = experimental::filesystem;
}
#else
#include <filesystem>
#endif

using nlohmann::json;

static void stream_hex(std::ostream& os, size_t num, size_t ndigits) {
  os << std::hex << std::setfill('0') << std::setw(ndigits) << std::right
     << num;
}

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

int32_t RegisterValue::to_integer(const std::vector<uint16_t>& value) {
  // TODO We currently do not need more than 32bit values as per
  // our current/planned regmaps. If such a value should show up in the
  // future, then we might need to return std::variant<int32_t,int64_t>.
  if (value.size() > 2)
    throw std::out_of_range("Value does not fit as decimal");
  // Everything in modbus is Big-endian. So when we have a list
  // of registers forming a larger value; For example,
  // a 32bit value would be 2 16bit regs.
  // Then the first register would be the upper nibble of the
  // resulting 32bit value.
  return std::accumulate(
      value.begin(), value.end(), 0, [](int32_t ac, uint16_t v) {
        return (ac << 16) + v;
      });
}

std::string RegisterValue::to_string(const std::vector<uint16_t>& value) {
  std::stringstream os;
  // When displaying as a hexstring, the choice is made to make it
  // readable, so 0x1234 is printed as "1234". Hence the reason we
  // are printing in the order of MSB:LSB.
  for (auto& d : value) {
    uint8_t l = d & 0xff, h = (d >> 8) & 0xff;
    stream_hex(os, h, 2);
    stream_hex(os, l, 2);
  }
  return os.str();
}

std::string RegisterValue::format() const {
  std::stringstream os;
  switch (desc.format) {
    case RegisterFormatType::ASCII: {
      // String is stored normally H L H L, so a we
      // need reswap the bytes in each nibble.
      for (auto& reg : value) {
        char ch = reg >> 8;
        char cl = reg & 0xff;
        if (ch == '\0')
          break;
        os << ch;
        if (cl == '\0')
          break;
        os << cl;
      }
      break;
    }
    case RegisterFormatType::DECIMAL: {
      os << std::dec << to_integer(value);
      break;
    }
    case RegisterFormatType::FIXED_POINT: {
      int32_t ivalue = to_integer(value);
      // Y = X / 2^N
      os << std::setprecision(desc.precision)
         << (float(ivalue) / float(1 << desc.precision));
      break;
    }
    case RegisterFormatType::TABLE: {
      // We could technically be clever and pack this as a
      // JSON object. But considering this is designed for
      // human consumption only, we can make it pretty
      // (and backwards compatible with V1's output).
      uint32_t ivalue = (uint32_t)to_integer(value);
      for (const auto& [pos, n] : desc.table) {
        bool bit = (ivalue & (1 << pos)) != 0;
        if (bit)
          os << "*[1] ";
        else
          os << " [0] ";
        os << n << '\n';
      }
      break;
    }
    case RegisterFormatType::HEX:
    default: {
      os << to_string(value);
      break;
    }
  }
  return os.str();
}

void to_json(json& j, const RegisterValue& m) {
  j["time"] = m.timestamp;
  j["data"] = m.to_string(m.value);
}

std::string RegisterValueStore::format() const {
  std::stringstream ss;

  // Format we are going for.
  // "  <0x0000> MFG_MODEL                        :700-014671-0000  "
  ss << "  <0x";
  stream_hex(ss, desc.begin, 4);
  ss << "> " << std::setfill(' ') << std::setw(32) << std::left << desc.name
     << " :";
  for (const auto& v : history) {
    if (v) {
      ss << v.format();
      if (desc.format != RegisterFormatType::TABLE)
        ss << ' ';
    }
  }
  return ss.str();
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
