#include <iostream>
#include <cstdio>
#include <cstring>
#include <openbmc/pal.h>
#include "server.h"

using namespace std;

void Server::ready()
{
  int ret;
  uint8_t status;

  ret = pal_is_fru_prsnt(FRU_SCM, &status);
  if (ret < 0) {
    cerr << "pal_is_fru_prsnt failed for fru " << _slot_id << endl;
    throw "get " + fru_name + " present failed";
  }
  if (status == 0) {
    cerr << "slot" << _slot_id << " is empty!" << endl;
    throw fru_name + " is empty";
  }
  ret = pal_get_server_power(FRU_SCM, &status);
  if (ret < 0) {
    cerr << "Failed to get server power status. Stopping update" << endl;
    throw "get " + fru_name + " power status failed";
  }
  if (status == SERVER_POWER_OFF) {
    cerr << "Can't update FW version since the server is off!" << endl;
    throw fru_name + " power off";
  }
  if (!pal_is_slot_support_update(FRU_SCM)) {
    cerr << "slot" << _slot_id << " is not a server" << endl;
    throw "fru " + fru_name + " is not a server";
  }
}
