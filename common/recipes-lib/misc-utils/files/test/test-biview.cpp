#include "../biview.hpp"
#include <iostream>
#include <string>

int main(void)
{
  openbmc::biview<int, std::string> my_view;

  my_view.insert({1, "hello"});
  if (my_view[1] != std::string("hello") ||
      my_view["hello"] != 1) {
    std::cout << "RW BIVIEW Basic test: FAILED" << std::endl;
    return -1;
  } else {
    std::cout << "RW BIVIEW Basic test: PASSED" << std::endl;
  }

  const openbmc::biview<int, std::string> cview = {
    {1, "hello"},
    {2, "world"}
  };
  if (cview[1] != std::string("hello") ||
      cview[2] != std::string("world") ||
      cview["hello"] != 1 ||
      cview["world"] != 2) {
    std::cerr  << "CONST BIVIEW Basic test: FAILED" << std::endl;
    return -1;
  } else {
    std::cout  << "CONST BIVIEW Basic test: PASSED" << std::endl;
  }
  std::unordered_map<int, std::string> refmap = {
    {1, "hello"},
    {2, "world"}
  };
  for (const auto& it : cview) {
    const auto& lkey = it.first;
    const auto& rkey = it.second;
    if (refmap.find(lkey) == refmap.end() ||
        refmap[lkey] != rkey) {
      std::cerr << "BEGIN Test: FAILED" << std::endl;
      return -1;
    } else {
      std::cout << "BEGIN Test: PASSED" << std::endl;
    }
  }

  std::cout << "ALL TESTS PASSED\n";
  return 0;
}
