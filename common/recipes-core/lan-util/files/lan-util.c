/**
 * (C) 2011-2020 Wiwynn Corporation. All Rights Reserved.
 * Jack Hsin <Jack_Hsin@wiwynn.com>
 * Amy Chang <Amy_Chang@wiwynn.com>
 * Tony Ao <tony_ao@wiwynn.com>
 * Matt Chen <matt_chen@wiwynn.com>
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
 *
 * History      :
 *   1. 2018/06/19 Create
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "if_parser.h"
#include "net_lib.h"
#include "strlib.h"

//#define __DEBUG__
#ifdef __DEBUG__
  #define DEBUG(fmt, ...) printf(fmt, ## __VA_ARGS__)
#else
  #define DEBUG(fmt, ...)
#endif

#define IPV4_PREFIX 32
#define IPV6_PREFIX 128

const uint16_t status_list[]=
{
  IPV4_ADD_DHCP_CHECK,
  IPV4_ADD_STATIC_CHECK,
  IPV4_DEL_CHECK,
  IPV6_ADD_DHCP_CHECK,
  IPV6_ADD_STATIC_CHECK,
  IPV6_DEL_CHECK,
  SHOW_GIVEN_IF,
  SHOW_ALL_IF
};

uint8_t status_list_size = sizeof(status_list);

static struct argu
{
  char *param;
  char *desc;
}
argu_info[] =
{
  {"Usage:", ""},
  {"lan-util", "[add|del|show] -i $interface [Options]"},
  {"Actions:", ""},
  {"add", "add a new setting or replace the old one"},
  {"del", "remove the existent interface setting"},
  {"show", "show the all/specific interfaces"},
  {"Options:", ""},
  {"-i", "Inteface"},
  {"-f", "The family with [4|inet|6|inet6]"},
  {"-dhcp", "Set a interface with DHCP"},
  {"-dhcp6", "Set a interface with DHCP6"},
  {"-ip", "Set a static ip to the interface"},
  {"-mask", "Set a mask to the interface"},
  {"-gw", "Set a gateway to the interface"},
  {"-r", "Show the available IP range. The option is valid when the static method is set"},
  {"-run", "Set the data and run the setting now"},
  {"Example:", ""},
  {"DHCP", "lan-util add -i eth0 -dhcp"},
  {"Static", "lan-util add -i eth0 -ip 192.168.88.8 -mask 255.255.255.0 -gw 192.168.88.1"},
  {"Static", "lan-util add -i eth0 -ip 192.168.88.8 -mask 24 -gw 192.168.88.1"},
  {"Static", "lan-util add -i eth0 -ip 192.168.88.8/24 -gw 192.168.88.1"},
  {"Static", "lan-util add -i eth0 -ip 2000::44 -mask 24"},
  {"Static", "lan-util add -i eth0 -ip 2000::44/24"},
  {"VLan"  , "lan-util add -i eth0.100 -ip 192.168.188.8 -mask 255.255.255.0 -gw 192.168.188.1"},
  {"Delete IPv4 interface", "lan-util del -f inet -i eth0"},
  {"Show the specific interface", "lan-util show -i eth0"},
  {"Show the all interfaces", "lan-util show"}
};

static int usage_size = (sizeof(argu_info)/sizeof(struct argu));

void
print_help(void)
{
  int i;
  for ( i=0; i<usage_size; i++ )
  {
    if ( 0 == strcmp("", argu_info[i].desc) )
    {
      if ( i > 0 )
      {
        printf("\n");
      }
      printf("%s\n", argu_info[i].param);
    }
    else
    {
      if ( 0 == strcmp("", argu_info[i].param) )
      {
        printf("%21s\n", argu_info[i].desc);
      }
      else
      {
        printf("%10s : %s\n", argu_info[i].param, argu_info[i].desc);
      }
    }
  }
}

int
is_valid_ip_address(char *ip)
{
  struct sockaddr_in sa;
  int result;

  if ( index_of(ip, ":") != FAIL )
  {
    result = inet_pton(AF_INET6, ip, &(sa.sin_addr));
    return (result != 0)?IPV6:FAIL;
  }
  else
  {
    result = inet_pton(AF_INET, ip, &(sa.sin_addr));
    return (result != 0)?IPV4:FAIL;
  }

  return FAIL;
}

int 
bit_count(uint32_t i)
{
  int c = 0;
  uint8_t is_fully_check = 0;
  uint8_t seen_one = 0;

  while (i > 0)
  {
    if (i & 1)
    {
      seen_one = 1;
      c++;
    }
    else
    {
      if (seen_one)
      {
        return -1;
      }
    }
    i >>= 1;
    is_fully_check++;
  }

  if ( 32 != is_fully_check )
  {
    return -1;
  }

  return c;
}

static const char *p2_table(unsigned pow)
{
  static const char *pow2[] = {
    "1",
    "2",
    "4",
    "8",
    "16",
    "32",
    "64",
    "128",
    "256",
  };

  if (pow <= 8)
  {
    return pow2[pow];
  }

  return "";
}

uint32_t
get_ipv4_mask(int prefix)
{
  int size = sizeof(struct in_addr);
  int i;
  int fixed_len = prefix / 8;
  int bit_op = prefix % 8;
  int offset = strtol(p2_table(8 - bit_op), NULL, 0) - 1;
  uint8_t buff[sizeof(struct in_addr)] = {0};  
  uint32_t mask = 0;

  for ( i=0; i<size; i++ )
  {
    if ( (i == fixed_len) && (bit_op != 0) )
    {
      buff[i] = ~offset;
    }
    else if ( i < fixed_len )
    {
      buff[i] = 0xff;
    }
    else
    {
      buff[i] = 0x0;
    }
  }

  mask |= buff[0] << 24;
  mask |= buff[1] << 16;
  mask |= buff[2] << 8;
  mask |= buff[3];

  DEBUG("[%s]mask:%d\n", __func__, mask);
  return mask;
  
}

int
get_dev_info(char *input, ip_info *ip)
{
  int ret = OK;

  if ( ADD_IF == (ip->flag&ADD_IF) )
  {
    ret = is_valid_if_name(input);
    if ( FAIL == ret )
    {
      return FAIL;
    }
  } 
 
  ip->flag |= IF;
  ip->ip_dev = create_buff(SIZE);
  memcpy(ip->ip_dev, input, strlen(input));

  return OK;
}

int
is_valid_ip_range(struct in_addr in_ip, struct in_addr in_mask, struct in_addr in_gw, uint16_t flag)
{
  uint32_t mask_val = ntohl(in_mask.s_addr);
  uint32_t ip_val = ntohl(in_ip.s_addr);
  uint32_t min = 0;
  uint32_t max = 0;
  uint8_t min_buf[INET_ADDRSTRLEN] = {0}, max_buf[INET_ADDRSTRLEN] = {0};

  if ( in_ip.s_addr == in_gw.s_addr )
  {
    printf("IP and Gateway are the same!\n");
    return FAIL;
  }

  min = (ip_val & mask_val) + 1;
  max = htonl(min + (~mask_val) - 2);
  min = htonl(min);
  ip_val = htonl(ip_val);

  if ( NULL == inet_ntop(AF_INET, (void*)&min, min_buf, sizeof(min_buf)) )
  {
    printf("Get host min fail\n");
    return FAIL;
  }

  if ( NULL == inet_ntop(AF_INET, (void*)&max, max_buf, sizeof(max_buf)) )
  {
    printf("Get host max fail\n");
    return FAIL;
  }

  if ( ((in_mask.s_addr & in_ip.s_addr) != (in_mask.s_addr & in_gw.s_addr)) ||
       ((in_mask.s_addr & in_ip.s_addr) == (in_gw.s_addr)) ||
       ((~in_mask.s_addr | in_ip.s_addr) == (in_gw.s_addr)) )
  {
    printf("Incorrect gateway is set!\n");
    printf("The gateway should between %s and %s\n", min_buf, max_buf);

    return FAIL;
  }
  else if ( (ip_val < min) || (ip_val > max) )
  {
    printf("Incorrect IP is set!\n");
    printf("The IP should between %s and %s\n", min_buf, max_buf);
    return FAIL;
  }

  if ( (flag & SHOW_RANGE) == (SHOW_RANGE) )
  {
    printf("HostMin: %s\n", min_buf);
    printf("HostMax: %s\n", max_buf);
  }

  return OK;
}

int
is_valid_ipv4_setting(ip_info *ip)
{
  struct in_addr in_mask, in_gw, in_ip;
  int ret;

  //check IP format
  ret = inet_pton(AF_INET, ip->ip_ip, &in_ip);
  if ( 0 == ret )
  {
    printf("Incorrect IPv4 address!\n");
    return FAIL;
  }

  if ( in_ip.s_addr == 0 )
  {
    printf("All zeros cannot be accepted!\n");
    return FAIL;
  }

  //is it the prefix?
  ret = is_number(ip->ip_mask);
  if ( FAIL != ret )
  {
    //convert it to aa.bb.cc.dd format
    int prefix = ret;
    if ( prefix >= IPV4_PREFIX || prefix < 1 )
    {
      printf("IPv4 prefix need to between 1 and 31!\n");
      return FAIL;
    }
    //get the IP format
    prefix = htonl(get_ipv4_mask(prefix));
    memset(ip->ip_mask, 0, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, (void*)&prefix, ip->ip_mask, INET_ADDRSTRLEN);
  }
  
  DEBUG("[%s]%s\n", __func__, ip->ip_mask);
  //get the in_addr format
  ret = inet_pton(AF_INET, ip->ip_mask, &in_mask);
  if ( 0 == ret )
  {
    printf("Incorrect Mask address format!\n");
    return FAIL;
  }
 
  ret = bit_count(ntohl(in_mask.s_addr));
  if ( ret < 0 )
  {
    printf("Incorrect bit value in Mask address!\n");
    return FAIL;
  }
  
  ret = inet_pton(AF_INET, ip->ip_gw, &in_gw);
  if ( 0 == ret )
  {
    printf("Incorrect Gateway address!\n");
    return FAIL;
  } 

  ret = is_valid_ip_range(in_ip, in_mask, in_gw, ip->flag);
  if ( FAIL == ret )  
  {
    return FAIL;
  }

  sprintf(ip->key, "%s_4", ip->ip_dev);

  return OK;
}

void
show_ipv6_range(struct in6_addr *ip, int prefix)
{
  struct in6_addr mask;
  int size = sizeof(struct in6_addr);
  int i;
  int fixed_len = prefix / 8;
  int bit_op = prefix % 8;
  int offset = strtol(p2_table(8 - bit_op), NULL, 0) - 1;
  char buf[INET6_ADDRSTRLEN] = {0};

  //get mask
  memcpy(mask.s6_addr, ip->s6_addr, fixed_len);

  for ( i=fixed_len; i<size; i++ )
  {
    if ( i == fixed_len )
    {
      mask.s6_addr[i] = offset;
    }
    else
    {
      mask.s6_addr[i] = 0xff;
    }
  }

  //show host min
  printf("HostMin: ");
  for ( i=0; i<size; i++ )
  {
    buf[i] =  (~mask.s6_addr[i]);

    if ( i == fixed_len )
    {
      buf[i] = (buf[i] & ip->s6_addr[i]);
    }
    else if ( i < fixed_len )
    {
      buf[i] =  (~buf[i]);
    }

    if ( (0 == (i % 2)) && (0 != i) )
    {
      printf(":");
    }
    printf("%02X", (uint8_t)buf[i]);
  }
  printf("\n");

  //show host max
  memset(buf, 0, sizeof(buf));
  printf("HostMax: ");
  for ( i=0; i<size; i++ )
  {
    buf[i] = (ip->s6_addr[i] | mask.s6_addr[i]);
    if ( (0 == (i % 2)) && (0 != i) )
    {
      printf(":");
    }

    printf("%02X", (uint8_t)buf[i]);
  }
  printf("\n");
}

int
is_valid_ipv6_format(char *s, int size)
{
  int i;
  int cnt = 0;

  for (i=0; i<size; i++)
  {
    cnt++;

    if ( s[i] == ':' )
    {
      cnt = 0;
      continue;
    }
    else if ( cnt >= 5 )
    {
      return FAIL;
    }
  }

  return OK;
}

int
is_valid_ipv6_setting(ip_info *ip)
{
  struct in6_addr in_ip;
  int ret;
  int prefix = 0;
  int i;
  int size = sizeof(struct in6_addr);
  uint8_t zero_chk = 0;

  ret = inet_pton(AF_INET6, ip->ip_ip, &in_ip);
  if ( 0 == ret )
  {
    printf("Incorrect IPv6 format!\n");
    return FAIL;
  }

  for ( i=0; i<size; i++ )
  {
    zero_chk |= in_ip.s6_addr[i];
  }

  if ( 0 == zero_chk )
  {
    printf("All zeros cannot be accepted!\n");
    return FAIL;    
  }

  ret = is_valid_ipv6_format(ip->ip_ip, strlen(ip->ip_ip));
  if ( FAIL == ret )
  {
    printf("Incorrect IPv6 length!\n");
    return FAIL;  
  }
 
  //is it the prefix?
  ret = is_number(ip->ip_mask);
  if ( FAIL != ret )
  {
    prefix = ret;
    if ( prefix >= IPV6_PREFIX || prefix < 1 )
    {
      printf("IPv6 prefix need to between 1 and 127!\n");
      return FAIL;
    }
  }
  else
  {
    printf("Incorrect prefix!\n");
    return FAIL;
  }

  if ( (ip->flag & SHOW_RANGE) == (SHOW_RANGE) )
  {
    show_ipv6_range(&in_ip, prefix);
  }

  sprintf(ip->key, "%s_6", ip->ip_dev);

  return OK;
}

int
verify_param(ip_info *ip)
{
  int ret = OK;
  uint16_t flag = (~(RUN_IF|SHOW_RANGE)) & ip->flag;

  switch (flag)
  {
    case IPV4_ADD_DHCP_CHECK:
    case IPV6_ADD_DHCP_CHECK:
      sprintf(ip->key, "%s_%d", ip->ip_dev, (flag == IPV4_ADD_DHCP_CHECK)?4:6);
      break;
    
    case IPV4_ADD_STATIC_CHECK:
      ret = is_valid_ipv4_setting(ip);
      break;

    case IPV6_ADD_STATIC_CHECK:
      ret = is_valid_ipv6_setting(ip);
      break;

    case IPV4_DEL_CHECK:
    case IPV6_DEL_CHECK:
      sprintf(ip->key, "%s_%d", ip->ip_dev,(flag == IPV4_DEL_CHECK)?4:6);
      break;

    case SHOW_GIVEN_IF:
    case SHOW_ALL_IF:
      print_conf(ip->ip_dev);
      break;

    default:
      ret = FAIL;
      printf("Unknown flag is set!\n");
      break;
  }

  return ret;
}

int
run_setting(ip_info *ip)
{
  uint16_t flag = (~(RUN_IF|SHOW_RANGE)) & ip->flag;
  const char *set_ip = "ip %s addr add %s/%s dev %s";
  const char *reinit_net = "/etc/init.d/setup-init_net_interface.sh restart";
  int ret;
  char cmd[128]={0};

  DEBUG("[%s]cmd:%s", __func__, reinit_net);
  ret = system(reinit_net);
 
  switch (flag)
  {
    case IPV4_ADD_STATIC_CHECK:
      sprintf(cmd, set_ip, "", ip->ip_ip, ip->ip_mask, ip->ip_dev); 
      DEBUG("[%s]cmd:%s", __func__, cmd);
      ret = system(cmd);

      const char *set_gw = "ip route add default via %s dev %s";
      sprintf(cmd, set_gw, ip->ip_gw, ip->ip_dev);
      DEBUG("[%s]cmd:%s", __func__, cmd);
      ret = system(cmd);
      break;

    case IPV6_ADD_STATIC_CHECK:
      sprintf(cmd, set_ip, "-6", ip->ip_ip, ip->ip_mask, ip->ip_dev);
      DEBUG("[%s]cmd:%s", __func__, cmd);
      ret = system(cmd);
      break;
  }

  return OK;
}

int
read_param(char **input_data, int input_len, ip_info *ip)
{
  int ret = 0;
  int i;
  uint8_t curr_flag = 0;
  //bool show_range = false;

  for (i=0; i<input_len; i++)
  {
    if ( 0 == strcmp("-i", input_data[i]) )
    {
      curr_flag = IF;
      DEBUG("Get IF: %s, FLAG:%x\n", input_data[i], curr_flag);
    }
    else if ( 0 == strcmp("-ip", input_data[i]) )
    {      
      curr_flag = IPV4 | IPV6;
      DEBUG("Get IP: %s, FLAG:%x\n", input_data[i], curr_flag); 
    }
    else if ( 0 == strcmp("-mask", input_data[i]) )
    {
      curr_flag = MASK;
      DEBUG("Get MASK: %s, FLAG:%x\n", input_data[i], curr_flag); 
    }
    else if ( 0 == strcmp("-gw", input_data[i]) )
    {
      curr_flag = GATEWAY;
      DEBUG("Get GATEWAY: %s, FLAG:%x\n", input_data[i], curr_flag);
    }
    else if ( 0 == strcmp("-dhcp", input_data[i]) )
    {
      ip->flag |= DHCP;
      DEBUG("Get DHCP: %s, FLAG:%x\n", input_data[i], ip->flag);
    }
    else if ( 0 == strcmp("-f", input_data[i]) )
    {
      curr_flag = NET_FAMILY;
      DEBUG("Get Family: %s, FLAG:%x\n", input_data[i], ip->flag);
    }
    else if ( 0 == strcmp("-dhcp6", input_data[i]) )
    {
      ip->flag |= DHCP6;
      DEBUG("Get DHCP6: %s, FLAG:%x\n", input_data[i], ip->flag);
    }
    else if ( 0 == strcmp("-r", input_data[i]) )
    {
      ip->flag |= SHOW_RANGE;
      DEBUG("Get SHOW RANGE: %s, FLAG:%x\n", input_data[i], ip->flag);
    }
    else if ( 0 == strcmp("-run", input_data[i]) )
    {
      ip->flag |= RUN_IF;
      DEBUG("Get RUN_IF: %s, FLAG:%x\n", input_data[i], ip->flag);
    }
    else if ( IF == curr_flag )
    {
      ret = get_dev_info(input_data[i], ip);
      if ( FAIL == ret )
      {
        goto exit;
      }
      curr_flag = 0;
    }
    else if ( (IPV4 | IPV6) == curr_flag )
    {
      //search for the split symbol
      ret = index_of(input_data[i], "/");
      ip->ip_ip = create_buff(SIZE);
      if ( FAIL != ret )
      {
        if ( MASK == (ip->flag&MASK) )
        {
          printf("Duplicate mask is set!\n");
          ret = FAIL;
          goto exit;
        }

        //get IP and mask based on the index of '\'
        int index = ret;
        ip->ip_mask = create_buff(SIZE);
        memcpy(ip->ip_ip, input_data[i], index);
        memcpy(ip->ip_mask, &input_data[i][index+1], strlen(input_data[i])-index);
        ip->flag |= MASK;
      }
      else
      {
        memcpy(ip->ip_ip, input_data[i], strlen(input_data[i]));
      }

      //check IP
      ret = is_valid_ip_address(ip->ip_ip);
      if ( FAIL != ret )
      {
        ip->flag |= (IPV4 == ret)?IPV4:IPV6;
      }
      else
      {
        printf("Incorrect IP is set!\n");
        ret = FAIL;
        goto exit;
      }
      DEBUG("[%s]GetIP:%s, GetMask:%s, Flag:IPv%d\n", __func__, ip->ip_ip, (NULL == ip->ip_mask)?"not set":ip->ip_mask, (ret==IPV4)?4:6); 
      curr_flag = 0;
    }
    else if ( MASK == curr_flag )
    {
      if ( MASK == (ip->flag&MASK) )
      {
        printf("Duplicate mask is set!\n");
        ret = FAIL;
        goto exit;
      }
      
      ip->ip_mask = create_buff(SIZE);
      memcpy(ip->ip_mask, input_data[i], strlen(input_data[i]));
      ip->flag |= MASK;
      curr_flag = 0;
    }
    else if ( GATEWAY == curr_flag )
    {
      ip->ip_gw = create_buff(SIZE);
      memcpy(ip->ip_gw, input_data[i], strlen(input_data[i]));
      ip->flag |= GATEWAY;
      curr_flag = 0;
    }
    else if ( NET_FAMILY == curr_flag )
    {
      if ( (0 == strcmp("6", input_data[i])) || (0 == strcmp("inet6", input_data[i])) )
      {
        ip->flag |= IPV6;
      }
      else if ( (0 == strcmp("4", input_data[i])) || (0 == strcmp("inet", input_data[i])) )
      {
        ip->flag |= IPV4;
      }
      else
      {
        printf("Incorrect net family is set!\n");
        ret = FAIL;
        goto exit;
      }
      curr_flag = 0;
    }
    else
    {
      printf("Cannot parse the options, or \"%s\" is a garbage\n", input_data[i]);
      ret = FAIL;
      goto exit;
    }
  }

  uint8_t setting_cnt = 0;
  uint16_t flag_check = (~(SHOW_RANGE | RUN_IF)) & ip->flag;
  for ( i=0; i<status_list_size; i++ )
  {
    DEBUG("[%s]ip flag:%x ip->flag & status_list[i]:%x, flag:%x\n", __func__, ip->flag, (ip->flag & status_list[i]), (status_list[i]));
    if ( (flag_check ^ status_list[i]) == 0 )
    {
      setting_cnt++;
    }     
  }  

  DEBUG("[%s]settings?%d\n", __func__, setting_cnt); 
  if ( 0 == setting_cnt )
  {
    printf("Incorrect setting!\n");
    ret = FAIL;
    goto exit;
  }

  ret = OK;

exit:
  return ret;
}

int
main(int argc, char **argv)
{
  ip_info ip_setting;
  int ret = OK;
  int i;

  memset(&ip_setting, 0, sizeof(ip_info));

  if ( (argc == 1) || 0 == strcmp(argv[1], "--help") || 0 == strcmp(argv[1], "help") )
  {
    print_help();
    goto exit;
  }

  if ( 0 == strcmp(argv[1], "add") )
  {
    ip_setting.flag |= ADD_IF;
  }
  else if ( 0 == strcmp(argv[1], "del") )
  {
    ip_setting.flag |= DEL_IF;
  }
  else if ( 0 == strcmp(argv[1], "show") )
  {
    ip_setting.flag |= SHOW_IF;
  }
  else
  {
    printf("Unknown action is entered!\n");
    goto exit;
  }

  //init the interface 
  ret = get_conf();
  if ( FAIL == ret )
  {
    goto exit;
  } 
  
  //get the parameters
  ret = read_param(&argv[2], argc-2, &ip_setting);
  if ( FAIL == ret )
  {
    goto exit;
  }

  //verify the parameters
  ret = verify_param(&ip_setting);
  if ( FAIL == ret )
  {
    goto exit;
  }

  if ( ADD_IF == (ip_setting.flag&ADD_IF) )
  {
    //write data to conf
    ret = write_data_to_conf(&ip_setting);
    if ( FAIL == ret )
    {
      goto exit;
    }
    else
    {
      printf("Add the setting successfully!\n");
      if ( NULL != ip_setting.ip_dev )  printf("Interface : %-20s\n", ip_setting.ip_dev);
      if ( (IPV4_ADD_DHCP_CHECK == (ip_setting.flag&IPV4_ADD_DHCP_CHECK)) ||
           (IPV6_ADD_DHCP_CHECK == (ip_setting.flag&IPV6_ADD_DHCP_CHECK)) )
      {
        printf("   Method : DHCP\n");
      }
      else 
      {
        printf("   Method : Static\n");
      }
      if ( NULL != ip_setting.ip_ip )   printf("       IP : %-20s\n", ip_setting.ip_ip);
      if ( NULL != ip_setting.ip_mask ) printf("     Mask : %-20s\n", ip_setting.ip_mask);
      if ( NULL != ip_setting.ip_gw )   printf("  Gateway : %-20s\n", ip_setting.ip_gw);
    }
  }
  else if ( DEL_IF == (ip_setting.flag&DEL_IF) )
  {
    ret = delete_data_from_conf(ip_setting.key);
    if ( FAIL == ret )
    {
      printf("Cannot delete the non-existent setting\n");      
      goto exit;
    }
    else
    {
      printf("Delete the setting successfully!\n");
    }
  }

  //run the setting now
  if ( RUN_IF == (RUN_IF & ip_setting.flag) )
  {
    printf("Run the setting...\n");
    ret = run_setting(&ip_setting);
  }
  
  DEBUG("Interface : %-20s\n", (NULL == ip_setting.ip_dev)?"not set":ip_setting.ip_dev);
  DEBUG("       IP : %-20s\n", (NULL == ip_setting.ip_ip)?"not set":ip_setting.ip_ip);
  DEBUG("     Mask : %-20s\n", (NULL == ip_setting.ip_mask)?"not set":ip_setting.ip_mask);
  DEBUG("  Gateway : %-20s\n", (NULL == ip_setting.ip_gw)?"not set":ip_setting.ip_gw);

exit:
  
  if ( NULL != ip_setting.ip_dev )
  {
    free(ip_setting.ip_dev);
  }

  if ( NULL != ip_setting.ip_mask )
  {
    free(ip_setting.ip_mask);
  }

  if ( NULL != ip_setting.ip_ip )
  {
    free(ip_setting.ip_ip);
  }

  if ( NULL != ip_setting.ip_gw )
  {
    free(ip_setting.ip_gw);
  }

  free_all();
  return 0;
}

