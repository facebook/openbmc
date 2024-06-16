#pragma once
#include <string>
#include <string_view>

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

/** Get the first element of a deliminated string.
 *
 * @param[in] s - The string (or object_path).
 * @param[in] c - The character to use as the deliminator.
 */
static auto first_element(const auto& s, char c = '/')
{
    if constexpr (requires(decltype(s)& t) { t.str; })
    {
        return first_element(s.str, c);
    }
    else
    {
        return s.substr(0, s.find_first_of(c));
    }
}

/** Replace a substring with another one.
 *
 *  @param[in] s - The string (or object_path) to update.
 *  @param[in] old_p - The old pattern.
 *  @param[in] new_p - The new pattern.
 */
static auto replace_substring(const auto& s, const std::string_view& old_p,
                              const auto& new_p)
{
    if constexpr (requires(decltype(s)& t) { t.str; })
    {
        return replace_substring(s.str, old_p, new_p);
    }
    else
    {
        auto pos = s.find(old_p);
        if (pos == std::string::npos)
        {
            return s;
        }
        auto new_s = s;
        new_s.replace(pos, old_p.length(), new_p);

        return new_s;
    }
}

} // namespace mfgtool::utils::string
