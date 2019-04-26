#ifndef _AEN_H_
#define _AEN_H_

#include "ncsi.h"


/* AEN Type */
#define AEN_TYPE_LINK_STATUS_CHANGE            0x0
#define AEN_TYPE_CONFIGURATION_REQUIRED        0x1
#define AEN_TYPE_HOST_NC_DRIVER_STATUS_CHANGE  0x2
#define AEN_TYPE_MEDIUM_CHANGE                0x70
#define AEN_TYPE_PENDING_PLDM_REQUEST         0x71
#define AEN_TYPE_OEM                          0x80

#define MAX_AEN_DATA_IN_SHORT                   16


/* Broadcom-specific  OEM AEN definitions */
#define NCSI_AEN_TYPE_OEM_BCM_HOST_ERROR                       0x80
#define BCM_HOST_ERR_TYPE_UNGRACEFUL_HOST_SHUTDOWN             0x01
#define NCSI_AEN_TYPE_OEM_BCM_RESET_REQUIRED                   0x81
#define NCSI_AEN_TYPE_OEM_BCM_HOST_DECOMMISSIONED              0x82

/* Mellanox-specific  OEM AEN definitions */
#define NCSI_AEN_TYPE_OEM_MLX_THERMAL_EVENT                    0x80
#define NCSI_AEN_TYPE_OEM_MLX_HIGH_MODULE_CURRENT              0x81
#define NCSI_AEN_TYPE_OEM_MLX_DEVICE_SELF_RESET                0x82




#define AEN_CTRL_STD_STARTING_BIT   0
#define AEN_CTRL_OEM_STARTING_BIT   16

// default set of AENs to enable
#define AEN_ENABLE_DEFAULT \
   ( (1<<AEN_TYPE_LINK_STATUS_CHANGE) | \
     (1<<AEN_TYPE_CONFIGURATION_REQUIRED) | \
     (1<<AEN_TYPE_HOST_NC_DRIVER_STATUS_CHANGE) \
   )


// set of AENs to enable for Broadcom NICs
#define AEN_ENABLE_MASK_BCM \
   (  AEN_ENABLE_DEFAULT | \
		  /* Broadcom OEM AENs */ \
     ((1<<(NCSI_AEN_TYPE_OEM_BCM_HOST_ERROR - AEN_TYPE_OEM)) | \
		  (1<<(NCSI_AEN_TYPE_OEM_BCM_RESET_REQUIRED - AEN_TYPE_OEM)) |  \
		  (1<<(NCSI_AEN_TYPE_OEM_BCM_HOST_DECOMMISSIONED - AEN_TYPE_OEM))) << AEN_CTRL_OEM_STARTING_BIT \
	 )

// set of AENs to enable for Mellanox NICs
#define AEN_ENABLE_MASK_MLX \
   (  AEN_ENABLE_DEFAULT \
	 )

typedef struct {
/* Ethernet Header */
	unsigned char  DA[6];
	unsigned char  SA[6]; /* Network Controller SA = FF:FF:FF:FF:FF:FF */
	unsigned short EtherType; /* DMTF NC-SI */
/* AEN Packet Format */
	/* AEN HEADER */
	/* Network Controller should set this field to 0x00 */
	unsigned char  MC_ID;
	/* For NC-SI 1.0 spec, this field has to set 0x01 */
	unsigned char  Header_Revision;
	unsigned char  Reserved_1; /* Reserved has to set to 0x00 */
	unsigned char  IID;        /* Instance ID = 0 in AEN */
	unsigned char  Command;    /* AEN = 0xFF */
	unsigned char  Channel_ID;
	unsigned short Payload_Length; /* Payload Length = 4 in Network Controller AEN Packet */
	unsigned long  Reserved_2;
	unsigned long  Reserved_3;
/* end of  AEN HEADER */
	unsigned char  Reserved_4[3];
	unsigned char  AEN_Type;
	unsigned short  Optional_AEN_Data[MAX_AEN_DATA_IN_SHORT];
} __attribute__((packed)) AEN_Packet;



int is_aen_packet(AEN_Packet *buf);
void enable_default_aens();
int process_NCSI_AEN(AEN_Packet *buf);

#endif
