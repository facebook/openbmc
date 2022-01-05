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
    RegisterValueType,
    {
        {RegisterValueType::HEX, "hex"},
        {RegisterValueType::STRING, "string"},
        {RegisterValueType::INTEGER, "integer"},
        {RegisterValueType::FLOAT, "float"},
        {RegisterValueType::FLAGS, "flags"},
    })

void from_json(const json& j, RegisterDescriptor& i) {
  j.at("begin").get_to(i.begin);
  j.at("length").get_to(i.length);
  j.at("name").get_to(i.name);
  i.keep = j.value("keep", 1);
  i.changes_only = j.value("changes_only", false);
  i.format = j.value("format", RegisterValueType::HEX);
  if (i.format == RegisterValueType::FLOAT) {
    j.at("precision").get_to(i.precision);
  } else if (i.format == RegisterValueType::FLAGS) {
    j.at("flags").get_to(i.flags);
  }
}
void to_json(json& j, const RegisterDescriptor& i) {
  j["begin"] = i.begin;
  j["length"] = i.length;
  j["name"] = i.name;
  j["keep"] = i.keep;
  j["changes_only"] = i.changes_only;
  j["format"] = i.format;
  if (i.format == RegisterValueType::FLOAT) {
    j["precision"] = i.precision;
  } else if (i.format == RegisterValueType::FLAGS) {
    j["flags"] = i.flags;
  }
}

void RegisterValue::make_string(const std::vector<uint16_t>& reg) {
  new (&value.strValue)(std::string);
  // String is stored normally H L H L, so we
  // need reswap the bytes in each nibble.
  for (const auto& r : reg) {
    char ch = r >> 8;
    char cl = r & 0xff;
    if (ch == '\0')
      break;
    value.strValue += ch;
    if (cl == '\0')
      break;
    value.strValue += cl;
  }
}

void RegisterValue::make_hex(const std::vector<uint16_t>& reg) {
  new (&value.hexValue)(std::vector<uint8_t>);
  for (uint16_t v : reg) {
    value.hexValue.push_back(v >> 8);
    value.hexValue.push_back(v & 0xff);
  }
}

void RegisterValue::make_integer(const std::vector<uint16_t>& reg) {
  // TODO We currently do not need more than 32bit values as per
  // our current/planned regmaps. If such a value should show up in the
  // future, then we might need to return std::variant<int32_t,int64_t>.
  if (reg.size() > 2)
    throw std::out_of_range("Register does not fit as an integer");
  // Everything in modbus is Big-endian. So when we have a list
  // of registers forming a larger value; For example,
  // a 32bit value would be 2 16bit regs.
  // Then the first register would be the upper nibble of the
  // resulting 32bit value.
  value.intValue =
      std::accumulate(reg.begin(), reg.end(), 0, [](int32_t ac, uint16_t v) {
        return (ac << 16) + v;
      });
}

void RegisterValue::make_float(
    const std::vector<uint16_t>& reg,
    uint16_t precision) {
  make_integer(reg);
  // Y = X / 2^N
  value.floatValue = float(value.intValue) / float(1 << precision);
}

void RegisterValue::make_flags(
    const std::vector<uint16_t>& reg,
    const RegisterDescriptor::FlagsDescType& flags_desc) {
  make_integer(reg);
  uint32_t bitfield = static_cast<uint32_t>(value.intValue);
  new (&value.flagsValue)(FlagsType);
  for (const auto& [pos, name] : flags_desc) {
    bool bit_val = (bitfield & (1 << pos)) != 0;
    value.flagsValue.push_back(std::make_tuple(bit_val, name));
  }
}

RegisterValue::RegisterValue(
    const std::vector<uint16_t>& reg,
    const RegisterDescriptor& desc,
    uint32_t tstamp)
    : type(desc.format), timestamp(tstamp) {
  switch (desc.format) {
    case RegisterValueType::STRING:
      make_string(reg);
      break;
    case RegisterValueType::INTEGER:
      make_integer(reg);
      break;
    case RegisterValueType::FLOAT:
      make_float(reg, desc.precision);
      break;
    case RegisterValueType::FLAGS:
      make_flags(reg, desc.flags);
      break;
    case RegisterValueType::HEX:
      make_hex(reg);
  }
}

RegisterValue::RegisterValue(const std::vector<uint16_t>& reg)
    : type(RegisterValueType::HEX) {
  make_hex(reg);
}

RegisterValue::RegisterValue(const RegisterValue& other)
    : type(other.type), timestamp(other.timestamp) {
  switch (type) {
    case RegisterValueType::HEX:
      new (&value.hexValue) auto(other.value.hexValue);
      break;
    case RegisterValueType::INTEGER:
      value.intValue = other.value.intValue;
      break;
    case RegisterValueType::FLOAT:
      value.floatValue = other.value.floatValue;
      break;
    case RegisterValueType::STRING:
      new (&value.strValue) auto(other.value.strValue);
      break;
    case RegisterValueType::FLAGS:
      new (&value.flagsValue) auto(other.value.flagsValue);
      break;
  }
}

