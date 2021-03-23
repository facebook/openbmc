#ifndef __NAME_HPP_
#define __NAME_HPP_
#include <string>
#include <ostream>

class FruNames {
  protected:
    virtual bool is_present(uint8_t fru);
    virtual int get_capability(uint8_t fru, unsigned int* caps);
    virtual int get_fru_name(uint8_t fru, std::string& name);
  public:
    FruNames() {}
    int get_list(const std::string& fru_type, std::ostream& os);
    virtual ~FruNames() {}
};

#endif
