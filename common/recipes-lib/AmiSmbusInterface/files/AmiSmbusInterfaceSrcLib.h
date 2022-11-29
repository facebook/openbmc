#include <stdint.h>

#define PROT_SESSION_KEY "slot%d_prot_sessionid"

#define DATA_LAYER_MAX_PAYLOAD          (512)  // include 1 byte CheckSum
#define DATA_LAYER_MAX_ACK_PAYLOAD      (252)  // include 1 byte CheckSum
#define PLATFIRE_SLAVE_ADDRESS          (0x51)
#define SMBUS_READ_COMMAND              (0x7F)
#define LINK_LAYER_MAX_LOAD             (30)      // include 1 byte CheckSum
#define LINK_LAYER_MAX_RECEIVE_LOAD     (28)

enum {
  ACK_CMD_UNSUPPORTED = 0x01,
  ACK_BUSY_FLAG = 0x02,
  NOACK_FLAG = 0x03,
  ACK_FLAG = 0x04,
  CHECKSUM_FAILURE = 0x05,
  DECOMMISSIONED = 0x06,
};

#define DERE_PUBLIC_KEY_LENGTH            96
#define EPHEM_PUBLIC_KEY_LENGTH           96
#define DERE_ENCRYPTED_DATA_LENGTH        208
#define SMBUS_MSK_DATA_LENGTH             232
#define TIMEBOUND_MSK_DATA_LENGTH         232
#define SMBUS_ENCRYPTED_DATA_LENGTH       64
#define TIMEBOUND_ENCRYPTED_DATA_LENGTH   64

typedef struct {
    uint32_t type:2;
    uint32_t hour:6;
    uint32_t minute:7;
    uint32_t second:7;
    uint32_t class:4;
    uint32_t subclass:3;
    uint32_t reserved:3;

    uint16_t code;
    uint32_t parameter;

}__attribute__((packed)) PROT_LOG; //total 10 byte

enum {
	// (V0.65) 8/17
	CMD_RECOVERY_SPI_UNLOCK_REQUEST = 0x80,
	CMD_UPDATE_COMPLTE = 0x81,
	CMD_BOOT_STATUS = 0x82,
	CMD_PROT_GRANT_ACCESS = 0x83,
	CMD_FW_UPDATE_INTENT_REQUEST = 0x84,
	CMD_UPDATE_STATUS = 0x85,
	CMD_ON_DEMAND_CPLD_UPDATE = 0x86,
	CMD_ENABLE_TIME_BOUND_DEBUG = 0x87,
	CMD_DYNAMIC_REGOIN_SPI_UNLOCK_REQUEST = 0x88,
	CMD_SYNC_COMPELETE_AND_BOOT_PCH = 0x89,
	CMD_DECOMMSION_REQUST = 0x8A,
	CMD_RECOMMISSION_REQUST = 0x8B,
	CMD_UFM_LOG_READOUT_ENTRY = 0x8C,
	CMD_LOG_DATA_LAYER = 0x8D,
	CMD_READ_MANIFEST_DATA = 0x8E,
  CMD_FW_ATTESTATION = 0x8F,
  CMD_PROT_ATTESTATION = 0x90,
  CMD_READ_UNIQUE_SERIAL_NUMBER = 0x91,
  CMD_SMBUS_FILTER_SUPPORT = 0x94,
  CMD_SMBUS_FILTER_MSK_PROVISION = 0x95,
  CMD_DEBUG_MSK_PROVISION = 0x96,
};

typedef struct {
    unsigned char Command:6; //Command will be unique for each datalayer transaction.
    unsigned char AckNeeded:1;
    unsigned char PackageType:1; 
    unsigned char Length;
    unsigned char SubPackageIndex:4;
    unsigned char LastPackageIndex:4;
    unsigned char PayLoad[LINK_LAYER_MAX_LOAD + 1]; //Last payload will be checksum.
} LINK_LAYER_PACKET_MASTER;

///
///  LinkLayer receive acknowledgment structure
///
typedef struct {
    unsigned char Length;
    unsigned char Command:6;
    unsigned char AckNeeded:1; 
    unsigned char PackageType:1; // 0->data, 1->ack
    unsigned char SubPackageIndex:4;
    unsigned char AckFlag:4; //ACK=0x1, NoACK=0x2
    unsigned char PayLoad[LINK_LAYER_MAX_RECEIVE_LOAD];
    // [2021-10-4 Feng] Actual data-> 20 81 10  0 0 0 0  0 0 0 0  0 0 0 0   0 0 0 0   0 0 0 0   0 0 0 0   0 0 0 0   4f
    // [2021-10-6 Madhan] BIOS Smbus is not able to handle 33 bytes in physical layer. They can send only 32 bytes. We need to maintain the same smbus layer in the code we will provide the updated document with this information and library update.

    unsigned char CheckSum; //checksum of all the bytes in the linklayer pkg except the checksum
} LINK_LAYER_PACKET_ACK_MASTER; // 32

typedef struct {
    unsigned char Command;
    unsigned char Length;
    unsigned char Flag; // Bit 0, 0 Data, 1 ACK    Bit 1, 0 NoAck 1 ACK    Bit 2 need ACK? 
    unsigned char PayLoad[255];  // include 1 byte CheckSum
} DATA_LAYER_PACKET;

