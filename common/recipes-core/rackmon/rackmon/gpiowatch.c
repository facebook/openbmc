/*
 * Copyright 2014-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#define CHECK(x) { if((x) < 0) { \
    error = x;  \
    goto cleanup; \
} }

#define CHECKNULL(x) { if((x) == NULL) { \
    error = -1;  \
    goto cleanup; \
} }

typedef struct {
  int fd;
  int gpio;
  char value;
} wgpio;

int main(int argc, char **argv) {
    int error = 0;
    int nfds =  argc - 1;
    int i = 0;
    int polliv = 10000;
    wgpio* wgpios = NULL;

    if(argc < 2) {
      fprintf(stderr, "Usage: %s <gpio num> [gpio num] [gpio num...]\n", argv[0]);
      exit(1);
    }
    if(getenv("POLL_US")) {
      polliv = atoi(getenv("POLL_US"));
    }

    wgpios = calloc(nfds, sizeof(wgpio));
    CHECKNULL(wgpios)

    fprintf(stderr, "Watching %d gpios:", nfds);
    for(i = 1; i < argc; i++) {
      char filename[255];
      int gpio_num = atoi(argv[i]);
      wgpios[i - 1].gpio = gpio_num;
      snprintf(filename, 255, "/sys/class/gpio/gpio%d/value", gpio_num);
      wgpios[i - 1].fd = open(filename, O_RDONLY);
      CHECK(wgpios[i - 1].fd);
      CHECK(read(wgpios[i - 1].fd, &(wgpios[i - 1].value), 1));
      fprintf(stderr, " %d (currently: %c)", gpio_num, wgpios[i - 1].value);
    }
    fprintf(stderr, "\n");

    do {
      usleep(polliv);
      for(i = 0; i < nfds; i++) {
          wgpio* w = &wgpios[i];
          char value;
          lseek(w->fd, 0, SEEK_SET);
          CHECK(read(w->fd, &value, 1));
          if(value != w->value) {
            printf("GPIO%d: %c -> %c\n", w->gpio, w->value, value);
            w->value = value;
          }
      }
    } while (1);

cleanup:
    if(error != 0) {
      fprintf(stderr, "Error %d: %s\n", errno, strerror(errno));
      error = 1;
    }
    return error;
}
