#include "mem_helper.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logging.h"

static const ASD_LogStream stream = ASD_LogStream_JTAG;
static const ASD_LogOption option = ASD_LogOption_None;
int memcpy_safe(void* dest, size_t destsize, const void* src, size_t count)
{
    if (dest == NULL || src == NULL || destsize < count)
        return 1;
    while (count--)
        *(char*)dest++ = *(const char*)src++;
    return 0;
}

int strcpy_safe(char* dest, size_t destsize, const char* src, size_t count)
{
    if (dest == NULL || src == NULL || destsize < count)
        return 1;
    while (count--)
        *(char*)dest++ = *(const char*)src++;
    return 0;
}

int INT_TO_STR_DIGITS_L[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                               '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

int int_to_str(int x, char* buf, size_t size, int base)
{
    int length = (int)ceil(log((double)x) / log((double)10));
    int r, i = 0;
    char c;

    if (size < length)
    {
        x /= (int)pow(10, (float)(length - size));
        length = size;
    }

    do
    {
        if (i >= size)
            break;
        r = x % 10;
        c = INT_TO_STR_DIGITS_L[r];
        buf[length - i - 1] = c;
        x /= base;
        i++;
    } while (x != 0);

    return i;
}

int snprintf_safe(char* str, size_t max_size, const char* fmt, int* values,
                  int number)

{
    int chars_printed = 0;
    int num, len, m = 0;
    int base = 10;

    for (int i = 0; fmt[i] != 0; i++)
    {
        if (max_size - chars_printed <= 0)
        {
            break;
        }
        else if (fmt[i] == '%')
        {
            i++;
            num = values[m];
            if (m < number)
            {
                m++;
            }
            else
            {
                ASD_log(
                    ASD_LogLevel_Error, ASD_LogStream_JTAG, ASD_LogOption_None,
                    "snprintf_safe function failed, number of integer argument needs  \
						to be higher than zero.");
                return 1;
            }

            len = int_to_str(num, str + chars_printed, max_size - chars_printed,
                             base);
            chars_printed += len;
        }
        else
        {
            str[chars_printed++] = fmt[i];
        }
    }

    if (chars_printed == max_size)
        chars_printed--;
    str[chars_printed] = 0;

    return 0;
}
