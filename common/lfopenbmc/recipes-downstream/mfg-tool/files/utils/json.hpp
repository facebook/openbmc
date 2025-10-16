#pragma once

#include <nlohmann/json.hpp>

#include <regex>
namespace mfgtool
{
using js = nlohmann::json;
using ordered_js = nlohmann::ordered_json;
namespace json
{
void display(const ordered_js&);
inline auto empty_map()
{
    return R"({})"_json;
}
void sort_json_values(std::vector<std::pair<std::string, js>>& values);
ordered_js merge_duplicate_keys_to_json(
    const std::vector<std::pair<std::string, js>>& values);
} // namespace json
} // namespace mfgtool
