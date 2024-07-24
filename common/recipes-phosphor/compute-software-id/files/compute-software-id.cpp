#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <cstdint>

#define VERSION_LENGTH_OFFSET 0x23
#define VERSION_START_OFFSET 0x24

size_t calculateHash(const std::string& image);

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <image>" << std::endl;
        return 1;
    }

    std::string image = argv[1];
    size_t hashValue = calculateHash(image);
    std::cout << hashValue << std::endl;

    return 0;
}

size_t calculateHash(const std::string& image)
{
    std::ifstream file(image, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << image << std::endl;
        return 0;
    }

    file.seekg(VERSION_LENGTH_OFFSET);
    if (!file)
    {
        std::cerr << "Failed to seek to offset " << std::hex << VERSION_LENGTH_OFFSET << std::endl;
        return 0;
    }

    uint8_t versionLength;
    file.read(reinterpret_cast<char*>(&versionLength), sizeof(versionLength));

    size_t endOffset = VERSION_START_OFFSET + versionLength;

    file.seekg(VERSION_START_OFFSET);
    if (!file)
    {
        std::cerr << "Failed to seek to offset " << std::hex << VERSION_START_OFFSET << std::endl;
        return 0;
    }

    // Read data from start offset to end offset
    std::string data;
    data.resize(endOffset - VERSION_START_OFFSET);
    file.read(&data[0], endOffset - VERSION_START_OFFSET);

    std::hash<std::string> hasher;
    return hasher(data);
}