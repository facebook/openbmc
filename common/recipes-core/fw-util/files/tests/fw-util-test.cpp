#include "fw-util.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace std;

TEST(ComponentTest, FruListTest) {
  EXPECT_NE(nullptr, Component::fru_list);
  EXPECT_EQ(1, Component::fru_list->size());
  EXPECT_EQ(true, Component::fru_list->find("bmc") != Component::fru_list->end());
  EXPECT_EQ(true, (*Component::fru_list)["bmc"].find("bmc") != (*Component::fru_list)["bmc"].end());
}

TEST(ComponentTest, BaseClassTest) {
  EXPECT_EQ(1, Component::fru_list->size());
  Component *c = new Component("test_fru", "test_component");
  EXPECT_EQ(2, Component::fru_list->size());
  EXPECT_EQ(true, Component::fru_list->find("test_fru") != Component::fru_list->end());
  EXPECT_EQ(true, (*Component::fru_list)["test_fru"].find("test_component") != (*Component::fru_list)["test_fru"].end());
  Component *f = Component::find_component("test_fru", "test_component");
  EXPECT_NE(nullptr, f);
  EXPECT_EQ(c, f);
  string image("unused");
  EXPECT_EQ(FW_STATUS_NOT_SUPPORTED, c->update(image));
  EXPECT_EQ(FW_STATUS_NOT_SUPPORTED, c->dump(image));
  EXPECT_EQ(FW_STATUS_NOT_SUPPORTED, c->print_version());
  EXPECT_EQ("test_fru", c->fru());
  EXPECT_EQ("test_component", c->component());
  EXPECT_EQ("test_fru", c->alias_fru());
  EXPECT_EQ("test_component", c->alias_component());
  EXPECT_EQ(false, c->is_alias());
  delete c;
  EXPECT_EQ(1, Component::fru_list->size());
  f = Component::find_component("test_fru", "test_component");
  EXPECT_EQ(nullptr, f);
}

class MockComponent : public Component {
  public:
    MockComponent(string fru, string comp) : Component(fru, comp) {}
    MOCK_METHOD0(print_version, int());
    MOCK_METHOD1(update, int(string image));
    MOCK_METHOD1(dump, int(string image));
};

TEST(ComponentTest, AliasTest) {
  MockComponent *c = new MockComponent("base_fru", "base_comp");
  EXPECT_NE(nullptr, c);
  AliasComponent *a = new AliasComponent("test_fru", "test_comp", "base_fru", "base_comp");
  EXPECT_NE(nullptr, a);
  EXPECT_EQ(false, c->is_alias());
  EXPECT_EQ(true, a->is_alias());
  EXPECT_EQ("base_fru", a->alias_fru());
  EXPECT_EQ("base_comp", a->alias_component());
  string image("test");
  EXPECT_CALL(*c, print_version()).Times(1);
  EXPECT_CALL(*c, dump(image)).Times(1);
  EXPECT_CALL(*c, update(image)).Times(1);
  EXPECT_EQ(0, a->print_version());
  EXPECT_EQ(0, a->update(image));
  EXPECT_EQ(0, a->dump(image));
  delete c;
  delete a;
}


