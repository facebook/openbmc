#include <fstream>
#include <functional>
#include <iostream>
#include <string>

#define VERSION_START_OFFSET 0x24
#define VERSION_END_OFFSET 0x35

size_t calculateHash(const std::string& image, size_t startOffset,
                     size_t endOffset);

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <image>" << std::endl;
        return 1;
    }

    std::string image = argv[1];
    size_t hashValue = calculateHash(image, VERSION_START_OFFSET,
                                     VERSION_END_OFFSET);
    std::cout << hashValue << std::endl;

    return 0;
}

size_t calculateHash(const std::string& image, size_t startOffset,
                     size_t endOffset)
{
    std::ifstream file(image, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << image << std::endl;
        return 0;
    }

    file.seekg(startOffset);
    if (!file)
    {
        std::cerr << "Failed to seek to offset " << std::hex << startOffset
                  << std::endl;
        return 0;
    }

    std::string data;
    data.resize(endOffset - startOffset);
    file.read(&data[0], endOffset - startOffset);

    std::hash<std::string> hasher;
    return hasher(data);
}