typedef struct {
    volatile unsigned char Command;
    volatile uint16_t Length;
    volatile unsigned char Flag; // Bit 0, 0 Data, 1 ACK    Bit 1, 0 NoAck 1 ACK    Bit 2 need ACK? 
    unsigned char PayLoad[DATA_LAYER_MAX_PAYLOAD];  // include 1 byte CheckSum
}__attribute__((packed)) DATA_LAYER_PACKET_EXTEND;


// sizeof(DATA_LAYER_PACKET) = 258 
// sizeof(DATA_LAYER_PACKET_EXTEND) = 518 
typedef struct {
    unsigned char Length;
    unsigned char AckCommand; 
    unsigned char Flag;
    unsigned char PayLoad[DATA_LAYER_MAX_ACK_PAYLOAD];  // include 1 byte CheckSum
} DATA_LAYER_PACKET_ACK; // 255

// Function Declarations
//

/**
  Sending data by splitting into number of LinkLayer Packets

  @param  IN DataPacket - Buffer contains data layer structure data
  
  @retval int 0 - SUCCESS 
              1 - FAIL

**/
int SendDataPacketThroughLinkLayer(
        unsigned char slot_id,
        int fd,
        unsigned char *DataPacket,
        unsigned int data_payload_length,
        unsigned int linklayer_ack_delay,
        int verbose
        );

/**
  Function is to send the splitted linklayer packets

  @param  IN unsigned char Command,
  @param  IN unsigned char *SendBuffer,
  @param  IN unsigned int  BufferLength,
  @param  IN unsigned char AckNeeded,
  @param  IN unsigned char SubPackageIndex,
  @param  IN unsigned char LastSubPackageIndex 
  
  @param  int 0 - SUCCESS 
              1 - FAIL

**/
int SendPacketThroughLinkLayer( 
        int fd,
        unsigned char Command,
        unsigned char *SendBuffer,
        unsigned int  BufferLength,
        unsigned char ACKneeded,
        unsigned char Subpkgindex,
        unsigned char Lastsubpkgindex,
        int verbose
        ); 

/**
  Function is receive acknowldgement for Link Layer Packet
  
  @param IN unsigned char *Buffer
  @param IN unsigned int Length
  
  @retval 0 - SUCCESS, 1 - FAIL
**/
int LinkLayerReceiveAcknowldgement(
        int fd,
        unsigned char session_id,
        unsigned char *RecieveBuffer,
        int verbose
        );

/**
  Function is to Send acknowledgment after receiving packet

  @param  unsigned int SessionId used to updated command for linklayer packet 
  @param  unsigned char SubPackageId
  
  @retval int 0 - SUCCESS, 1 - FAIL

**/
int LinkLayerSendAcknowldgement(
        int fd,
        unsigned int SessionId, 
        unsigned char SubPackageId,
        int verbose
        ); 

/**
  Function is Send Acknowldgement for Link Layer Packet
  
  @param IN unsigned char *RecieveBuffer
  @param IN unsigned int Length
  
  @retval int 0 - SUCCESS, 1 - FAIL
**/
// int LinkLayerReceiveAcknowldgement(
//         unsigned char *RecieveBuffer,
//         int *Length
//         );

/**
  Function is to Calculate Checksum for the data except the checksum field

  @param  unsigned char *buffer, 
  @param  int Length
  
  @param  unsigned char 

**/
unsigned char checksum_verify(unsigned char *buffer, int length);

unsigned char CalculateCheckSum(
        unsigned char *Buffer, 
        int Length
        );

int
datalayer_send_receive(unsigned char slot_id,
                       int fd,
                       unsigned char command,
                       unsigned char *data_payload,
                       unsigned int data_payload_length,
                       unsigned char *DataLayerAckBuffer,
                       int verbose);


int sent_data_packet(unsigned char slot_id,
                         int fd,
        								 unsigned char command,
        								 unsigned char *data_payload,
        								 unsigned int data_payload_length,
        							 	 int verbose);

/**
  This function is use to receive back the data Layer packet

  @param  none
  
  @param  none

**/
int DataLayerReceiveFromPlatFire(unsigned char slot_id, int fd, unsigned char *DataLayerAckBuffer, int verbose);

/**
  Function is Smbus Locate And Write Execute
  
  @param unsigned char Command
  @param unsigned int Length
  @param unsigned char *Buffer
  
  @retval 0 - SUCCESS, 1 - FAIL
**/
int SmbusWriteBlockExecute(
        int fd,
        unsigned char Command,
        unsigned int Length,
        unsigned char *Buffer,
        int verbose
        );


		
/**
  MicroSecond Delay
  
  @param IN int Delay
  
  @retval void
  
**/
void MicroSecondDelay(
        int Delay,
        int verbose
        );
        
/**
  Print buffer content
  
  @param IN UINT8 *Buffer
  @param IN UINTN BufferLength
  
  @retval void
  
**/
void PrintBuffer(
        unsigned char *Buffer,
        unsigned int BufferLength,
        int verbose
        );
        
#define DEBUG(fmt, args...)		\
	do {					\
		if (verbose)		\
			printf(fmt, ##args);	\
	} while (0)