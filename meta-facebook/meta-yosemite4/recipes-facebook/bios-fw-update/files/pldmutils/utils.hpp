#pragma once

#include "types.hpp"

#include <libpldm/base.h>
#include <libpldm/bios.h>
#include <libpldm/edac.h>
#include <libpldm/entity.h>
#include <libpldm/pdr.h>
#include <libpldm/platform.h>
#include <string>
#include <variant>
#include <vector>

namespace pldm {
namespace utils {

/** @brief Convert the buffer to std::string
 *
 *  If there are characters that are not printable characters, it is replaced
 *  with space(0x20).
 *
 *  @param[in] var - pointer to data and length of the data
 *
 *  @return std::string equivalent of variable field
 */
std::string toString(const struct variable_field& var);

} // namespace utils
} // namespace pldm
