#include <arpa/inet.h>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <zlib.h>
#include <iomanip>
#include <iostream>
#include <string>
#include "bmc.h"
#include "image.hpp"
#include "libfdt_compat.h"

using nlohmann::json;

// Add compatibility for SSL 1.x :-/
#if OPENSSL_VERSION_NUMBER < 0x10100000L
#define EVP_MD_CTX_new EVP_MD_CTX_create
#define EVP_MD_CTX_free EVP_MD_CTX_destroy
#endif

std::string getMd5(const char* data, size_t size) {
  unsigned int MD5Len = EVP_MD_size(EVP_md5());
  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  EVP_DigestInit_ex(ctx, EVP_md5(), NULL);
  EVP_DigestUpdate(ctx, data, size);
  auto MD5Digest = (unsigned char*)OPENSSL_malloc(MD5Len);
  EVP_DigestFinal_ex(ctx, MD5Digest, &MD5Len);
  EVP_MD_CTX_free(ctx);
  std::stringstream ss;
  for (size_t i = 0; i < MD5Len; i++) {
    unsigned int v = (unsigned int)MD5Digest[i];
    ss << std::setw(2) << std::hex << std::setfill('0') << v;
  }
  return ss.str();
}

void validateRaw(
    const Image& image,
    size_t offset,
    size_t size,
    const std::string& expectedDigest) {
  char* data = image.peek(offset, size);
  std::string md5 = getMd5(data, size);
  if (md5 != expectedDigest) {
    throw std::runtime_error("Bad MD5");
  }
}

void validateLegacy(const Image& image, size_t offset, size_t size) {
  uint32_t MAGIC = 0x27051956;
  static constexpr size_t HEADER_SIZE = 64;
  static constexpr off_t MAGIC_OFFSET = 0;
  static constexpr off_t HEADER_CRC_OFFSET = 4;
  static constexpr off_t SIZE_OFFSET = 12;
  static constexpr off_t DATA_CRC_OFFSET = 24;
  auto get_word = [](const unsigned char* bytes, off_t offset) {
    return ntohl(*(uint32_t*)(bytes + offset));
  };
  auto set_word = [](const unsigned char* bytes, off_t offset, uint32_t word) {
    *(uint32_t*)(bytes + offset) = htonl(word);
  };

  if (size <= HEADER_SIZE) {
    throw std::runtime_error("Image too small");
  }
  unsigned char hdr[HEADER_SIZE];
  memcpy(hdr, image.peekExact(offset, sizeof(hdr)), sizeof(hdr));
  if (get_word(hdr, MAGIC_OFFSET) != MAGIC) {
    throw std::runtime_error("Incorrect magic");
  }
  uint32_t hcrc = get_word(hdr, HEADER_CRC_OFFSET);
  set_word(hdr, HEADER_CRC_OFFSET, 0);
  uint32_t hcrc_c = crc32(0, hdr, HEADER_SIZE);
  if (hcrc != hcrc_c) {
    throw std::runtime_error("CRC Mismatch");
  }
  uint32_t dcrc = get_word(hdr, DATA_CRC_OFFSET);
  off_t len = (off_t)get_word(hdr, SIZE_OFFSET);
  if (len + HEADER_SIZE > size) {
    throw std::runtime_error("Size mismatch");
  }
  unsigned char* data =
      (unsigned char*)image.peekExact(offset + HEADER_SIZE, len);
  uint32_t dcrc_c = crc32(0, data, len);
  if (dcrc != dcrc_c) {
    throw std::runtime_error("Data CRC mismatch");
  }
}

const unsigned char* getNodeData(const void* fdt, int node, size_t& dataSize) {
  int len = 0;
  // Get the data portion from the data property, if not available,
  // look for the data-size and data-position property.
  const unsigned char* data =
      (const unsigned char*)fdt_getprop(fdt, node, "data", &len);
  if (!data || len == 0) {
    // Could not find data property. See if we have
    // a link to the image instead
    data = (const unsigned char*)fdt_getprop(fdt, node, "data-size", &len);
    if (!data || len != 4) {
      throw std::runtime_error("Neither data or data-size property available");
    }
    dataSize = ntohl(*(uint32_t*)data);
    data = (const unsigned char*)fdt_getprop(fdt, node, "data-position", &len);
    if (!data || len != 4) {
      throw std::runtime_error(
          "data-position property expected but not provided");
    }
    size_t data_pos = ntohl(*(uint32_t*)data);
    data = (const unsigned char*)fdt + data_pos;
  } else {
    dataSize = (size_t)len;
  }
  return data;
}

