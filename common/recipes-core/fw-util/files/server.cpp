#include <iostream>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <openbmc/pal.h>
#include "server.h"

using namespace std;

void Server::ready()
{
  int ret;
  uint8_t status;
  ret = pal_is_fru_prsnt(_slot_id, &status);
  if (ret < 0) {
  #ifdef DEBUG
    cerr << "pal_is_fru_prsnt failed for fru " << _slot_id << endl;
  #endif
    throw runtime_error("get " + fru_name + " present failed");
  }
  if (status == 0) {
#ifdef DEBUG
    cerr << "slot" << _slot_id << " is empty!" << endl;
#endif
    throw runtime_error(fru_name + " is empty");
  }
  ret = pal_get_server_power(_slot_id, &status);
  if (ret < 0) {
#ifdef DEBUG
    cerr << "Failed to get server power status. Stopping update" << endl;
#endif
    throw runtime_error( "get " + fru_name + " power status failed");
  }
  if (status == SERVER_12V_OFF) {
#ifdef DEBUG
    cerr << "Can't update FW version since the server is 12V off!" << endl;
#endif
    throw runtime_error( fru_name + " power 12V-off");
  }
  if (!pal_is_slot_support_update(_slot_id)) {
#ifdef DEBUG
    cerr << "slot" << _slot_id << " is not a server" << endl;
#endif
    throw runtime_error("fru " + fru_name + " is not a server");
  }
}
