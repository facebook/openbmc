#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <math.h>
#include <ctype.h>
#include "if_parser.h"
#include "strlib.h"

//#define __DEBUG__
#ifdef __DEBUG__
  #define DEBUG(fmt, ...) printf(fmt, ## __VA_ARGS__)
#else
  #define DEBUG(fmt, ...)
#endif

#define IF_ADDR "/mnt/data/etc/network/interfaces"

char init_node[] = "INIT";

if_list *if_head = NULL;
if_list *if_curr = NULL;

if_list* 
create_node(char *key)
{
  if ( (NULL != if_curr) && (0 == strcmp(if_curr->key, init_node)) )
  {
    //modify the key of init node from the default name to the real net interface 
    memset(if_curr->key, 0, sizeof(if_curr->key));
    memcpy(if_curr->key, key, strlen(key));
    return if_curr;
  }
  else if ( (NULL != if_curr) && (0 == strcmp(if_curr->key, key)) )
  {
    //avoid the duplicate key node
    DEBUG("Duplicate key node!\n");
    return if_curr;
  }

  if_list *node = malloc(sizeof(if_list));
  memset(node, 0, sizeof(if_list));
  memcpy(node->key, key, strlen(key));

  DEBUG("Create:%s node\n", node->key);
  if ( NULL == if_head )
  {
    if_head = node;
    if_curr = node;
  }
  else
  {
    if_curr->next = node;
    if_curr = node;
  }
 
  return if_curr;
}

void
free_node(if_list *node)
{
  int i;
  for ( i=0; i<node->config_size; i++ )
  {
    DEBUG("[%s]free:%s\n", __func__, node->key);
    if ( NULL != node->config.content[i] ) 
    {
      DEBUG("[%s] free ptr :%d/%d\n", __func__, i, node->config_size);
      free(node->config.content[i]);
    }
  }

  if ( NULL != node ) 
  {
    DEBUG("[%s]done\n", __func__);
    free(node);
  }
}

void
free_all()
{
  if_list *node = if_head;
  if_list *tmp = NULL;

  while ( NULL != node )
  {
    tmp = node;
    node = node->next;
    free_node(tmp);
  }
}

if_list* 
search_node(char *key)
{
  if_list* node = if_head;
  
  while( NULL != node )
  {
    if ( 0 == strcmp(node->key, key) )
    {
      DEBUG("Found:%s\n", key);
      return node;
    }
    node = node->next;
  }
  
  return NULL; 
}

int
get_opposite_dev_name(char *new_key, char *key)
{
  int ret;
  char if_family = 0;
 
  ret = index_of(key, "6");
  if ( FAIL != ret )
  {
    if_family = '6';
    memcpy(new_key, key, ret);
    sprintf(new_key, "%s%d", new_key, 4);
  }
  else
  {
    ret = index_of(key, "4");
    if ( FAIL != ret )
    {
      if_family = '4';
      memcpy(new_key, key, ret);
      sprintf(new_key, "%s%d", new_key, 6);
    }
  }
 
  if ( FAIL == ret )
  {
    return FAIL;
  }

  return OK;
}

void 
replace_node_content(if_list *replaced_node, if_list *new_node)
{
  int i = 0, j = 0;
  int new_size = new_node->config_size;
  int replaced_size = replaced_node->config_size;

  DEBUG("[%s] r_size:%d n_size:%d\n", __func__, replaced_size, new_size);

  if ( YES == start_with(replaced_node->config.content[i], "auto") )
  {
    i++;
  }

  while ( NULL != new_node->config.content[j] )
  {
    if ( i < replaced_size )
    {
      memset(replaced_node->config.content[i], 0, SIZE);
    }
    else
    {
      replaced_node->config.content[i] = create_buff(SIZE);
      memset(replaced_node->config.content[i], 0, SIZE);
    }

    DEBUG("[%s]i:%d, j:%d cp:%s\n", __func__, i, j, new_node->config.content[j]);
    memcpy(replaced_node->config.content[i], new_node->config.content[j], strlen(new_node->config.content[j]));
    i++;
    j++;
  }

  replaced_node->config_size = i;
  
  while ( NULL != replaced_node->config.content[i] )
  {
    free(replaced_node->config.content[i]);
    i++;
  }
  
}

