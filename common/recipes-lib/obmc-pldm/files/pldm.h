
#ifndef _PLDM_OEM_H_
#define _PLDM_OEM_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define MCTP_PLDM_TYPE               (0x01)
#define PLDM_HEADER_SIZE             (3)
#define PLDM_RESP_HEADER_SIZE        (4)
#define IPMI_IANA_LEN                (3)
#define PLDM_OEM_IPMI_CC_OFFSET      (9)
#define PLDM_IPMI_HEAD_LEN           (10)
#define PLDM_OEM_IPMI_DATA_OFFSET    (PLDM_IPMI_HEAD_LEN + IPMI_IANA_LEN)



/**
 * @brief Test code
 */
void pldm_msg_handle (uint8_t payload_id, uint8_t* req, size_t req_size, uint8_t** resp, int* resp_bytes);

/**
 * @brief Send a PLDM request message, don't wait for response. Essentially an
 *        async API. A user of this would typically have added the MCTP fd to an
 *        event loop for polling. Once there's data available, the user would
 *        invoke pldm_recv().
 *
 * @param[in] eid           - destination MCTP eid
 * @param[in] pldmd_fd      - target fd (:=PLDM Daemon fd)
 * @param[in] pldm_req_msg  - caller owned pointer to PLDM request msg
 * @param[in] req_msg_len   - size of PLDM request msg
 *
 * @return pldm_requester_rc_t (errno may be set)
 */
int oem_pldm_send (int eid, int pldmd_fd,
			      const uint8_t *pldm_req_msg, size_t req_msg_len);

/**
 * @brief Read MCTP socket. If there's data available, return success only if
 *        data is a PLDM response message that matches eid and instance_id.
 *
 * @param[in]  eid           - destination MCTP eid
 * @param[in]  pldmd_fd      - target fd (:=PLDM Daemon fd)
 * @param[out] pldm_resp_msg - *pldm_resp_msg will point to PLDM response msg,
 *                             this function allocates memory, caller to 
 *                             free(*pldm_resp_msg) on success.
 * @param[out] resp_msg_len  - caller owned pointer that will be made point to
 *                             the size of the PLDM response msg.
 */
int oem_pldm_recv (int eid, int pldmd_fd,
                uint8_t **pldm_resp_msg, size_t *resp_msg_len);

/**
 * @brief Send a PLDM request message to PLDM Daemon. Wait for corresponding 
          response message, which once received, is returned to the caller.
 *
 * @param[in]  bus           - bus number
 * @param[in]  eid           - destination MCTP eid
 * @param[in]  pldm_req_msg  - caller owned pointer to PLDM request msg
 * @param[in]  req_msg_len   - size of PLDM request msg
 * @param[out] pldm_resp_msg - *pldm_resp_msg will point to PLDM response msg,
 *                             this function allocates memory, caller to 
 *                             free(*pldm_resp_msg) on success.
 * @param[out] resp_msg_len  - caller owned pointer that will be made point to
 *                             the size of the PLDM response msg.
 */
int oem_pldm_send_recv (uint8_t bus, int eid,
                        const uint8_t *pldm_req_msg, size_t req_msg_len, 
                        uint8_t **pldm_resp_msg, size_t *resp_msg_len);

/**
 * @brief Send a PLDM request message to PLDM Daemon. Wait for corresponding 
          response message, which once received, is returned to the caller.
 *
 * @param[in]  eid           - destination MCTP eid
 * @param[in]  fd            - target file descriptor
 * @param[in]  pldm_req_msg  - caller owned pointer to PLDM request msg
 * @param[in]  req_msg_len   - size of PLDM request msg
 * @param[out] pldm_resp_msg - *pldm_resp_msg will point to PLDM response msg,
 *                             this function allocates memory, caller to 
 *                             free(*pldm_resp_msg) on success.
 * @param[out] resp_msg_len  - caller owned pointer that will be made point to
 *                             the size of the PLDM response msg.
 */
int oem_pldm_send_recv_w_fd (int eid, int pldmd_fd,
                        const uint8_t *pldm_req_msg, size_t req_msg_len, 
                        uint8_t **pldm_resp_msg, size_t *resp_msg_len);

/**
 * @brief Init PLDM (firmware update) fd which connect to PLDM daemon.
 * @param[in]  bus           - bus number
 */
int oem_pldm_init_fd (uint8_t bus);
int oem_pldm_init_fwupdate_fd (uint8_t bus);

/**
 * @brief Close PLDM (firmware update) fd which connect to PLDM daemon.
 * @param[in]  fd            - file descriptor
 */
void oem_pldm_close (int fd);

/**
 * @brief Get PLDM request header for ipmi message.
 * @param[in]  buf           - Full PLDM message that transfer by PLDM daemon or 
 *                             PLDM library function.
 * @param[in]  IANA          - PLDM-IPMI message is an OEM command, payload include IANA.
 * @param[in]  IANA_length   - IANA length.
 */
void get_pldm_ipmi_req_hdr (uint8_t *buf);
void get_pldm_ipmi_req_hdr_w_IANA (uint8_t *buf, uint8_t *IANA, size_t IANA_length);
int oem_pldm_ipmi_send_recv(uint8_t bus, uint8_t eid,
                            uint8_t netfn, uint8_t cmd,
                            uint8_t *txbuf, uint8_t txlen,
                            uint8_t *rxbuf, size_t *rxlen,
                            bool is_ipmi_iana_auto);

#ifdef __cplusplus
}
#endif
#endif