RegisterValue::RegisterValue(RegisterValue&& other)
    : type(other.type), timestamp(other.timestamp) {
  switch (type) {
    case RegisterValueType::HEX:
      new (&value.hexValue) auto(std::move(other.value.hexValue));
      break;
    case RegisterValueType::INTEGER:
      value.intValue = other.value.intValue;
      break;
    case RegisterValueType::FLOAT:
      value.floatValue = other.value.floatValue;
      break;
    case RegisterValueType::STRING:
      new (&value.strValue) auto(std::move(other.value.strValue));
      break;
    case RegisterValueType::FLAGS:
      new (&value.flagsValue) auto(std::move(other.value.flagsValue));
      break;
  }
}

RegisterValue::~RegisterValue() {
  switch (type) {
    case RegisterValueType::HEX:
      value.hexValue.~vector<uint8_t>();
      break;
    case RegisterValueType::STRING:
      value.strValue.~basic_string();
      break;
    case RegisterValueType::FLAGS:
      value.flagsValue.~FlagsType();
      break;
    case RegisterValueType::INTEGER:
      [[fallthrough]];
    case RegisterValueType::FLOAT:
      // Do nothing
      break;
  }
}

RegisterValue::operator std::string() {
  std::string ret = "";
  std::stringstream os;
  switch (type) {
    case RegisterValueType::STRING:
      os << value.strValue;
      break;
    case RegisterValueType::INTEGER:
      os << value.intValue;
      break;
    case RegisterValueType::FLOAT:
      os << std::fixed << std::setprecision(2) << value.floatValue;
      break;
    case RegisterValueType::FLAGS:
      // We could technically be clever and pack this as a
      // JSON object. But considering this is designed for
      // human consumption only, we can make it pretty
      // (and backwards compatible with V1's output).
      for (auto& [bitval, name] : value.flagsValue) {
        if (bitval)
          os << "\n*[1] ";
        else
          os << "\n [0] ";
        os << name;
      }
      break;
    case RegisterValueType::HEX:
      for (uint8_t byte : value.hexValue) {
        os << std::hex << std::setw(2) << std::setfill('0') << int(byte);
      }
      break;
    default:
      throw std::runtime_error("Unknown format type: " + std::to_string(type));
  }
  return os.str();
}

void to_json(json& j, const RegisterValue& m) {
  j["type"] = m.type;
  j["time"] = m.timestamp;
  switch (m.type) {
    case RegisterValueType::HEX:
      j["value"] = m.value.hexValue;
      break;
    case RegisterValueType::STRING:
      j["value"] = m.value.strValue;
      break;
    case RegisterValueType::INTEGER:
      j["value"] = m.value.intValue;
      break;
    case RegisterValueType::FLOAT:
      j["value"] = m.value.floatValue;
      break;
    case RegisterValueType::FLAGS:
      j["value"] = m.value.flagsValue;
      break;
  }
}

Register::operator std::string() const {
  return RegisterValue(value, desc, timestamp);
}

Register::operator RegisterValue() const {
  return RegisterValue(value, desc, timestamp);
}

void to_json(json& j, const Register& m) {
  j["time"] = m.timestamp;
  std::string data = RegisterValue(m.value);
  j["data"] = data;
}

RegisterStore::operator std::string() const {
  std::stringstream ss;

  // Format we are going for.
  // "  <0x0000> MFG_MODEL                        :700-014671-0000  "
  ss << "  <0x";
  stream_hex(ss, desc.begin, 4);
  ss << "> " << std::setfill(' ') << std::setw(32) << std::left << desc.name
     << " :";
  for (const auto& v : history) {
    if (v) {
      if (desc.format != RegisterValueType::FLAGS)
        ss << ' ';
      else
        ss << '\n';
      ss << std::string(v);
    }
  }
  return ss.str();
}

RegisterStore::operator RegisterStoreValue() const {
  RegisterStoreValue ret(reg_addr, desc.name);
  for (const auto& reg : history) {
    if (reg)
      ret.history.emplace_back(reg);
  }
  return ret;
}

void to_json(json& j, const RegisterStoreValue& m) {
  j["begin"] = m.reg_addr;
  j["name"] = m.name;
  j["readings"] = m.history;
}

void to_json(json& j, const RegisterStore& m) {
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

const RegisterMap& RegisterMapDatabase::at(uint8_t addr) {
  const auto& result = find_if(
      regmaps.begin(),
      regmaps.end(),
      [addr](const std::unique_ptr<RegisterMap>& m) {
        return m->applicable_addresses.contains(addr);
      });
  if (result == regmaps.end())
    throw std::out_of_range("not found: " + std::to_string(int(addr)));
  return **result;
}

void RegisterMapDatabase::load(const nlohmann::json& j) {
  std::unique_ptr<RegisterMap> rmap = std::make_unique<RegisterMap>();
  *rmap = j;
  regmaps.push_back(std::move(rmap));
}

void RegisterMapDatabase::load(const std::string& dir_s) {
  for (auto const& dir_entry : std::filesystem::directory_iterator{dir_s}) {
    std::ifstream ifs(dir_entry.path().string());
    json j;
    ifs >> j;
    load(j);
    ifs.close();
  }
}

void RegisterMapDatabase::print(std::ostream& os) {
  json j = {};
  std::transform(
      regmaps.begin(),
      regmaps.end(),
      std::back_inserter(j),
      [](const auto& ptr) { return *ptr; });
  os << j.dump(4);
}
