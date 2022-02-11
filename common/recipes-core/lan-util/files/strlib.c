#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "strlib.h"

//#define __DEBUG__
#ifdef __DEBUG__
  #define DEBUG(fmt, ...) printf(fmt, ## __VA_ARGS__)
#else
  #define DEBUG(fmt, ...)
#endif

int
index_of(const char *str, const char *c)
{
  char *ptr = strstr(str, c);
  int  index = 0;

  if ( ptr )
  {
    index = ptr - str;
  }
  else
  {
    index = -1;
  }

  return index;
}

int
from_index_of(int from_index, const char *str, const char *c)
{
  int index = 0;
  char *ptr = strstr(&str[from_index], c);

  if ( ptr )
  {
    index = ptr - str;
  }
  else
  {
    index = -1;
  }

  return index;
}

int
end_with(const char *str, const char *c)
{
  int len = strlen(c);
  int i = strlen(str)-1;
  int j = 0;

  for ( ; i>=0 && j<len; i--,j++ )
  {
    DEBUG("[%s] str:%c c:%c", __func__, str[i], c[j]);
    if ( str[i] != c[j] )
    {
       return -1;
    }
  }
  return 0;
}

int
start_with(const char *str, const char *c)
{
  int len = strlen(c);
  int i;

  for ( i=0; i<len; i++ )
  {
    if ( str[i] != c[i] )
    {
       return -1;
    }
  }

  return 0;
}

int
trim(char *dst, char *str)
{
  int start = 0;
  int end = strlen(str);

  if ( 0 == end )
  {
    return 0;
  }

  if ( (1 == end) && (0 != isspace(str[start])) ) return 0;

  while ( 0 != isspace(str[start]) ) start++;

  while ( 0 != isspace(str[end]) || '\0' == str[end] ) end--;

  memcpy(dst, &str[start], (end-start)+1);

  DEBUG("[%s]'%s' start:%d,'%c' end:%d,'%c'\n", __func__, dst, start, str[start], end, str[end]);

  return 0;
}

int
split(char **dst, char *src, char *delim)
{
  char *s = strtok(src, delim);
  int size = 0;

  while ( NULL != s )
  {
     *dst++ = s;
     size++;
     s = strtok(NULL, delim);
  }

  return size;
}

int
is_number(const char *p)
{
  char *endptr = NULL;
  long int result = strtol(p, &endptr, 0);

  return ('\0' == *endptr)?result:-1;
}
