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
#include <syslog.h>
#include <atomic>
#include <openbmc/pal.h>
#ifdef __TEST__
#include <gtest/gtest.h>
#endif
#include "fw-util.h"

using namespace std;

std::atomic<bool> quit_process(false);

string exec_name = "Unknown";
map<string, map<string, Component *>> * Component::fru_list = NULL;

Component::Component(string fru, string component)
  : _fru(fru), _component(component), sys(), update_initiated(false)
{
  fru_list_setup();
  (*fru_list)[fru][component] = this;
}

Component::~Component()
{
  (*fru_list)[_fru].erase(_component);
  if ((*fru_list)[_fru].size() == 0) {
    fru_list->erase(_fru);
  }
  // Clear the update-ongoing flag if we were the ones
  // setting it.
  if (update_initiated)
    set_update_ongoing(0);
}

Component *Component::find_component(string fru, string comp)
{
  if (!fru_list) {
    return NULL;
  }
  if (fru_list->find(fru) == fru_list->end()) {
    return NULL;
  }
  if ((*fru_list)[fru].find(comp) == (*fru_list)[fru].end()) {
    return NULL;
  }
  return (*fru_list)[fru][comp];
}

AliasComponent::AliasComponent(string fru, string comp, string t_fru, string t_comp) :
  Component(fru, comp), _target_fru(t_fru), _target_comp_name(t_comp), _target_comp(NULL)
{
  // This might not be successful this early.
  _target_comp = find_component(t_fru, t_comp);
}

bool AliasComponent::setup()
{
  if (!_target_comp) {
    _target_comp = find_component(_target_fru, _target_comp_name);
    if (!_target_comp) {
      return false;
    }
  }
  return true;
}

int AliasComponent::update(string image)
{
  if (!setup())
    return FW_STATUS_NOT_SUPPORTED;
  return _target_comp->update(image);
}

int AliasComponent::fupdate(string image)
{
  if (!setup())
    return FW_STATUS_NOT_SUPPORTED;
  return _target_comp->fupdate(image);
}

int AliasComponent::dump(string image)
{
  if (!setup())
    return FW_STATUS_NOT_SUPPORTED;
  return _target_comp->dump(image);
}

int AliasComponent::print_version()
{
  if (!setup())
    return FW_STATUS_NOT_SUPPORTED;
  return _target_comp->print_version();
}

void AliasComponent::set_update_ongoing(int timeout)
{
  if (setup())
    _target_comp->set_update_ongoing(timeout);
}

bool AliasComponent::is_update_ongoing()
{
  if (!setup())
    return FW_STATUS_NOT_SUPPORTED;
  return _target_comp->is_update_ongoing();
}

void fw_util_sig_handler(int signo)
{
  quit_process.store(true);
  cout << "Terminated requested. Waiting for earlier operation complete.\n";
  syslog(LOG_DEBUG, "fw_util_sig_handler signo=%d requesting exit", signo);
}

void usage()
{
  cout << "USAGE: " << exec_name << " all|FRU --version [all|COMPONENT]" << endl;
  cout << "       " << exec_name << " FRU --update [--]COMPONENT IMAGE_PATH" << endl;
  cout << "       " << exec_name << " FRU --force --update [--]COMPONENT IMAGE_PATH" << endl;
  cout << "       " << exec_name << " FRU --dump [--]COMPONENT IMAGE_PATH" << endl;
  cout << left << setw(10) << "FRU" << " : Components" << endl;
  cout << "---------- : ----------" << endl;
  for (auto fkv : *Component::fru_list) {
    string fru_name = fkv.first;
    cout << left << setw(10) << fru_name << " : ";
    for (auto ckv : fkv.second) {
      string comp_name = ckv.first;
      Component *c = ckv.second;
      if (comp_name.find(" ") == comp_name.npos) {
        // No spaces in the component name, print as-is.
        cout << comp_name;
      } else {
        // Add quotes to ensure there is no confusion from usage.
        cout << "\"" << comp_name << "\"";
      }
      if (c->is_alias()) {
        AliasComponent *a = (AliasComponent *)c;
        cout << "(" << a->alias_fru() << ":" << a->alias_component() << ")";
      }
      cout << " ";
    }
    cout << endl;
  }
}