void 
copy_node(if_list *dst_node, if_list *src_node)
{
  int i, j;
  int size = src_node->config_size;

  memcpy(dst_node->key, src_node->key, strlen(src_node->key));
  dst_node->config_size = size;

  for ( i=1,j=0; i<size; i++,j++ )
  {
    dst_node->config.content[j] = create_buff(SIZE);
    memset(dst_node->config.content[j], 0, SIZE);
    memcpy(dst_node->config.content[j], src_node->config.content[i], strlen(src_node->config.content[i]));
  }
}

int
delete_node(char *key)
{
  int ret = 0;
  
  if_list *tmp = search_node(key);
  if ( NULL == tmp )
  {
    DEBUG("[%s] the node is not exist!\n", __func__);
    return FAIL;
  }

  //is it the first node?
  if ( tmp == if_head )
  {
    if_head = tmp->next;
  } 
  else
  {
    if_list *prev = if_head;
  
    //wade through the list
    while ( NULL != prev )
    {
      if ( prev->next == tmp )
      {
        prev->next = tmp->next;
        break;
      }
      prev = prev->next;
    } 
  }

  free_node(tmp);
  
  return OK;
}

int delete_data_from_conf(char *key)
{
  int ret = 0;
  char temp_key[16] = {0};
  char *temp_str = NULL;
  int i;
  int opposite_node_size = 0;
  if_list *opposite_node = NULL;
  char *opposite_temp_str = NULL;
  bool is_rm = false;

  ret = get_opposite_dev_name(temp_key, key);
  if ( FAIL != ret )
  {
    opposite_node = search_node(temp_key);
    if ( NULL != opposite_node )
    {
      if ( YES == end_with(temp_key, "6") )
      {
        for ( i=0; i<opposite_node->config_size; i++ )      
        {
          if ( index_of(opposite_node->config.content[i], "dhcp") > 0 )
          {
            is_rm = true;
            break;
          } 
        } 
      }

      if ( true == is_rm )
      {
        ret = delete_node(temp_key);
      }
      else
      {
        opposite_node_size = opposite_node->config_size;
        if ( NO == start_with(opposite_node->config.content[0], "auto") )
        {
          //add auto
          memset(temp_key, 0, sizeof(temp_key));
          memcpy(temp_key, key, index_of(key, "_"));
          DEBUG("[%s]Get:%s\n", __func__, temp_key);

          opposite_temp_str = create_buff(SIZE);
          sprintf(opposite_temp_str, "auto %s", temp_key);

          for ( i=opposite_node_size; i>=1; i-- )
          {
            opposite_node->config.content[i] = opposite_node->config.content[i-1];
          }
          opposite_node->config.content[0] = opposite_temp_str;
          opposite_node->config_size++;
        }
      }
    }
  }

  ret = delete_node(key);
  if ( FAIL == ret )
  {
    return FAIL;
  }

  ret = update_data_to_conf();
  if ( FAIL == ret )
  {
    printf("[%s]Cannot write data to conf", __func__);
    return FAIL;
  }

  return ret;
}

char*
create_buff(int size)
{
  int buff_size = size*sizeof(char);
  char *p = (char*) malloc(buff_size);

  if ( NULL == p )
  {
    printf("Cannot allocate buff!\n");
    return NULL;
  }

  memset(p, 0, buff_size);
  return p;
}

