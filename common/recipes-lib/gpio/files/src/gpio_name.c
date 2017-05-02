#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

// Return GPIO number from given string
// e.g. GPIOA0 -> 0, GPIOB6->14, GPIOAA2->210

#define GPIO_BASE_AA0  208

int
gpio_num(char *str)
{
  int len = strlen(str);
  int ret = 0;

  if (len != 6 && len != 7) {
    return -1;
  }
  ret = str[len-1] - '0' + (8 * (str[len-2] - 'A'));
  if (len == 7)
    ret += GPIO_BASE_AA0;
  return ret;
}

char *
gpio_name(int gpio, char *str)
{
  uint8_t n, c, i = 4;
  static char persist_str[8];

  if (!str)
    str = persist_str;

  strcpy(str, "GPIO");
  if (gpio >= GPIO_BASE_AA0) {
    gpio = gpio - GPIO_BASE_AA0;
    strcat(str, "A");
    i++;
  }
  c = gpio / 8;
  n = gpio % 8;
  str[i++] = 'A' + c;
  str[i++] = '0' + n;
  return str;
}

