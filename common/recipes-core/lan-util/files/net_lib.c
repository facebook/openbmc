#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include "net_lib.h"
#include "strlib.h"

//#define __DEBUG__
#ifdef __DEBUG__
  #define DEBUG(fmt, ...) printf(fmt, ## __VA_ARGS__)
#else
  #define DEBUG(fmt, ...)
#endif

#define BYTE_MASK 0xFF
#define BYTE1_OFFSET 8
#define BYTE2_OFFSET 16
#define BYTE3_OFFSET 24

int
is_valid_if_name(char *if_name)
{
  struct ifaddrs *ifaddr, *ifa;
  bool is_valid_if = false, is_vlan_exist = false;
  char *temp = NULL;
  int len = 0, index = 0, size = 0;

  if ( getifaddrs(&ifaddr) == -1 )
  {
    printf("Cannot get interface info!\n");
    return -1;
  }

  for ( ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next )
  {
    //check the eth by AF_PACKET
    if ( ifa->ifa_addr->sa_family == AF_PACKET )
    {
      if ( 0 == start_with(if_name, ifa->ifa_name) )
      {
        is_valid_if = true;
      }

      if ( index_of(ifa->ifa_name, ".") > 0 ) 
      {
        is_vlan_exist = true;
      }           
    }
  }
  freeifaddrs(ifaddr);

  if ( false == is_valid_if )
  {
    printf("Invalid interface - %s\n", if_name);
    return -1;
  }

  index = index_of(if_name, ".");
  if ( index > 0 )
  {
    len = strlen(if_name) - index - 1;
    size = len * sizeof(char);
    temp = (char*) malloc(size); 
    memset(temp, 0, size);
    memcpy(temp, &if_name[index+1], len);

    DEBUG("[%s] if_name:%s, vlan:%s, len:%d, index:%d\n", __func__, if_name, temp, len, index);
    if ( -1 == is_number(temp) ) 
    {
      printf("vlan tag must be a number!\n");
      free(temp);
      return -1;
    }

    if ( true == is_vlan_exist )
    {
      printf("vlan is already set!\n");
      free(temp);
      return -1;
    }

    free(temp);
  }

  return 0;
}
