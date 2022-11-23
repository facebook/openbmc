#pragma once

#define MCTP_CMD_GET_EID   2
#define MCTP_CMD_GET_UUID  3

#define MSG_TYPE_PLDM  (1)
#define MSG_TYPE_NCSI  (2)
#define MSG_TYPE_SPDM  (5)
#define MSG_HDR_SIZE  (4)

#define IPMI_REQ_MCTP_HADR_SIZE (9)

//0-7
enum {
  TAG_MSG_SENSORD = 3,
  TAG_MSG_PLDMD,
};

enum {
    MCTP_OVER_SMBUS = 0,
    MCTP_OVER_I3C = 1,
};