int
convert_data_to_conf_format(ip_info *ip)
{
  bool is_replaced = false, is_exist_same_if = false;
  char temp_key[16]={0};
  int ret;
  int index;
  if_list *temp_node = NULL, *node = NULL, *new_node = NULL;

  //create a node 
  new_node = malloc(sizeof(if_list));
  memset(new_node, 0, sizeof(if_list));
 
  DEBUG("[%s]input_key:%s\n", __func__, ip->key); 
  //there is the key in the conf
  node = search_node(ip->key);
  if ( NULL != node )
  {
    is_replaced = true;
  }
  else
  {
    //search for the if_family of dev exist or not
    ret = get_opposite_dev_name(temp_key, ip->key);
    if ( FAIL == ret )
    {
      return FAIL;
    }

    DEBUG("[%s]temp_key:%s\n", __func__, temp_key);

    temp_node = search_node(temp_key);
    //there is no if_family in the conf
    if ( NULL == temp_node )
    {
      //dhcp6 setting is depends on IPv4 setting or the interface cannot be initialized
      //if user want to use dhcp6 but there is no any IPv4 setting in conf, give it the default setting - dhcp
      if ( DHCP6 == (DHCP6&ip->flag) )
      {
        temp_node = malloc(sizeof(if_list));
        memset(temp_node, 0, sizeof(if_list));
        sprintf(new_node->key, "%s", temp_key);
        DEBUG("[%s]Get new temp key:%s\n", __func__, temp_key);
        temp_node->config.content[temp_node->config_size] = create_buff(SIZE);
        sprintf(temp_node->config.content[temp_node->config_size++],"auto %s", ip->ip_dev);

        temp_node->config.content[temp_node->config_size] = create_buff(SIZE);
        sprintf(temp_node->config.content[temp_node->config_size++],"iface %s inet dhcp", ip->ip_dev);

        if_curr->next = temp_node;
        if_curr = temp_node;
      }
      else
      {
        //create the new key name
        sprintf(new_node->key, "%s", ip->key);
        new_node->config.content[new_node->config_size] = create_buff(SIZE);
        sprintf(new_node->config.content[new_node->config_size++],"auto %s", ip->ip_dev);
      }
    }
    else
    {
      is_exist_same_if = true;
    }
  }

  DEBUG("[%s] is_replaced?%d is_exist_same_if?%d\n", __func__, is_replaced, is_exist_same_if);
  if ( ((DHCP&ip->flag) == DHCP) )
  {
    new_node->config.content[new_node->config_size] = create_buff(SIZE);
    sprintf(new_node->config.content[new_node->config_size++],"iface %s inet dhcp", ip->ip_dev);
  }
  else if ( ((DHCP6&ip->flag) == DHCP6) )
  {
    new_node->config.content[new_node->config_size] = create_buff(SIZE);
    sprintf(new_node->config.content[new_node->config_size++],"iface %s inet6 dhcp", ip->ip_dev);
  }
  else
  {
    new_node->config.content[new_node->config_size] = create_buff(SIZE);
    sprintf(new_node->config.content[new_node->config_size++],"iface %s inet%s static", ip->ip_dev, ((IPV4&ip->flag) == IPV4)?"":"6");
    if ( NULL != ip->ip_ip ) 
    {
      new_node->config.content[new_node->config_size] = create_buff(SIZE);
      sprintf(new_node->config.content[new_node->config_size++],"address %s", ip->ip_ip);
    }
    
    if ( NULL != ip->ip_mask ) 
    {
      new_node->config.content[new_node->config_size] = create_buff(SIZE);
      sprintf(new_node->config.content[new_node->config_size++],"netmask %s", ip->ip_mask);
    }

    if ( NULL != ip->ip_gw ) 
    {
      new_node->config.content[new_node->config_size] = create_buff(SIZE);
      sprintf(new_node->config.content[new_node->config_size++],"gateway %s", ip->ip_gw);
    }
  } 

#ifdef __DEBUG__
  int i;
  for (i=0;i<new_node->config_size;i++)
    printf("[%s]%s\n", __func__, new_node->config.content[i]);
#endif  
 
  if ( true == is_replaced )
  {
    replace_node_content(node, new_node);
    DEBUG("[%s]replaced done\n", __func__);
    free_node(new_node);
  }
  else
  {
    if ( false == is_exist_same_if )
    {
      if_curr->next = new_node;
      if_curr = new_node;
    }
    else
    {
      if_list *next_node = NULL;
      if_list *swap_node = NULL;

      if ( YES == end_with(temp_node->key, "6") )
      {
        swap_node = malloc(sizeof(if_list));
        memset(swap_node, 0, sizeof(if_list));
        copy_node(swap_node, temp_node);

        memset(temp_node->key, 0, strlen(temp_node->key));
        memcpy(temp_node->key, new_node->key, strlen(new_node->key));
        replace_node_content(temp_node, new_node);

        memset(new_node->key, 0, strlen(new_node->key));
        memcpy(new_node->key, swap_node->key, strlen(swap_node->key));
        replace_node_content(new_node, swap_node);
        free(swap_node);
      }

      next_node = temp_node->next;
      temp_node->next = new_node;
      new_node->next = next_node;
    }
  }
  
  return OK;
}

