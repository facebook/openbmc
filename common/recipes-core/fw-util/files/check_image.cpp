#include <string>
#include <list>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <arpa/inet.h>
extern "C" {
  #include <libfdt.h>
}
#include <openssl/sha.h>
#include <zlib.h>
#include "bmc.h"

int __attribute__((weak)) fdt_first_subnode(const void *fdt, int offset)
{
  int depth = 0;

  offset = fdt_next_node(fdt, offset, &depth);
  if (offset < 0 || depth != 1)
    return -FDT_ERR_NOTFOUND;

  return offset;
}
int __attribute__((weak)) fdt_next_subnode(const void *fdt, int offset)
{
  int depth = 1;
  do {
    offset = fdt_next_node(fdt, offset, &depth);
    if (offset < 0 || depth < 1)
      return -FDT_ERR_NOTFOUND;
  } while (depth > 1);
  return offset;
}

#ifndef fdt_for_each_subnode
#define fdt_for_each_subnode(node, fdt, parent)    \
  for (node = fdt_first_subnode(fdt, parent);  \
       node >= 0;          \
       node = fdt_next_subnode(fdt, node))
#endif

#define FLASH_SIZE (32 * 1024 * 1024)

using namespace std;

class Checker {
  protected:
  string name;
  off_t offset;
  off_t size;
  public:
  Checker(string n, off_t of, off_t sz) : name(n), offset(of), size(sz) {}
  virtual bool is_valid(const unsigned char *image) {
    return true;
  }
};

class LegacyChecker : public Checker {
  private:
    static const uint32_t MAGIC             = 0x27051956;
    static const off_t    HEADER_SIZE       = 64;
    static const off_t    MAGIC_OFFSET      = 0;
    static const off_t    HEADER_CRC_OFFSET = 4;
    static const off_t    SIZE_OFFSET       = 12;
    static const off_t    DATA_CRC_OFFSET   = 24;
    uint32_t get_word(const unsigned char *bytes, off_t offset)
    {
      return ntohl(*(uint32_t *)(bytes + offset));
    }
    void set_word(const unsigned char *bytes, off_t offset, uint32_t word)
    {
      *(uint32_t *)(bytes + offset) = htonl(word);
    }
  public:
    LegacyChecker(string n, off_t of, off_t sz) : Checker(n, of, sz) {}

  virtual bool is_valid(const unsigned char *image) {
    uint32_t hcrc, dcrc, hcrc_c, dcrc_c;
    unsigned char hdr[HEADER_SIZE];
    const unsigned char *data;

    if (size <= HEADER_SIZE) {
      return false;
    }
    image = image + offset;

    memcpy(hdr, image, HEADER_SIZE);
    if (get_word(hdr, MAGIC_OFFSET) != MAGIC) {
      return false;
    }
    hcrc = get_word(hdr, HEADER_CRC_OFFSET);
    set_word(hdr, HEADER_CRC_OFFSET, 0);
    hcrc_c = crc32(0, hdr, HEADER_SIZE);
    if (hcrc != hcrc_c) {
      return false;
    }
    dcrc = get_word(hdr, DATA_CRC_OFFSET);
    off_t len  = (off_t)get_word(hdr, SIZE_OFFSET);
    data = image + HEADER_SIZE;
    if (len + HEADER_SIZE > size) {
      return false;
    }
    dcrc_c = crc32(0, data, len);
    if (dcrc != dcrc_c) {
      return false;
    }
    return true;
  }
};

class FITChecker : public Checker {
  int num_nodes;
  public:
  FITChecker(string n, off_t of, off_t sz, int nodes) : Checker(n, of, sz), num_nodes(nodes) {}

