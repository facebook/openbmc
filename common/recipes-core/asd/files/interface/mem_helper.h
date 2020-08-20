#ifndef AT_SCALE_DEBUG_MEM_HELPER_H
#define AT_SCALE_DEBUG_MEM_HELPER_H

#include <stddef.h>
int memcpy_safe(void* dest, size_t destsize, const void* src, size_t count);
int strcpy_safe(char* dest, size_t destsize, const char* src, size_t count);
int snprintf_safe(char* str, size_t max_size, const char* fmt, int* values,
                  int number);

#endif // AT_SCALE_DEBUG_MEM_HELPER_H
