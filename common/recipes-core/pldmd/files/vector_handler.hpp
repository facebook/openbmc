#pragma once

#include <vector>

template <typename T>
size_t get_count(const std::vector<T> &first) {
    return first.size();
};

template <typename T, typename... Args>
size_t get_count(const std::vector<T> &first, const Args&... args) {
    return first.size() + get_count(args...);
};

template <typename T>
void add_element(std::vector<T> &vec, T element) {
    vec.push_back(element);
};

template <typename T>
void pop_element(std::vector<T> &vec, int index) {
    auto it = vec.begin() + index;
    vec.erase(it);
};
