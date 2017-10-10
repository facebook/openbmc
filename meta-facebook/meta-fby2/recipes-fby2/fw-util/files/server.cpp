#include <iostream>
#include <cstdio>
#include <cstring>
#include <openbmc/pal.h>
#include <facebook/bic.h>
#include "server.h"

using namespace std;

bool Server::ready()
{
  int ret;
  uint8_t status;

  ret = pal_is_fru_prsnt(_slot_id, &status);
  if (ret < 0) {
    cerr << "pal_is_fru_prsnt failed for fru " << _slot_id << endl;
    return false;
  }
  if (status == 0) {
    cerr << "slot" << _slot_id << " is empty!" << endl;
    return false;
  }
  ret = pal_is_server_12v_on(_slot_id, &status);
  if (ret < 0 || 0 == status) {
    cerr << "slot" << _slot_id << " 12V is off" << endl;
    return false;
  }
  if (!pal_is_slot_server(_slot_id)) {
    cerr << "slot" << _slot_id << " is not a server" << endl;
    return false;
  }
  return true;
}
