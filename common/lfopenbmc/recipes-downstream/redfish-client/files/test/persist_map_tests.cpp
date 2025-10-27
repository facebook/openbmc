#include <redfish_client/core/persist_map.hpp>

#include <cstdio>
#include <fstream>

#include <gtest/gtest.h>

namespace redfish_client::core
{

TEST(PersistMapTest, InMemoryTest)
{
    PersistMap map;
    EXPECT_EQ(map.update("k1", "v1"), true);
    EXPECT_EQ(map.update("k1", "v1"), false);

    PersistMap recoveryMap;
    EXPECT_EQ(recoveryMap.update("k1", "v1"), true);
}

TEST(PersistMapTest, OnFileTest)
{
    std::string tmpf = std::tmpnam(nullptr);
    PersistMap map(tmpf);
    EXPECT_EQ(map.update("k1", "v1"), true);
    EXPECT_EQ(map.update("k1", "v1"), false);

    PersistMap recoveryMap(tmpf);
    EXPECT_EQ(recoveryMap.update("k1", "v1"), false);
    std::remove(tmpf.c_str());
}

TEST(PersistMapTest, CorruptedFileTest)
{
    std::string tmpf = std::tmpnam(nullptr);
    PersistMap map(tmpf);
    EXPECT_EQ(map.update("k1", "v1"), true);
    EXPECT_EQ(map.update("k1", "v1"), false);
    std::ofstream file(tmpf);
    file << "corrupted";
    file.close();

    PersistMap recoveryMap(tmpf);
    EXPECT_EQ(recoveryMap.update("k1", "v1"), true);
    std::remove(tmpf.c_str());
}

} // namespace redfish_client::core