void validateFITNode(const void* fdt, int node, size_t maxSize) {
  size_t dataSize;
  const unsigned char* data = getNodeData(fdt, node, dataSize);
  if (dataSize > maxSize) {
    // This partition cannot contain this FDT. Most
    // probably we are using the wrong partition
    // description
    throw std::runtime_error("FDT larger than partition");
  }
  unsigned char shasum[SHA256_DIGEST_LENGTH];
  SHA256(data, dataSize, shasum);

  // Get the sha256 digest stored in the image */
  int hashnode = fdt_subnode_offset(fdt, node, "hash@1");
  if (hashnode < 0) {
    throw std::runtime_error("Could not get hash@1 property");
  }
  int len;
  data = (const unsigned char*)fdt_getprop(fdt, hashnode, "algo", &len);
  if (!data || strlen("sha256") > (unsigned int)len ||
      strcmp((char*)data, "sha256") != 0) {
    throw std::runtime_error("Could not get algo or unsupported algo received");
  }
  data = (const unsigned char*)fdt_getprop(fdt, hashnode, "value", &len);
  if (!data || len != SHA256_DIGEST_LENGTH) {
    throw std::runtime_error("Could not read sha256 correctly");
  }

  // Check if they match
  if (memcmp(data, shasum, SHA256_DIGEST_LENGTH)) {
    throw std::runtime_error("Checksums do not match");
  }
}

void validateFIT(
    const Image& image,
    size_t offset,
    size_t size,
    size_t numExpectedNodes) {
  const void* fdt = image.peek(offset, size);
  if (numExpectedNodes < 1 || size < FDT_V17_SIZE ||
      fdt_check_header(fdt) != 0) {
    throw std::runtime_error("Invalid numNodes or size or FDT header");
  }
  if (fdt_totalsize(fdt) > size) {
    throw std::runtime_error("Image too small to hold FDT");
  }

  // Get the list of images stored in this FDT
  int nodep = fdt_subnode_offset(fdt, 0, "images");
  if (nodep < 0) {
    throw std::runtime_error("Could not get \"images\"");
  }

  size_t numNodes = 0;
  // For each image, compute the sha256 digest and
  // compare it against the one stored in the FDT
  int node;
  fdt_for_each_subnode(node, fdt, nodep) {
    if (node < 0) {
      continue;
    }
    validateFITNode(fdt, node, size);
    numNodes++;
  }
  nodep = fdt_subnode_offset(fdt, 0, "configurations");
  if (nodep < 0) {
    throw std::runtime_error("configurations subnode not found");
  }
  if (numNodes < numExpectedNodes) {
    throw std::runtime_error("Unexpected number of FDT nodes");
  }
}

void getMeta(const Image& img, json& desc) {
  char* data = img.peekExact(0x000F0000, 64 * 1024);
  if (data[0] != '{') {
    throw std::runtime_error("Not a JSON");
  }
  auto off = std::find(data, data + 64 * 1024, '\0');
  std::string str(data, off);
  auto end = str.find_first_of('\n');
  std::string obj1 = str.substr(0, end);
  std::string obj2 = str.substr(end + 1, str.length() + 1);
  json metaDesc = json::parse(obj2);
  std::string currMd5 = getMd5(data, obj1.length());
  if (currMd5 != metaDesc["meta_md5"]) {
    throw std::runtime_error("meta md5 mismatch!");
  }
  desc = json::parse(obj1);
}

