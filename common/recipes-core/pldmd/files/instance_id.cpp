#include "instance_id.hpp"

#include <stdexcept>

namespace pldm
{

int InstanceId::next() {
  uint8_t idx = 0;
  while (idx < id.size() && id.test(idx)) {
    ++idx;
  }

  if (idx == id.size()) {
    return -1;
  }

  id.set(idx);
  return idx;
}

} // namespace pldm
