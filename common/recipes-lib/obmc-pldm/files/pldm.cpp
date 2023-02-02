#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <memory>
#include <libpldm/platform.h>
#include <libpldm/base.h>
#include <libpldm/pldm.h>
#include "base.hpp"
#include "platform.hpp"
#include "oem.hpp"
#include "pldm.h"

using namespace pldm::responder;

const char pldm_path[] = "\0pldm-mux";
const uint8_t MCTP_MSG_TYPE_PLDM = 1;

void pldm_msg_handle (uint8_t payload_id, uint8_t* req, size_t req_size, uint8_t** resp, int* resp_bytes) {
  std::unique_ptr<CmdHandler> handler;
  Response response;
  auto request = reinterpret_cast<pldm_msg*>(req);

  // choosing handler base on PLDM header type.
  switch (request->hdr.type) {
    case PLDM_PLATFORM:
      handler = std::make_unique<platform::Handler>();
      break;
    case PLDM_OEM:
      handler = std::make_unique<oem::Handler>();
      break;
    default:
      handler = std::make_unique<base::Handler>();
      break;
  }

  handler->payload_id = payload_id;

  // handle command
  response = handler->handle(request->hdr.command, request, req_size - PLDM_HEADER_SIZE);
  if (response.empty()) {
    *resp_bytes = 0;
  } else {
    *resp = (uint8_t*)malloc(response.size());
    if (!(*resp)) {
      *resp_bytes = 0;
      std::cerr << "Response allocation failure\n";
    } else {
      std::copy(response.begin(), response.end(), *resp);
      *resp_bytes = (int)response.size();
    }
  }
}

static int connect_to_socket (const char * path, int length) {
  int returnCode = 0;
  int sockfd = socket(AF_UNIX, SOCK_SEQPACKET, 0);

  if (-1 == sockfd) {
    returnCode = -errno;
    std::cerr << "Failed to create the socket, RC= " << returnCode << "\n";
    return -1;
  }

  struct timeval tv_timeout;
  tv_timeout.tv_sec  = 10;
  tv_timeout.tv_usec = 0;
  if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (void *) &tv_timeout, sizeof(struct timeval)) < 0) {
    returnCode = -errno;
    std::cerr << "Failed to set send() timeout, RC= " << returnCode << "\n";
    goto socketfail;
  }

  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (void *) &tv_timeout, sizeof(struct timeval)) < 0) {
    returnCode = -errno;
    std::cerr << "Failed to set recv() timeout, RC= " << returnCode << "\n";
    goto socketfail;
  }

  struct sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  memcpy(addr.sun_path, "\0", 1);
  memcpy(addr.sun_path+1, path, length++);
  if (connect(sockfd, (struct sockaddr *)&addr, length + sizeof(addr.sun_family)) < 0) {
    returnCode = -errno;
    std::cerr << "Failed to connect the socket, RC= " << returnCode << "\n";
    goto socketfail;
  }

  return sockfd;

socketfail:
  oem_pldm_close(sockfd);
  return -1;
}

static pldm_requester_rc_t mctp_recv (mctp_eid_t eid, int mctp_fd,
                                    uint8_t **pldm_resp_msg,
                                    size_t *resp_msg_len) {
  ssize_t min_len = sizeof(eid) + sizeof(MCTP_MSG_TYPE_PLDM) +
                    sizeof(struct pldm_msg_hdr);
  ssize_t length = recv(mctp_fd, NULL, 0, MSG_PEEK | MSG_TRUNC);

  if (length <= 0) {
    return PLDM_REQUESTER_RECV_FAIL;
  } else if (length < min_len) {
    /* read and discard */
    uint8_t buf[length];
    recv(mctp_fd, buf, length, 0);
    return PLDM_REQUESTER_INVALID_RECV_LEN;
  } else {
    struct iovec iov[2];
    size_t mctp_prefix_len = sizeof(eid) + sizeof(MCTP_MSG_TYPE_PLDM);
    uint8_t mctp_prefix[mctp_prefix_len];
    size_t pldm_len = length - mctp_prefix_len;

    iov[0].iov_len = mctp_prefix_len;
    iov[0].iov_base = mctp_prefix;
    *pldm_resp_msg = (uint8_t *)malloc(pldm_len);
    iov[1].iov_len = pldm_len;
    iov[1].iov_base = *pldm_resp_msg;

    struct msghdr msg = {0};
    msg.msg_iov = iov;
    msg.msg_iovlen = sizeof(iov) / sizeof(iov[0]);
    ssize_t bytes = recvmsg(mctp_fd, &msg, 0);

    if (length != bytes) {
      free(*pldm_resp_msg);
      return PLDM_REQUESTER_INVALID_RECV_LEN;
    }
    if ((mctp_prefix[0] != eid) ||
        (mctp_prefix[1] != MCTP_MSG_TYPE_PLDM)) {
      free(*pldm_resp_msg);
      return PLDM_REQUESTER_NOT_PLDM_MSG;
    }
    *resp_msg_len = pldm_len;
    return PLDM_REQUESTER_SUCCESS;
  }
}