  virtual bool is_valid(const unsigned char *image) {
      const void *fdt = (const void *)(image + offset);
      int nodep, node, hashnode;
      size_t data_size;
      uint32_t data_pos;
      const unsigned char *data = NULL;
      unsigned char shasum[SHA256_DIGEST_LENGTH];
      int len = 0;
      int valid_nodes = 0;

      if (size < (off_t)FDT_V17_SIZE || fdt_check_header(fdt) != 0) {
        return false;
      }

      // Get the list of images stored in this FDT
      nodep = fdt_subnode_offset(fdt, 0, "images");
      if (nodep < 0) {
        return false;
      }

      // For each image, compute the sha256 digest and
      // compare it against the one stored in the FDT */
      fdt_for_each_subnode(node, fdt, nodep) {
        if (node < 0) {
          continue;
        }
        // Get the data portion and compute its sha256 digest */
        data = (const unsigned char *)fdt_getprop(fdt, node, "data", &len);
        if (!data || len == 0) {
          // Could not find data property. See if we have
          // a link to the image instead */
          data = (const unsigned char *)fdt_getprop(fdt, node, "data-size", &len);
          if (!data || len != 4) {
            return false;
          }
          data_size = ntohl(*(uint32_t *)data);
          data = (const unsigned char *)fdt_getprop(fdt, node, "data-position", &len);
          if (!data || len != 4) {
            return false;
          }
          data_pos = ntohl(*(uint32_t *)data);
          data = (const unsigned char *)fdt + data_pos;
        } else {
          data_size = (size_t)len;
        }
        if ((off_t)data_size > size) {
          // This partition cannot contain this FDT. Most
          // probably we are using the wrong partition 
          //description 
          return false;
        }
        SHA256(data, data_size, shasum);

        // Get the sha256 digest stored in the image */
        hashnode = fdt_subnode_offset(fdt, node, "hash@1");
        if (hashnode < 0) {
          return false;
        }
        data = (const unsigned char *)fdt_getprop(fdt, hashnode, "algo", &len);
        if (!data || strlen("sha256") > (unsigned int)len || strcmp((char *)data, "sha256") != 0) {
          return false;
        }
        data = (const unsigned char *)fdt_getprop(fdt, hashnode, "value", &len);
        if (!data || len != SHA256_DIGEST_LENGTH) {
          return false;
        }

        // Check if they match */
        if (memcmp(data, shasum, SHA256_DIGEST_LENGTH)) {
          return false;
        }
        valid_nodes++;
      }
      nodep = fdt_subnode_offset(fdt, 0, "configurations");
      if (nodep < 0) {
        return false;
      }
      return valid_nodes >= num_nodes ? true : false;
    }
};

class PartitionDescriptor
{
    string  name;
    off_t   offset;
    off_t   size;
    Checker *checker;
    int     num_nodes;
    string  type;
  public:
    PartitionDescriptor(string n, json& obj) : name(n), offset(0), size(0), num_nodes(1) {
      auto tmp = obj["offset"].get_ptr<const json::number_integer_t* const>();
      if ( tmp == nullptr ) {
        throw "OFFSET not found parsing " + name;
      }
      offset = (*tmp) * 1024;

      tmp = obj["size"].get_ptr<const json::number_integer_t* const>();
      if ( tmp == nullptr ) {
        throw "SIZE not found parsing " + name;
      }
      size = (*tmp) * 1024;
      if ( size == 0 ) {
        throw "SIZE cannot be zero parsing " + name;
      }

      tmp = obj["num-nodes"].get_ptr<const json::number_integer_t* const>();
      if ( tmp != nullptr ) {
        num_nodes = *tmp;
      }

      auto tmp_str = obj["type"].get_ptr<const json::string_t* const>();
      if ( tmp_str == nullptr ) {
        throw "TYPE not found parsing " + name;
      }
      type = *tmp_str;

      if (type == "fit") {
        checker = new FITChecker(name, offset, size, num_nodes);
      } else if (type == "legacy") {
        checker = new LegacyChecker(name, offset, size);
      } else if (type == "ignore") {
        checker = new Checker(name, offset, size);
      } else {
        throw "TYPE unknown" + type + " in " + name;
      }
    }
    bool valid(const unsigned char *image, off_t image_size)
    {
      if (image_size < offset)
        return false;
      // A valid image might not take up the whole partition.
      // So image_size < offset + size is possible.
      return checker->is_valid(image);
    }
};

class ImageDescriptor {
  string name;
  list<PartitionDescriptor *> partitions;
  public:
  ImageDescriptor(string n, json& obj) : name(n), partitions() {
    if (obj.size() == 0) {
      return;
    }
    for (auto& j : obj.items()) {
      try {
        partitions.push_back(new PartitionDescriptor(j.key(), j.value()));
      } catch (string &ex) {
        throw "Exception received parsing " + j.key() + " (" + ex + ")";
      }
    }
  }
  bool is_valid(const unsigned char *image, size_t size) {
    for (auto it = partitions.begin(); it != partitions.end(); it++) {
      if (!(*it)->valid(image, size)) {
        return false;
      }
    }
    return true;
  }
  ~ImageDescriptor() {
    partitions.clear();
  }
};

