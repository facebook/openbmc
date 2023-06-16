#include "instance_id.hpp"

#include <stdexcept>

namespace pldm
{

int InstanceId::next() {
  static uint8_t last_idx = 0;
  uint8_t idx = 0;
  idx = last_idx + 1;

  for (uint8_t i = 0; i < id.size(); i ++, idx ++) {
    if (idx >= id.size()) {
      idx = 0;
    }
    if (!id.test(idx)) {
      last_idx = idx;
      return idx;
    }
  }
  return -1;
}

} // namespace pldm
