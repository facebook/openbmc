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
  const char *server_err_str[SERVER_ERR_TYPE_CNT] = {
    "get %s present failed", "%s is empty", "get %s power status failed", "%s power 12V-off", "fru %d is not a %s",
  };

  ret = pal_is_fru_prsnt(_slot_id, &status);
  if (ret < 0) {
  #ifdef DEBUG
    cerr << "pal_is_fru_prsnt failed for fru " << _slot_id << endl;
  #endif
    sprintf(_err_str, server_err_str[0], _fru_name);
    return false;
  }
  if (status == 0) {
#ifdef DEBUG
    cerr << "slot" << _slot_id << " is empty!" << endl;
#endif
    sprintf(_err_str, server_err_str[1], _fru_name);
    return false;
  }
  ret = pal_get_server_power(_slot_id, &status);
  if (ret < 0) {
#ifdef DEBUG
    cerr << "Failed to get server power status. Stopping update" << endl;
#endif
    sprintf(_err_str, server_err_str[2], _fru_name);
    return false;
  }
  if (status == SERVER_12V_OFF) {
#ifdef DEBUG
    cerr << "Can't update FW version since the server is 12V off!" << endl;
#endif
    sprintf(_err_str, server_err_str[3], _fru_name);
    return false;
  }
  if (!pal_is_slot_server(_slot_id)) {
#ifdef DEBUG
    cerr << "slot" << _slot_id << " is not a server" << endl;
#endif
    sprintf(_err_str, server_err_str[4], _slot_id, _fru_name);
    return false;
  }
  return true;
}