std::string getPlatformName(const std::string_view& uboot) {
  size_t off = 0;
  const std::string keyStr{"U-Boot 20"};
  const std::regex ubootVerRegex(R"(U-Boot \d{4}\.\d{2}\s+(\S+)\s+)");
  while (off != uboot.npos) {
    off = uboot.find(keyStr, off);
    if (off == uboot.npos) {
      break;
    }
    std::string_view checkStr = uboot.substr(off, 256);
    std::match_results<typename decltype(checkStr)::const_iterator>
        matchResults{};
    auto ret = std::regex_search(
        checkStr.begin(), checkStr.end(), matchResults, ubootVerRegex);
    if (ret) {
      size_t matchedStrLen = static_cast<size_t>(matchResults.length(1));
      auto matchedStr = checkStr.data() + matchResults.position(1);
      std::string_view vers{matchedStr, matchedStrLen};
      std::string_view platform = vers.substr(0, vers.find_first_of('-'));
      return std::string(platform.data(), platform.length());
    }
    off += keyStr.length();
  }
  throw std::runtime_error("Cannot find platform name");
}

void validatePlatformName(const Image& image, const std::string& machine) {
  std::string checkMachine = machine;
  std::transform(
      checkMachine.begin(),
      checkMachine.end(),
      checkMachine.begin(),
      ::tolower);
  size_t ubootSize = 1024 * 1024;
  std::string_view uboot(image.peekExact(0, ubootSize), ubootSize);
  std::string machineName = getPlatformName(uboot);
  std::transform(
      machineName.begin(), machineName.end(), machineName.begin(), ::tolower);
  if (machineName != checkMachine) {
    throw std::runtime_error("Image does not support: " + machine);
  }
}

void validateMeta(const Image& image) {
  json desc;
  getMeta(image, desc);
  for (const auto& part : desc["part_infos"]) {
    if (part["type"] == "data" || part["type"] == "meta") {
      continue;
    }
    std::string name = part["name"];
    if (part["type"] == "fit") {
      std::cout << "Validating FIT " << name << std::endl;
      validateFIT(image, part["offset"], part["size"], part["num-nodes"]);
    } else if (part["type"] == "rom" || part["type"] == "raw") {
      std::cout << "Validating RAW " << name << std::endl;
      validateRaw(image, part["offset"], part["size"], part["md5"]);
    } else {
      std::cout << "WARNING: IGNORING UNKNOWN PARTITION: " << part.dump()
                << std::endl;
    }
  }
}

void validateLayout(const Image& image, json& layout) {
  for (auto& partition : layout.items()) {
    auto& desc = partition.value();
    std::string partType = desc["type"];
    if (partType == "ignore") {
      continue;
    }
    size_t offset = desc.at("offset");
    size_t size = desc.at("size");
    size_t numNodes = desc.value("num-nodes", 1);
    offset *= 1024;
    size *= 1024;
    if (partType == "fit") {
      validateFIT(image, offset, size, numNodes);
    } else if (partType == "legacy") {
      validateLegacy(image, offset, size);
    } else {
      throw std::runtime_error("Unrecognized partition type: " + partType);
    }
  }
}

void validateLegacy(const Image& image, const std::string& partsPath) {
  std::ifstream ifs(partsPath);
  json parts = json::parse(ifs);
  for (auto& image_layout : parts.items()) {
    try {
      // At least one layout should succeed.
      validateLayout(image, image_layout.value());
      return;
    } catch (std::exception& e) {
      continue;
    }
  }
  throw std::runtime_error("None of the Partition validators succeeded");
}

void validate(
    const std::string& path,
    const std::string& machine,
    bool isMeta,
    const std::string& partsPath) {
  Image image(path);
  if (image.size() < 0x00F00000 + (64 * 1024)) {
    throw std::runtime_error("Image too small");
  }
  validatePlatformName(image, machine);
  if (isMeta) {
    validateMeta(image);
  } else {
    validateLegacy(image, partsPath);
  }
}

#ifndef TEST_STANDALONE_VALIDATOR
bool BmcComponent::is_valid(const std::string& image, bool /* pfr_active */) {
  try {
    validate(
        image,
        sys().name(),
        sys().get_mtd_name("meta"),
        "/etc/image_parts.json");
  } catch (std::exception& ex) {
    std::cerr << ex.what() << std::endl;
    return false;
  }
  return true;
}
#else
int main(int argc, char* argv[]) {
  if (argc < 5) {
    std::cerr << "USAGE: " << argv[0]
              << " IMAGE_PATH PLATFORM_NAME META_OR_NOT PARTS_PATH\n";
    return -1;
  }
  validate(argv[1], argv[2], strcmp(argv[3], "true") == 0, argv[4]);
}
#endif