int
update_data_to_conf()
{
  FILE *fp;
  int i;
  char stream[SIZE]={0};

  fp = fopen(IF_ADDR, "w");
  if ( fp == NULL )
  {
    printf("Cannot open the file:%s\n", IF_ADDR);
    return FAIL;
  }

  if_list *node = if_head;

  while ( NULL != node )
  {
    for ( i=0; i<node->config_size; i++ )
    {
      if ( YES == start_with(node->config.content[i], "auto") ||
           YES == start_with(node->config.content[i], "iface"))
      {
        sprintf(stream, "%s\n", node->config.content[i]);
      }
      else
      {
        sprintf(stream, "    %s\n", node->config.content[i]);
      }
      fputs(stream, fp);
    }

    fputs("\n", fp);
    node = node->next;
  }

  fclose(fp);

  return OK;
}

int
write_data_to_conf(ip_info *ip)
{
  int ret;

  ret = convert_data_to_conf_format(ip);
  if ( FAIL == ret )
  {
    printf("Cannot convert data to conf format!\n");
    return FAIL;
  }

  ret = update_data_to_conf();
  if ( FAIL == ret )
  {
    printf("Cannot update data to conf!\n");
    return FAIL;
  }

  return OK;
}

void
print_conf(char *key)
{
  if_list *node = if_head;
  int i, j;
  bool is_valid_key = false;
  char dev[16]={0};
  char temp_str[SIZE]={0};
  int temp_index;
  char *token_list[4] = {NULL};
  char *delim = " ";
 
  while( node != NULL )
  {
    if ( key != NULL )
    {
      if ( NO == start_with(node->key, key) )
      {
        node = node->next;
        continue;
      }
    }

    is_valid_key = true;

    for ( i=0; i<node->config_size; i++)
    {
      j = split(token_list, node->config.content[i], delim);
      if ( j == 0 )
      {
        printf("[%s] Cannot recognize the line: '%s'", __func__, node->config.content[i]);
      }

      DEBUG("[%s]Token[0]=%s\n", __func__, token_list[0]); 
      if ( 0 == strcmp(token_list[0], "auto") ) 
      {
        continue;
      }
      else if ( 0 == strcmp(token_list[0], "iface") )
      {
        printf(" Interface: %s\n", token_list[1]);
        printf(" NetFamily: %s\n", (0 == strcmp("inet", token_list[2])?"IPv4":"IPv6"));
        if ( index_of(token_list[1], ".") > 0 )
        {
          printf(" NetMethod: VLAN/%s\n", (0 == strcmp("dhcp", token_list[3])?"DHCP":"Static"));
        }
        else
        {
          if ( 0 == strcmp("loopback", token_list[3]) )
          {
            printf(" NetMethod: Loopback\n");
          }
          else
          {
            printf(" NetMethod: %s\n", (0 == strcmp("dhcp", token_list[3])?"DHCP":"Static"));
          }
        }
      }
      else if ( 0 == strcmp(token_list[0], "address") ) 
      {
        printf("   Address: %s\n", token_list[1]);
      }
      else if ( 0 == strcmp(token_list[0],  "netmask") )
      {
        printf("   NetMask: %s\n", token_list[1]);
      }
      else if ( 0 == strcmp(token_list[0], "gateway") )
      {
        printf("   Gateway: %s\n", token_list[1]);
      }
      else
      {
        memset(temp_str, 0, sizeof(temp_str));
        for ( temp_index = 0; temp_index < j; temp_index++ )
        {
          strcat(temp_str, token_list[temp_index]);
          if ( temp_index != (j - 1) )
          {
            strcat(temp_str, " ");
          }
        }
        printf("  Attrbute: %s\n", temp_str);
      }
    }
    printf("\n");
    node = node->next;
  }
  
  if ( false == is_valid_key )
  {
    printf("There is no the information of %s interface\n", key);
  }
}

