#pragma once
#include <string>

namespace mfgtool::utils::string
{

/** Get the last element of a deliminated string.
 *
 *  @param[in] s - The string (or object_path).
 *  @param[in] c - The character to use as the deliminator.
 */
static auto last_element(const auto& s, char c = '/')
{
    if constexpr (requires(decltype(s)& t) { t.str; })
    {
        return last_element(s.str, c);
    }
    else
    {
        return s.substr(s.find_last_of(c) + 1);
    }
}
} // namespace mfgtool::utils::string
