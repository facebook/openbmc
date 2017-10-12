#ifndef _SERVER_H_
#define _SERVER_H_

class Server {
  private:
    uint8_t _slot_id;
  public:
    Server(uint8_t slot_id) : _slot_id(slot_id) {}
    bool ready();
};

#endif