int oem_pldm_send (int eid, int pldmd_fd,
                      const uint8_t *pldm_req_msg, size_t req_msg_len)
{
  int ret;
  for (int retry = 0; retry < 2; retry++) {
    ret = pldm_send(eid, pldmd_fd, pldm_req_msg, req_msg_len);
    if (ret == 0 || errno != EAGAIN)
      return ret;
  }
  printf("%s: resource still unavailable after retry 2 times.\n", __func__);

  return ret;
}

int oem_pldm_recv (int eid, int pldmd_fd,
                      uint8_t **pldm_resp_msg, size_t *resp_msg_len)
{
  return (int)mctp_recv(eid, pldmd_fd, pldm_resp_msg, resp_msg_len);
}

int oem_pldm_send_recv (uint8_t bus, int eid,
                      const uint8_t *pldm_req_msg, size_t req_msg_len,
                      uint8_t **pldm_resp_msg, size_t *resp_msg_len) {
  std::string path = "pldm-mux" + std::to_string(bus);
  int pldmd_fd = connect_to_socket(path.c_str(), path.length());

  if (pldmd_fd < 0) {
    return PLDM_REQUESTER_OPEN_FAIL;
  }

  auto rc = oem_pldm_send_recv_w_fd (eid, pldmd_fd,
                                     pldm_req_msg, req_msg_len,
                                     pldm_resp_msg, resp_msg_len);

  // printf("%s return code = %d\n", __func__, (int)rc);
  oem_pldm_close(pldmd_fd);
  return rc;
}

int oem_pldm_send_recv_w_fd (int eid, int pldmd_fd,
                      const uint8_t *pldm_req_msg, size_t req_msg_len,
                      uint8_t **pldm_resp_msg, size_t *resp_msg_len)
{
  struct pldm_msg_hdr *hdr = (struct pldm_msg_hdr *)pldm_req_msg;
  int rc = PLDM_REQUESTER_SUCCESS;

  if ((hdr->request != PLDM_REQUEST) &&
    (hdr->request != PLDM_ASYNC_REQUEST_NOTIFY)) {
    printf("%s return code = %d\n", __func__, PLDM_REQUESTER_NOT_REQ_MSG);
    return PLDM_REQUESTER_NOT_REQ_MSG;
  }

  for (int retry = 0; retry < 2; retry++) {
    rc = oem_pldm_send(eid, pldmd_fd, pldm_req_msg, req_msg_len);
    if (rc) {
      printf("%s return code = %d(%d)\n", __func__, PLDM_REQUESTER_SEND_FAIL, -errno);
      return PLDM_REQUESTER_SEND_FAIL;
    }

    rc = oem_pldm_recv((int)eid, pldmd_fd, pldm_resp_msg, resp_msg_len);
    if (rc) {
      if (errno != EAGAIN && errno != EINTR) {
        printf("%s return code = %d(%d)\n", __func__, PLDM_REQUESTER_RECV_FAIL, -errno);
        return PLDM_REQUESTER_RECV_FAIL;
      }
    } else {
      return PLDM_REQUESTER_SUCCESS;
    }
  }
  return PLDM_REQUESTER_RECV_FAIL;
}

