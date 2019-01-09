#include <stdlib.h>

/*
 * This file needs to be overriden by "gpio_name.c" under "meta-aspeed".
 */
#error "gpio name->num mapping functions are not implemented"

int
gpio_num(char *str)
{
  return -1;
}

char *
gpio_name(int gpio, char *str)
{
  return NULL;
}

