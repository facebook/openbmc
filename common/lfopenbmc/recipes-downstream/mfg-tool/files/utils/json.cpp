#include "utils/json.hpp"

#include <iostream>

namespace mfgtool::json
{
void display(const ordered_js& j)
{
    std::cout << j.dump(4) << std::endl;
}

void sort_json_values(std::vector<std::pair<std::string, js>>& values)
{
    static const std::regex number_pattern(R"(\d+)");
    auto extract_numbers = [](const std::string& str) {
        std::vector<int> numbers;
        std::sregex_iterator it(str.begin(), str.end(), number_pattern);
        static const std::sregex_iterator end;
        while (it != end)
        {
            numbers.push_back(std::stoi(it->str()));
            ++it;
        }
        return numbers;
    };

    std::sort(values.begin(), values.end(), [&](const auto& a, const auto& b) {
        std::vector<int> numbersA = extract_numbers(a.first);
        std::vector<int> numbersB = extract_numbers(b.first);
        return numbersA < numbersB;
    });
}

ordered_js merge_duplicate_keys_to_json(
    const std::vector<std::pair<std::string, js>>& values)
{
    ordered_js json_result = ordered_js::object();

    for (const auto& [key, value] : values)
    {
        if (json_result.contains(key))
        {
            json_result[key].update(value); // Merge values with the same key
                                            // instead of overwriting.
        }
        else
        {
            json_result[key] =
                value; // If the key does not exist, insert the value.
        }
    }
    return json_result;
}
} // namespace mfgtool::json
