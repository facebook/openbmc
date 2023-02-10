#pragma once

extern "C" {
#include <libfdt.h>
}

int __attribute__((weak)) fdt_first_subnode(const void* fdt, int offset) {
  int depth = 0;

  offset = fdt_next_node(fdt, offset, &depth);
  if (offset < 0 || depth != 1)
    return -FDT_ERR_NOTFOUND;

  return offset;
}
int __attribute__((weak)) fdt_next_subnode(const void* fdt, int offset) {
  int depth = 1;
  do {
    offset = fdt_next_node(fdt, offset, &depth);
    if (offset < 0 || depth < 1)
      return -FDT_ERR_NOTFOUND;
  } while (depth > 1);
  return offset;
}

#ifndef fdt_for_each_subnode
#define fdt_for_each_subnode(node, fdt, parent)          \
  for (node = fdt_first_subnode(fdt, parent); node >= 0; \
       node = fdt_next_subnode(fdt, node))
#endif
