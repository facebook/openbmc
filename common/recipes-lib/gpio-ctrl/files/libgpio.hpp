#ifndef _LIBGPIO_HPP_
#define _LIBGPIO_HPP_
#include <iostream>
#include <system_error>

#ifdef __TEST__
#include "libgpio.h"
#else
#include <openbmc/libgpio.h>
#endif

class GPIO {
 protected:
  std::string shadow = "";
  std::string chip = "";
  std::string name = "";
  gpio_desc_t* desc = nullptr;
  int offset = -1;
  void opened() {
    if (!desc)
      throw std::system_error(EBADFD, std::system_category());
  }

 public:
  GPIO(const char* _shadow) : shadow(_shadow) {}
  GPIO(const char* _chip, const char* _name) : chip(_chip), name(_name) {}
  GPIO(const char* _chip, int _offset) : chip(_chip), offset(_offset) {}
  virtual ~GPIO() {}
  virtual void open() {
    if (desc)
      return;
    if (shadow != "") {
      desc = gpio_open_by_shadow(shadow.c_str());
    } else if (name != "") {
      desc = gpio_open_by_name(chip.c_str(), name.c_str());
    } else {
      desc = gpio_open_by_offset(chip.c_str(), offset);
    }
    if (!desc) {
      throw std::system_error(errno, std::system_category());
    }
  }
  virtual void close() {
    if (!desc)
      return;
    gpio_close(desc);
    desc = nullptr;
  }

  virtual gpio_value_t get_value() {
    opened();
    gpio_value_t val = GPIO_VALUE_INVALID;
    if (gpio_get_value(desc, &val) != 0) {
      throw std::system_error(errno, std::system_category());
    }
    return val;
  }
  virtual void set_value(gpio_value_t val, bool init = false) {
    opened();
    int ret = init ? gpio_set_init_value(desc, val) : gpio_set_value(desc, val);
    if (ret != 0) {
      throw std::system_error(errno, std::system_category());
    }
  }
  virtual gpio_direction_t get_direction() {
    opened();
    gpio_direction_t val = GPIO_DIRECTION_INVALID;
    if (gpio_get_direction(desc, &val) != 0) {
      throw std::system_error(errno, std::system_category());
    }
    return val;
  }
  virtual void set_direction(gpio_direction_t val) {
    opened();
    if (gpio_set_direction(desc, val) != 0) {
      throw std::system_error(errno, std::system_category());
    }
  }
  virtual gpio_edge_t get_edge() {
    opened();
    gpio_edge_t val = GPIO_EDGE_INVALID;
    if (gpio_get_edge(desc, &val) != 0) {
      throw std::system_error(errno, std::system_category());
    }
    return val;
  }
  virtual void set_edge(gpio_edge_t val) {
    opened();
    if (gpio_set_edge(desc, val) != 0) {
      throw std::system_error(errno, std::system_category());
    }
  }
};
#endif
