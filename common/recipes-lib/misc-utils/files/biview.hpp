#ifndef _BIVIEW_HPP_
#define _BIVIEW_HPP_
/*--------------------------------------------------------
 * CONTAINER: BIVIEW
 * DESCRIPTION: This is a container class which privides
 * a bi-directly read-only (const) map.
 *
 * NEED: There are a lot of places where we would want to
 * do bi-directional look-up. Example, given sensor-id,
 * find a sensor name and vice-versa.
 *
 * EXAMPLE:
 * const biview<int,std::string> sensor_map = {
 *   {0x01, "MB_TEST_SENSOR_1"},
 *   {0x02, "MB_TEST_SENSOR_2"},
 *   {0x03, "MB_TEST_SENSOR_3"},
 *   {0x04, "MB_TEST_SENSOR_4"},
 * };
 *
 * if (sensor_map.find(0x1) == sensor_map.end())
 *   std::cout << "Could not find 0x1\n";
 * cout << "Name of 0x01: " << sensor_map[0x1] << std::endl;
 * cout << "ID of MB_TEST_SENSOR_1: " << sensor_map["MB_TEST_SENSOR_1"] << std::endl;
 */
#include <unordered_map>
#include <iostream>
#include <string>

namespace openbmc {

template<class F, class R>
class biview {
  std::unordered_map<F, R> left;
  std::unordered_map<R, F> right;
  public:
  biview() {}
  biview(const std::initializer_list<std::pair<F, R>> il) {
    for (const auto& it : il) {
      insert(it);
    }
  }
  auto begin() const {
    return left.begin();
  }
  auto end() const {
    return left.end();
  }
  const R& left_at(const F& key) {
    return left.at(key);
  }
  const R& at(const F& key) {
    return left_at(key);
  }
  const F& right_at(const R& key) {
    return right.at(key);
  }
  const R& operator[](const F& key) const {
    return left.at(key);
  }
  const F& operator[](const R& key) const {
    return right.at(key);
  }
  void insert(const std::pair<F,R>& it) {
    if (left.find(it.first) != left.end() || right.find(it.second) != right.end())
      throw std::invalid_argument("Input needs to be strictly 1:1 map");
    left.insert(it);
    std::pair<R,F> r{it.second, it.first};
    right.insert(r);
  }
};
}

#endif

