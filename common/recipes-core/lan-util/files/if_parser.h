#ifndef __IF_PARSER_H__
#define __IF_PARSER_H__

#ifdef __cplusplus
extern "C" {
#endif

#define IF         (1 << 0)
#define IPV4       (1 << 1)
#define IPV6       (1 << 2)
#define MASK       (1 << 3)
#define GATEWAY    (1 << 4)
#define DHCP       (1 << 5)
#define DHCP6      (1 << 6)
#define NET_FAMILY (1 << 7)
#define RUN_IF     (1 << 11)
#define SHOW_IF    (1 << 12)
#define DEL_IF     (1 << 13)
#define ADD_IF     (1 << 14)
#define SHOW_RANGE (1 << 15)

#define IPV4_ADD_DHCP_CHECK   ( ADD_IF | IF | DHCP )
#define IPV4_ADD_STATIC_CHECK ( ADD_IF | IF | IPV4 | MASK | GATEWAY)
#define IPV4_DEL_CHECK        ( DEL_IF | IF | IPV4 )
#define IPV6_ADD_DHCP_CHECK   ( ADD_IF | IF | DHCP6 )
#define IPV6_ADD_STATIC_CHECK ( ADD_IF | IF | IPV6 | MASK)
#define IPV6_DEL_CHECK        ( DEL_IF | IF | IPV6 )
#define SHOW_GIVEN_IF         ( SHOW_IF | IF )
#define SHOW_ALL_IF           ( SHOW_IF )

#define SIZE 128

enum
{
  OK = 0,
  YES = 0,
  FAIL = -1,
  NO = -1,
};

typedef struct ip
{
  char key[16];
  char *ip_dev;
  char *ip_ip;
  char *ip_mask;
  char *ip_gw;
  uint16_t flag;
} ip_info;

typedef struct if_info
{
  char *content[SIZE];
} if_config;

typedef struct list
{
  char key[16];
  int config_size;
  if_config config;
  struct list *next;
} if_list;


void free_all();
char* create_buff(int size);
int index_of(const char *str, const char *c);
int from_index_of(int from_index, const char *str, const char *c);
int start_with(const char *str, const char *c);
int end_with(const char *str, const char *c);
int trim(char *dst, char *str);
int write_data_to_conf(ip_info *ip);
int delete_data_from_conf(char *key);
int update_data_to_conf();
void print_conf(char *key);
int get_conf();

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __IF_PARSER_H__ */