int main(int argc, char *argv[])
{
  int ret = 0;
  int find_comp = 0;
  struct sigaction sa;

  Component::fru_list_setup();

#ifdef __TEST__
  testing::InitGoogleTest(&argc, argv);
  ret = RUN_ALL_TESTS();
  if (ret != 0) {
    return ret;
  }
#endif
  exec_name = argv[0];
  if (argc < 3) {
    usage();
    return -1;
  }

  string fru(argv[1]);
  string action(argv[2]);
  string component("all");
  string image("");

  if (action == "--force") {
    if (argc < 4) {
      usage();
      return -1;
    }
    string action_ext(argv[3]);
    if (action_ext != "--update") {
      usage();
      return -1;
    }
    if (argc >= 5) {
      component.assign(argv[4]);
      if (component.compare(0, 2, "--") == 0) {
        component = component.substr(2);
      }
    }
  } else {
    if (argc >= 4) {
      component.assign(argv[3]);
      if (component.compare(0, 2, "--") == 0) {
        component = component.substr(2);
      }
    }
  }

  if ((action == "--update") || (action == "--dump")) {
    if (argc != 5) {
      usage();
      return -1;
    }
    image.assign(argv[4]);
    if (action == "--update") {
      ifstream f(image);
      if (!f.good()) {
        cerr << "Cannot access: " << image << endl;
        return -1;
      }
    } 
    if (component == "all") {
      cerr << "Upgrading all components not supported" << endl;
      return -1;
    }
  } else if (action == "--force") {
    if (argc != 6) {
      usage();
      return -1;
    }
    image.assign(argv[5]);
    ifstream f(image);
    if (!f.good()) {
      cerr << "Cannot access: " << image << endl;
      return -1;
    }
    if (component == "all") {
      cerr << "Upgrading all components not supported" << endl;
      return -1;
    }
  } else if (action == "--version") {
    if(argc > 4) {
      usage();
      return -1;
    }
  } else {
    cerr << "Invalid action: " << action << endl;
    usage();
    return -1;
  }

  sa.sa_handler = fw_util_sig_handler;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGHUP, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGQUIT, &sa, NULL);
  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGPIPE, &sa, NULL); // for ssh terminate

  for (auto fkv : *Component::fru_list) {
    if (fru == "all" || fru == fkv.first) {
      for (auto ckv : fkv.second) {
        string comp_name = ckv.first;
        if (component == "all" || component == comp_name) {
          find_comp = 1;
          Component *c = ckv.second;

          // Ignore aliases if their target is going to be
          // considered in one of the loops.
          if (c->is_alias()) {
            if (component == "all" && (fru == "all" || fru == c->alias_fru()))
              continue;
            if (fru == "all" && component == c->alias_component())
              continue;
          }

          if (c->is_update_ongoing()) {
            cerr << "Upgrade aborted due to ongoing upgrade on FRU: " << c->fru() << endl;
            return -1;
          }

          if (action == "--version") {
            ret = c->print_version();
            if (ret != FW_STATUS_SUCCESS && ret != FW_STATUS_NOT_SUPPORTED) {
              cerr << "Error getting version of " << c->component()
                << " on fru: " << c->fru() << endl;
            }
          } else {  // update or dump
            if (fru == "all") {
              usage();
              return -1;
            }
            string str_act("");
            c->set_update_ongoing(60 * 10);
            if (action == "--update") {
              ret = c->update(image);
              str_act.assign("Upgrade");
            } else if (action == "--force") {
              ret = c->fupdate(image);
              str_act.assign("Force upgrade");
            } else {
              ret = c->dump(image);
              str_act.assign("Dump");
            }
            c->set_update_ongoing(0);
            if (ret == 0) {
              cout << str_act << " of " << c->fru() << " : " << component << " succeeded" << endl;
            } else {
              cerr << str_act << " of " << c->fru() << " : " << component;
              if (ret == FW_STATUS_NOT_SUPPORTED) {
                cerr << " not supported" << endl;
              } else {
                cerr << " failed" << endl;
              }
            }
          }

          if (quit_process.load()) {
            syslog(LOG_DEBUG, "fw-util: Terminate request handled");
            cout << "Aborted action due to signal\n";
            return -1;
          }
        }
      }
    }
  }

  if (!find_comp) {
    usage();
    return -1;
  }

  return 0;
}