int oem_pldm_init_fd (uint8_t bus) {
  std::string path = "pldm-mux" + std::to_string(bus);
  int fd = connect_to_socket(path.c_str(), path.length());
  if (fd < 0) {
    return PLDM_REQUESTER_OPEN_FAIL;
  }

  return fd;
}

int oem_pldm_init_fwupdate_fd (uint8_t bus) {
  std::string path = "pldm-fwup-mux" + std::to_string(bus);
  int fd = connect_to_socket(path.c_str(), path.length());
  if (fd < 0) {
    return PLDM_REQUESTER_OPEN_FAIL;
  }

  return fd;
}

void oem_pldm_close (int fd)
{
  close(fd);
}

void get_pldm_ipmi_req_hdr (uint8_t *buf) {
  auto req = reinterpret_cast<pldm_msg*>(buf);
  struct pldm_header_info header;
  header.msg_type  = PLDM_REQUEST;
  header.instance  = 0x00;
  header.pldm_type = PLDM_OEM;
  header.command   = oem::CMD::IPMI;
  uint8_t rc = pack_pldm_header(&header, &(req->hdr));
  if (rc != PLDM_SUCCESS) {
    printf("%s error code : 0x%02X\n", __func__, rc);
  }
}

void get_pldm_ipmi_req_hdr_w_IANA (uint8_t *buf, uint8_t *IANA, size_t IANA_length) {
  get_pldm_ipmi_req_hdr(buf);
  auto req = reinterpret_cast<pldm_msg*>(buf);
  memcpy(req->payload, IANA, IANA_length);
}

int
oem_pldm_ipmi_send_recv(uint8_t bus, uint8_t eid,
                        uint8_t netfn, uint8_t cmd,
                        uint8_t *txbuf, uint8_t txlen,
                        uint8_t *rxbuf, size_t *rxlen) {
  uint8_t tbuf[255] = {0};
  uint8_t IANA[] = {0x15, 0xA0, 0x00};
  size_t payload_len = 0;
  struct pldm_msg *pldmbuf = (struct pldm_msg *)tbuf;

  // get pldm header$
  get_pldm_ipmi_req_hdr_w_IANA(tbuf, IANA, sizeof(IANA));
  payload_len += sizeof(IANA);

  // get ipmi message$
  pldmbuf->payload[payload_len++] = netfn << 2;     // IPMI NetFn
  pldmbuf->payload[payload_len++] = cmd;            // IPMI Cmd

  // OEM netfn range between 0x30 and 0x3F
  if(netfn >= 0x30 && netfn <= 0x3F) {
    // IPMI IANA
    memcpy(pldmbuf->payload + payload_len, IANA, sizeof(IANA));
    payload_len += sizeof(IANA);
  }

  // IPMI Data
  memcpy(pldmbuf->payload + payload_len, txbuf, txlen);
  payload_len += txlen;

  size_t tlen = payload_len + PLDM_HEADER_SIZE;
  int rc=0;
  uint8_t *pldm_rbuf = nullptr;

  do {
    rc = oem_pldm_send_recv(bus, eid, tbuf, tlen, &pldm_rbuf, rxlen);

    //check ipmi lens
    if(rc || *rxlen < PLDM_IPMI_HEAD_LEN) {
      break;
    }
    //ipmi complete code
    if (pldm_rbuf[PLDM_OEM_IPMI_CC_OFFSET] != 0) {
      printf("%s CC=0x%x\n", __func__, pldm_rbuf[PLDM_OEM_IPMI_CC_OFFSET] );
      rc = PLDM_REQUESTER_RECV_FAIL;
      break;
    }

    // OEM netfn range between 0x30 and 0x3F
    if(netfn >= 0x30 && netfn <= 0x3F) {
      *rxlen -= PLDM_OEM_IPMI_DATA_OFFSET;
      memcpy(rxbuf, pldm_rbuf + PLDM_OEM_IPMI_DATA_OFFSET, *rxlen);
    }
    else {
      *rxlen -= PLDM_IPMI_HEAD_LEN;
      memcpy(rxbuf, pldm_rbuf + PLDM_IPMI_HEAD_LEN, *rxlen);
    }

  } while(0);

  if(pldm_rbuf != nullptr) {
    free(pldm_rbuf);
  }
  return rc;
}
