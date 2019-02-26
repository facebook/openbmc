#ifndef _SERVER_H_
#define _SERVER_H_

class Server {
  private:
    uint8_t _slot_id;
    std::string fru_name;
  public:
    Server(uint8_t slot_id, std::string fru) : _slot_id(slot_id), fru_name(fru) {}
    // Throws exception if not
    void ready();
};

#endif
