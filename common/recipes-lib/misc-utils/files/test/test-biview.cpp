#include "../biview.hpp"
#include <iostream>
#include <string>
#include <gtest/gtest.h>

TEST(BIViewTest, basic)
{
  openbmc::biview<int, std::string> my_view;

  my_view.insert({1, "hello"});
  ASSERT_EQ(my_view[1], std::string("hello"));
  ASSERT_EQ(my_view["hello"], 1);
}

TEST(BiViewTest, constTest)
{
  const openbmc::biview<int, std::string> cview = {
    {1, "hello"},
    {2, "world"}
  };
  ASSERT_EQ(cview[1], std::string("hello"));
  ASSERT_EQ(cview[2], std::string("world"));
  ASSERT_EQ(cview["hello"], 1);
  ASSERT_EQ(cview["world"], 2);

  std::unordered_map<int, std::string> refmap = {
    {1, "hello"},
    {2, "world"}
  };
  for (const auto& it : cview) {
    const auto& lkey = it.first;
    const auto& rkey = it.second;
    ASSERT_NE(refmap.find(lkey), refmap.end());
    ASSERT_EQ(refmap[lkey], rkey);
  }
}