int
get_conf()
{
  uint8_t temp_buff[SIZE] = {0};
  uint8_t str[SIZE] = {0}, str_trim[SIZE]={0};
  char *line;
  FILE *file;
  int ret;
  int config_index = 0;
  int start_index = 0, end_index = 0;
  if_list *node = NULL, *temp_node = NULL;
  file = fopen(IF_ADDR, "r");
  if ( NULL == file )
  {
    printf("Cannot find %s\n", IF_ADDR);
    return FAIL;
  }

  //create first node
  node = create_node(init_node);

  while( NULL != fgets(str, SIZE, file) )
  {
    DEBUG("[%s]BeforeTrim:%s\n", __func__, str);
    memset(str_trim, 0, sizeof(str_trim));
    ret = trim(str_trim, str);
    if ( YES != ret )
    {
      DEBUG("[%s]Failed to do trim()\n", __func__);
    }
    DEBUG("[%s]AfterTrim:%s\n", __func__, str_trim);
 
    memset(str, 0, sizeof(str));
    
    if ( YES == start_with(str_trim, "#" ) || (0 == strcmp("", str_trim)) )
    {
      continue;
    }
    else if ( YES == start_with(str_trim, "auto") )
    {
      start_index = index_of(str_trim, " ") + 1;
      end_index = strlen(str_trim) - 1;
      memcpy(str, &str_trim[start_index], (end_index-start_index)+1);
      node = create_node(str);
    }
    else if ( YES == start_with(str_trim, "iface") )
    {
      //get the 2nd token. it is the net interface
      start_index = index_of(str_trim, " ") + 1;
      end_index = from_index_of(start_index, str_trim, " ") - 1;
    
      //get if name
      memcpy(str, &str_trim[start_index], (end_index-start_index)+1);
      temp_node = search_node(str);
      
      //get if family
      start_index = index_of(str_trim, "inet") + 1;
      end_index = from_index_of(start_index, str_trim, "6");
      
      sprintf(str, "%s_%d", str, (end_index != -1)?6:4);
      DEBUG("Get key:%s\n", str);
      if ( NULL == temp_node )
      {
        //create the new node
        node = create_node(str);
      }
      else
      {
        memset(node->key, 0, sizeof(node->key));
        memcpy(node->key, str, strlen(str));
      }
    }
      
    config_index = node->config_size;
    //store everything we read
    line = create_buff(SIZE);
    memcpy(line, str_trim, strlen(str_trim));
    node->config.content[config_index] = line;
    DEBUG("[RESULT]key:%s, content:[%d][%s]\n", node->key, config_index, node->config.content[config_index]);
    node->config_size++;
    memset(str, 0, sizeof(str)); 
  } 

  if ( (0 == strcmp(node->key, init_node)) )
  {
    printf("Incorrect interface file");
    print_conf(NULL);
    return FAIL;
  }

  return OK;
}
