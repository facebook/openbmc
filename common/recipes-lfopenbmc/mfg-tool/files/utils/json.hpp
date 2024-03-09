#pragma once

#include <nlohmann/json.hpp>

namespace mfgtool
{
using js = nlohmann::json;

namespace json
{
void display(const js&);
inline auto empty_map()
{
    return R"({})"_json;
}
} // namespace json
} // namespace mfgtool