class ImageDescriptorList;

class Image {
  const unsigned char *image;
  size_t               fsize;
  int                  fd;
  friend class         ImageDescriptorList;
  bool match(const char *s, const char *p)
  {
    for (; *s != '\0' && *p != '\0'; s++, p++) {
      bool esc = false;
      if (*p == '\\') {
        p++;
        esc = true;
      }
      // If escaping and we are looking for a decimal
      if (esc && *p == 'd') {
        if (!isdigit(*s)) {
          return false;
        }
      } else if (*p != *s) {
        return false;
      }
    }
    return true;
  }
  bool istrncmp(const char *s1, const char *s2, size_t len)
  {
    for (size_t i = 0; i < len; i++, s1++, s2++) {
      if (tolower(*s1) != tolower(*s2))
        return false;
    }
    return true;
  }
  public:
  Image(string &file) : image(NULL) {
    fd = open(file.c_str(), O_RDONLY);
    if (fd < 0) {
      throw "Cannot open " + string(file);
    }
    fsize = lseek(fd, 0, SEEK_END);
    if (fsize == 0) {
      throw "Zero size image file " + string(file);
    }
    lseek(fd, 0, SEEK_SET);
    unsigned char *img = (unsigned char *)calloc(FLASH_SIZE, 1);
    for (int bread = 0; bread < (int)fsize;) {
      int rc = read(fd, img + bread, fsize - bread);
      if (rc > 0) {
        bread += rc;
      }
    }
    image = (const unsigned char *)img;
  }
  ~Image() {
    if (image)
      free((void *)image);
    if (fd >= 0)
      close(fd);
  }
  bool supports_machine(string &machine) {
    // Just dont check in the last 256 bytes of the image. Technically we
    // need to find this in the uboot section so it should be pretty early on.
    for (int i = 0; i < (int)fsize - 256; i++) {
      const char *str = (const char *)&image[i];
      if (isalnum(image[i]) && match(str, "U-Boot \\d\\d\\d\\d\\.\\d\\d ")) {
        str += 15;
        if (*str == '(') {
          for (int j = 0; j < 32 && *str != ')'; j++, str++);
          if (*(str++) != ')')
            continue;
          for (; *str == ' '; str++);
        }
        if (istrncmp(str, machine.c_str(), machine.size()))
          return true;
      }
    }
    return false;
  }
};



class ImageDescriptorList {
  list<ImageDescriptor *> images;
  json conf;
  public:
  ImageDescriptorList(const char *desc_file) : conf(nullptr) {
    ifstream ifs(desc_file);
    if (!ifs) {
      throw "Cannot load file " + string(desc_file);
    }

    try {
      conf = json::parse(ifs);
    } catch (json::parse_error& e) {
      cout << e.what() << endl;
      throw "Cannot parse file " + string(desc_file);
    }

    if (conf.size() == 0) {
      throw "Unsupported number of images in " + string(desc_file);
    }

    for (auto& j : conf.items()) {
      try {
        images.push_back(new ImageDescriptor(j.key(), j.value()));
      } catch (string &ex) {
        throw "Parsing " + string(desc_file) + " invalid " + j.key() + " (" + ex + ")";
      }
    }
  }
  ~ImageDescriptorList() {
  }
  bool is_valid(Image &image)
  {
    for (auto it = images.begin(); it != images.end(); it++) {
      if ((*it)->is_valid(image.image, image.fsize))
        return true;
    }
    return false;
  }
};

bool BmcComponent::is_valid(string &file, bool pfr_active)
{
  bool valid = false;
  try {
    Image image(file);
    string machine = system.name();
    if (!image.supports_machine(machine)) {
      return false;
    }

    if (pfr_active) {
      return true;
    }

    ImageDescriptorList desc_list(system.partition_conf().c_str());
    valid = desc_list.is_valid(image);
  } catch(string &ex) {
    cerr << ex << endl;
    return false;
  }
  return valid;
}

