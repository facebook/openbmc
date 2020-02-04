/*
 * fw-util.cpp
 *
 * Copyright 2017-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <cstring>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <map>
#include <tuple>
#include <signal.h>
#include <getopt.h>
#include <syslog.h>
#include <vector>
#include "fw-util.h"

using namespace std;

string exec_name = "Unknown";
std::map<std::string, std::map<std::string, std::map<std::string, Component *>>> *Component::fru_list = NULL;

Component::Component(string fru, string board, string component)
  : _fru(fru), _board(board), _component(component)
{
  fru_list_setup();
  (*fru_list)[fru][board][component] = this;
}

Component::~Component()
{
  (*fru_list)[_fru][_board].erase(_component);
  (*fru_list)[_fru].erase(_board);
  if ((*fru_list)[_fru].size() == 0) {
    fru_list->erase(_fru);
  }
}

bool Component::is_fru_list_valid() {
  if (!fru_list) {
    cerr << "is fru list created?" << endl;
    return false;
  }
  return true;
}

bool Component::is_fru_valid(string fru) {
  if (fru_list->find(fru) == fru_list->end() && fru != "all" ) {
    if ( fru.empty() ) {
      cerr << "FRU is not set" << endl;
    } else {
      cerr << "Invalid fru: " << fru << endl;
    }
    return false;
  }
  return true;
}

bool Component::is_board_valid(string fru, string board) {
  if ((*fru_list)[fru].find(board) == (*fru_list)[fru].end()) {
    if ( board.empty() ) {
      cerr << "Board is not set" << endl;
    } else {
      cerr << "Invalid board: " << board << endl;
    }
    return false;
  }
  return true;
}

bool Component::is_component_valid(string fru, string board, string comp) {
  if ((*fru_list)[fru][board].find(comp) == (*fru_list)[fru][board].end()) {
    if ( comp.empty() ) {
      cerr << "a component or an action is not set" << endl;
    } else {
      cerr << "Invalid component: " << comp << endl;
    }
    return false;
  }
  return true;
}

Component *Component::get_component(string fru, string board, string comp)
{
  return (*fru_list)[fru][board][comp];
}

class ProcessLock {
  private:
    string file;
    int fd;
    uint8_t _fru;
    bool _ok;
    bool _terminate;
  public:
  ProcessLock() {
    _terminate = false;
  }

  void lock_file(uint8_t fru, string file){
    _ok = false;
    _fru = fru;
    fd = open(file.c_str(), O_RDWR|O_CREAT, 0666);
    if (fd < 0) {
      return;
    }
    if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
      close(fd);
      fd = -1;
      return;
    }
    _ok = true;
  }

  void setTerminate(bool terminate) {
    _terminate = terminate;
  }

  bool getTerminate() {
    return _terminate;
  }

  uint8_t getFru() {
    return _fru;
  }

  ~ProcessLock() {
    if (_ok) {
      remove(file.c_str());
      close(fd);
    }
  }

  bool ok() {
    return _ok;
  }
};

ProcessLock * plock = new ProcessLock();

void fw_util_sig_handler(int signo) {
  if (plock) {
    plock->setTerminate(true);
    printf("Terminated requested. Waiting for earlier operation complete.\n");
    syslog(LOG_DEBUG, "slot%u fw_util_sig_handler signo=%d",plock->getFru(), signo);
  }
}

void usage()
{
  cout << "USAGE: " << exec_name << " all|FRU --version" << endl;
  cout << "       " << exec_name << " FRU BOARD --update [--]COMPONENT IMAGE_PATH" << endl;
  cout << "       " << exec_name << " FRU BOARD --force --update [--]COMPONENT IMAGE_PATH" << endl;
  cout << "FRU    : Board        : Components" << endl;
  cout << "-------:--------------:-------------"<< endl;
  cout << "bmc    :bmc           :bmc cpld rom "<< endl;
  cout << "nic    :nic           :nic" << endl;
  cout << "slot1  :bb sb 1ou 2ou :bic bicbl bios cpld me vr" << endl;
  cout << "slot2  :sb 1ou 2ou    :bic bicbl bios cpld me vr" << endl;
  cout << "slot3  :sb 1ou 2ou    :bic bicbl bios cpld me vr" << endl;
  cout << "slot4  :sb 1ou 2ou    :bic bicbl bios cpld me vr" << endl;

  if (plock)
    delete plock;
}

#define BIT(x) (1 << x)
#define FW_UPDATE   BIT(0)
#define FW_VERSION  BIT(1)
#define FW_SCHEDULE_ADD  BIT(2)
#define FW_SCHEDULE_DEL  BIT(3)
#define FW_SCHEDULE_SHOW BIT(4)

struct args_t {
  string fru;
  string board;
  bool force;
  uint8_t action;
  string component;
  string image;
};

void parse_arg(int argc, char **argv, struct args_t &param) {
  int idx = 0;
  int args_size = 0;
  std::vector<std::string> all_args;

  if ( argc < 3 ) {
    cerr << "The length of params are invalid!" << endl;
    usage();
    exit(EXIT_FAILURE);
  }

  //convert it to the string
  all_args.assign(argv + 1, argv + argc);
  args_size = all_args.size();

  //get the fru
  param.fru = all_args[idx++];

  ///Suppose you want to get the firmware version
  if ( args_size == 2 && all_args[idx] == "--version" ) {
      param.action = FW_VERSION;
  } else {
    param.board = all_args[idx++];
    while ( idx < args_size ) {
      if ( all_args[idx] == "--update" ) {
        param.action |= FW_UPDATE;
        idx++;

        if ( idx >= args_size ) break;

        //check that the data are values or options
        if ( all_args[idx] == "--del" && all_args[idx] == "--add" && all_args[idx] == "--list" ) {
          continue;
        }

        if ( idx + 1 < args_size ) {
          param.component = all_args[idx++];
          if ( param.component.rfind("--", 0) == 0 )
            param.component = param.component.substr(2);
          param.image = all_args[idx];
        } else {
          //only get one option, suppose it to a component
          param.component = all_args[idx];
          if ( param.component.rfind("--", 0) == 0 )
            param.component = param.component.substr(2);
        }
      } else if ( all_args[idx] == "--force" ) {
        param.force = true;
      } else if ( all_args[idx] == "--add" ) {
        cout << "--add is not supported" << endl;
      } else if ( all_args[idx] == "--del" ) {
        cout << "--del is not supported" << endl;
      } else if ( all_args[idx] == "--list" ) {
        cout << "--list is not supported" << endl;
      }
      idx++;
    }
  }
}

void show_fw_version(string target) {
  for ( auto fru : (*Component::fru_list) ) {
    if ( target == "all" || target == fru.first ) {
      for ( auto board:fru.second ) {
        for ( auto comp:board.second ) {
          comp.second->print_version();

          if (plock && plock->getTerminate()) {
            // Do not call destructor explicitly, since it's an undefined behaviour.
            // While the constructor uses new, and the destructor uses delete.
            syslog(LOG_DEBUG, "slot%u Terminated complete.",plock->getFru());
            printf("Terminated complete.\n");
            delete plock;
            cout << endl;
          }
        }
      }
      cout << endl;
    }
  }
}

int do_fw_update(string fru, string board, string comp, string image, bool force) {

  Component *comp_hndler = Component::get_component(fru, board, comp);

  if ( comp_hndler == NULL ) {
    cerr << "Cannot be NULL here" << endl;
    return FW_STATUS_NOT_SUPPORTED;
  }

  if ( force == true ) {
    return comp_hndler->fupdate(image);
  } 

  return comp_hndler->update(image);
}

int main(int argc, char *argv[])
{
  struct args_t fw_args{};
  struct sigaction sa;
  int ret = FW_STATUS_NOT_SUPPORTED;
  uint8_t fru_id = 0;
  System system;

  exec_name = argv[0];

  parse_arg(argc, argv, fw_args);

#if 0
  cout << "fru " << fw_args.fru << endl;
  cout << "board " << fw_args.board << endl;
  cout << "force " << +fw_args.force << endl;
  cout << "action " << +fw_args.action << endl;
  cout << "comp " << fw_args.component << endl;
  cout << "img " << fw_args.image << endl;
#endif

  sa.sa_handler = fw_util_sig_handler;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGHUP, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGQUIT, &sa, NULL);
  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGPIPE, &sa, NULL); // for ssh terminate

  //check the fru list is created
  if ( Component::is_fru_list_valid() == false ) {
    exit(EXIT_FAILURE);
  }

  //check the fru is valid
  if ( Component::is_fru_valid(fw_args.fru) == false && fw_args.fru != "all" ) {
    usage();
    exit(EXIT_FAILURE);
  }

  //if user want to get the version of FWs, we check the fru only.
  if ( fw_args.action == FW_VERSION ) {
    //show version
    show_fw_version(fw_args.fru);
    return 0; 
  } else if ( fw_args.fru != "all" ) {
    //we don't support to update ALL components
    //check the other params further
    if ( Component::is_board_valid(fw_args.fru, fw_args.board) == false || \
         Component::is_component_valid(fw_args.fru, fw_args.board, fw_args.component) == false ) {
      usage();
      exit(EXIT_FAILURE);
    }
  } else {
    cerr << "Invalid length parameter" << endl;
    usage();
    exit(EXIT_FAILURE);
  }

  string str_act("");

  //valid the aciton
  switch (fw_args.action) {
    case FW_UPDATE:
      {
        ifstream f(fw_args.image);
        if ( fw_args.image.empty() ||  !f.good() ) {
          cerr << "The image path is empty or it cannot be accessed" << endl;
          usage();
          exit(EXIT_FAILURE);
        }

        if ( fw_args.force == true ) {
          str_act.assign("Force upgrade");
        } else {
          str_act.assign("Upgrade");
        }
        fru_id = system.get_fru_id(fw_args.fru);
        system.set_update_ongoing(fru_id, 60 * 10);
        ret = do_fw_update(fw_args.fru, fw_args.board, fw_args.component, fw_args.image, fw_args.force); 
        system.set_update_ongoing(fru_id, 0);
      }
      break;
    case FW_SCHEDULE_ADD:
    case FW_SCHEDULE_DEL:
    case FW_SCHEDULE_SHOW:
    default:
      cerr << "Invalid action!" << endl;
      usage();
      exit(EXIT_FAILURE);
      break;
  }

  if (ret == FW_STATUS_SUCCESS) {
    cout << str_act << " of " << fw_args.fru << " : " << fw_args.board << " : " << fw_args.component << " succeeded" << endl;
  } else {
    cerr << str_act << " of " << fw_args.fru << " : " << fw_args.board << " : " << fw_args.component;
    if (ret == FW_STATUS_NOT_SUPPORTED) {
      cerr << " not supported" << endl;
    } else {
      cerr << " failed" << endl;
    }
 }

  return ret;
}
