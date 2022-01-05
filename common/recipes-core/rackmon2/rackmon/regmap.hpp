#pragma once

#include <nlohmann/json.hpp>
#include <map>
#include <utility>
#include <vector>


// Storage for address ranges. Provides comparision operators
// to allow for it to be used as a key in a map --> This allows
// for us to do quick lookups of addr to register map to use.
struct addr_range {
  // pair of start and end address.
  std::pair<uint8_t, uint8_t> range{};
  addr_range() {}
  explicit addr_range(uint8_t a, uint8_t b) : range(a, b) {}
  explicit addr_range(uint8_t a) : range(a, a) {}

  bool contains(uint8_t) const;
  bool operator<(const addr_range& rhs) const;
};

// Describes how we intend on interpreting the value stored
// in a register.
enum RegisterValueType {
  HEX,
  ASCII,
  DECIMAL,
  FIXED_POINT,
  TABLE,
};

// Fully describes a Register (Retrieved from register map JSON)
struct RegisterDescriptor {
  // Starting address of the Register
  uint16_t begin = 0;
  // Length of the Register
  uint16_t length = 0;
  // Name of the Register
  std::string name{};
  // How deep is the historical record? If keep is 6, rackmon
  // will keep the latest 6 read values for later retrieval.
  uint16_t keep = 1;
  // This is a caveat of the 'keep' value. This forces us
  // to keep a value only if it changed from the previous read
  // value. Useful for state information.
  bool changes_only = false;

  // Describes how to interpret the contents of the register.
  RegisterValueType format = RegisterValueType::HEX;

  // If the register stores a FIXED_POINT type, this provides
  // the precision.
  uint16_t precision = 0;

  // If the register stores a table, this provides the desc.
  std::vector<std::tuple<uint8_t, std::string>> table{};
};

// Container of a instance of a register at a given point in time.
struct Register {
  // Reference to the register descriptor.
  const RegisterDescriptor& desc;
  // Timestamp when the register was read.
  uint32_t timestamp = 0;
  // Actual value of the register/register-range
  std::vector<uint16_t> value;

  explicit Register(const RegisterDescriptor& d)
      : desc(d), value(d.length) {}

  // equals operator works only on valid register reads. Register
  // with a zero timestamp is considered as invalid.
  bool operator==(const Register& other) const {
    return timestamp != 0 && other.timestamp != 0 && value == other.value;
  }
  // Same but opposite of the equals operator.
  bool operator!=(const Register& other) const {
    return timestamp == 0 || other.timestamp == 0 || value != other.value;
  }
  // Returns true if the register contents is valid.
  operator bool() const {
    return timestamp != 0;
  }
  // Returns a string formatted value.
  std::string format() const;

  // Helper methods to parse the registers.
  static int32_t to_integer(const std::vector<uint16_t>& value);
  static std::string to_string(const std::vector<uint16_t>& value);
};
void to_json(nlohmann::json& j, const Register& m);

// Container of values of a single register at multiple points in
// time. (RegisterDescriptor::keep defines the size of the depth
// of the historical record).
struct RegisterStore {
  // Reference to the register descriptor
  const RegisterDescriptor& desc;
  // Address of the register.
  uint16_t reg_addr;
  // History of the register contents to keep. This is utilized as
  // a circular buffer with idx pointing to the current slot to
  // write.
  std::vector<Register> history;
  int32_t idx = 0;

 public:
  explicit RegisterStore(const RegisterDescriptor& d)
      : desc(d), reg_addr(d.begin), history(d.keep, Register(d)) {}

  // Returns a reference to the last written value (Back of the list)
  Register& back() {
    return idx == 0 ? history.back() : history[idx - 1];
  }
  // Returns the front (Next to write) reference
  Register& front() {
    return history[idx];
  }
  // Advances the front.
  void operator++() {
    idx = (idx + 1) % history.size();
  }
  // Returns a string formatted representation of the historical record.
  std::string format() const;
};
void to_json(nlohmann::json& j, const RegisterStore& m);

// Container of an entire register map. This is the memory
// representation of each JSON register map descriptors
// at /etc/rackmon.d.
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

// Container of multiple register maps. It is keyed off
// the address range of each register map.
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
