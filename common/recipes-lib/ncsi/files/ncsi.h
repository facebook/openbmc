#ifndef _NCSI_H_
#define _NCSI_H_

#define NETLINK_USER 31
#define NCSI_IF_REINIT 0xEF



#define MAX_PAYLOAD 128 /* maximum payload size*/
#define MAX_RESPONSE_PAYLOAD 256 /* maximum payload size*/


typedef struct ncsi_nl_msg_t {
  char dev_name[10];
  unsigned char channel_id;
  unsigned char cmd;
  unsigned char payload_length;
  unsigned char msg_payload[MAX_PAYLOAD];
} NCSI_NL_MSG_T;

typedef struct ncsi_nl_response {
  unsigned char payload_length;
  unsigned char msg_payload[MAX_RESPONSE_PAYLOAD];
} NCSI_NL_RSP_T;



typedef union {
  struct
  {
     unsigned long link_flag :    1;
     unsigned long speed_duplex : 4;
     unsigned long auto_negotiate_en : 1;
     unsigned long auto_negoation_complete : 1;
     unsigned long parallel_detection : 1;
     unsigned long rsv : 1;
     unsigned long link_partner_1000_full_duplex : 1;
     unsigned long link_partner_1000_half_duplex : 1;
     unsigned long link_partner_100_T4 : 1;
     unsigned long link_partner_100_full_duplex : 1;
     unsigned long link_partner_100_half_duplex : 1;
     unsigned long link_partner_10_full_duplex : 1;
     unsigned long link_partner_10_half_duplex : 1;
     unsigned long tx_flow_control : 1;
     unsigned long rx_flow_control : 1;
     unsigned long link_partner_flow_control: 2;
     unsigned long serdes : 1;
     unsigned long oem_link_speed_valid : 1;
     unsigned long rsv2 : 10;
  } bits;
  unsigned long all32;
} Link_Status;

typedef union {
  struct
  {
     unsigned long host_NC_driver_status :    1;
     unsigned long rsv : 31;
  } bits;
  unsigned long all32;
} Other_Indications;

/* Get Link Status Response */
typedef struct {
  Link_Status link_status;
  Other_Indications other_indications;
  unsigned long oem_link_status;
} __attribute__((packed)) Get_Link_Status_Response;

/* NC-SI Response Packet */
typedef struct {
/* end of NC-SI header */
	unsigned short  Response_Code;
	unsigned short  Reason_Code;
	unsigned char   Payload_Data[128];
} __attribute__((packed)) NCSI_Response_Packet;


int ncsi_init_if(int);
void handle_ncsi_config(void);
int getMacAddr(int *values);
int checkValidMacAddr(int *value);
int check_ncsi_status(void);
void print_ncsi_resp(NCSI_NL_RSP_T *rcv_buf);

#endif
