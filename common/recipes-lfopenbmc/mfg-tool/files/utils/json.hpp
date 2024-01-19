#pragma once

#include <nlohmann/json.hpp>

namespace mfgtool
{
using js = nlohmann::json;

namespace json
{
void display(const js&);
}
} // namespace mfgtool
