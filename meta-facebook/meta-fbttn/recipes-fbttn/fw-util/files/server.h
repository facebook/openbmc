#ifndef _SERVER_H_
#define _SERVER_H_

#define SERVER_ERR_TYPE_CNT 5
#define SERVER_ERR_STR_LEN 64

class Server {
  private:
    uint8_t _slot_id;
    char *_fru_name;
    char *_err_str;
  public:
    Server(uint8_t slot_id, char *fru_name, char *err_str) : _slot_id(slot_id), _fru_name(fru_name), _err_str(err_str) {}
    bool ready();
};

#endif
