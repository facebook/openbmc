#pragma once

#include <nlohmann/json.hpp>
#include <map>
#include <utility>
#include <vector>

// Address formats:
// |--7--|--6--|--5--|--4--|--3--|--2--|--1--|--0--|
// | T1  | T0  | R2  | R1  | R0  | D2  | D1  | D0  |
// |-----|-----|-----|-----|-----|-----|-----|-----|
// T1:T0  | Desc
// -------------
// 1  1   | PSU
// 0  1   | BBU
// 0  0   | Special devices
// 1  0   | PSU+BBU (ORv2)
//
// R2:R0 - Rack address.
// D2:D0 - Device (PSU/BBU/PSU+BBU/Special) address.
//
// For legacy ORv2 devices,
// R2 - always 1
// R2:R1 - Rack address
// D2 - Shelf address
// D1:D0 - PSU Address
//
// For more flexibility, this is all provided by the JSON
// regmap files. All the source files here care about
// is the address ranges for which particular maps
// apply. We also use this to limit our scans.
struct addr_range {
  std::pair<uint8_t, uint8_t> range{};
  addr_range() {}
  explicit addr_range(uint8_t a, uint8_t b) : range(a, b) {}
  explicit addr_range(uint8_t a) : range(a, a) {}

  bool contains(uint8_t) const;

  // less operator for addr_range to allow for it to be
  // used as a key for map (Cool range map in DB)
  bool operator<(const addr_range& rhs) const;
};

enum RegisterFormatType {
  HEX,
  ASCII,
  DECIMAL,
  FIXED_POINT,
  TABLE,
};

struct RegisterDescriptor {
  uint16_t begin = 0;
  uint16_t length = 0;
  std::string name{};
  uint16_t keep = 1;
  bool changes_only = false;
  RegisterFormatType format = RegisterFormatType::HEX;
  uint16_t precision = 0;
  std::vector<std::tuple<uint8_t, std::string>> table;
};

struct RegisterValue {
  const RegisterDescriptor& desc;
  uint32_t timestamp = 0;
  std::vector<uint16_t> value;
  explicit RegisterValue(const RegisterDescriptor& d) : desc(d), value(d.length) {}
};

void to_json(nlohmann::json& j, const RegisterValue& m);

struct RegisterValueStore {
  const RegisterDescriptor& desc;
  uint16_t reg_addr;
  std::vector<RegisterValue> history;
  int32_t idx = 0;

 public:
  explicit RegisterValueStore(const RegisterDescriptor& d)
      : desc(d), reg_addr(d.begin), history(d.keep, RegisterValue(d)) {}
};
void to_json(nlohmann::json& j, const RegisterValueStore& m);

struct RegisterMap {
  addr_range applicable_addresses;
  std::string name;
  uint8_t probe_register;
  uint32_t default_baudrate;
  uint32_t preferred_baudrate;
  std::map<uint16_t, RegisterDescriptor> register_descriptors;
  const RegisterDescriptor& at(uint16_t reg) const {
    return register_descriptors.at(reg);
  }
};

struct RegisterMapDatabase {
  std::map<addr_range, RegisterMap> regmaps{};
  RegisterMap& at(uint16_t addr);
  void load(const std::string& dir_s);
  // For debug purpose only.
  void print(std::ostream& os);
};

// JSON conversion
void from_json(const nlohmann::json& j, RegisterMap& m);
void from_json(const nlohmann::json& j, addr_range& a);
void from_json(const nlohmann::json& j, RegisterDescriptor& i);

void from_json(nlohmann::json& j, const RegisterMap& m);
void from_json(nlohmann::json& j, const addr_range& a);
void from_json(nlohmann::json& j, const RegisterDescriptor& i);
