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
#include <unordered_map>
#include <map>
#include <tuple>
#include "fw-util.h"
#include <openbmc/pal.h>

using namespace std;

string exec_name = "Unknown";
ComponentList Component::registered_components[MAX_COMPONENTS] = {0};
ComponentAliasList Component::registered_aliases[MAX_COMPONENTS] = {0};
map<string, map<string, Component *>> Component::fru_list;

Component::Component(string fru, string component)
  : _fru(fru), _component(component)
{
  for (int i = 0; i < MAX_COMPONENTS; ++i) {
    if (registered_components[i].obj == NULL) {
      ComponentList *c = &registered_components[i];
      strncpy(c->fru, fru.c_str(), sizeof(c->fru));
      strncpy(c->comp, component.c_str(), sizeof(c->comp));
      c->fru[sizeof(c->fru) - 1] = c->comp[sizeof(c->comp) - 1] = '\0';
      c->obj  = this;
      break;
    }
  }
}

Component::Component(string fru, string component,
    string real_fru, string real_component)
  : _fru(fru), _component(component)
{
  for (int i = 0; i < MAX_COMPONENTS; ++i) {
    if (registered_aliases[i].fru[0] == '\0') {
      ComponentAliasList *c = &registered_aliases[i];
      strncpy(c->fru, fru.c_str(), sizeof(c->fru));
      strncpy(c->comp, component.c_str(), sizeof(c->comp));
      strncpy(c->real_fru, real_fru.c_str(), sizeof(c->real_fru));
      strncpy(c->real_comp, real_component.c_str(), sizeof(c->real_comp));
      c->fru[sizeof(c->fru) - 1] = c->comp[sizeof(c->comp) - 1] = '\0';
      c->real_fru[sizeof(c->real_fru) - 1] = '\0';
      c->real_comp[sizeof(c->real_comp) - 1] = '\0';
      break;
    }
  }
}

// Once initialization of static objects are complete, we can
// call this once in main to throw everything in registered_components
// into a map data structure for ease of use.
void Component::populateFruList()
{
  for (int i = 0; i < MAX_COMPONENTS; ++i) {
    ComponentList *c = &registered_components[i];
    if (c->obj == nullptr) {
      break;
    }
    fru_list[std::string(c->fru)][std::string(c->comp)] = c->obj;
    c->fru[0] = c->comp[0] = '\0';
    c->obj = nullptr;
  }
  for (int i = 0; i < MAX_COMPONENTS; ++i) {
    ComponentAliasList *c = &registered_aliases[i];
    if (c->fru[0] == '\0') {
      break;
    }
    string fru(c->fru);
    string comp(c->comp);
    string real_fru(c->real_fru);
    string real_comp(c->real_comp);
    if (fru_list.find(real_fru) != fru_list.end() &&
        fru_list[real_fru].find(real_comp) != fru_list[real_fru].end()) {
      fru_list[fru][comp] = fru_list[real_fru][real_comp];
    } else {
      cerr << "Ignoring alias (" << fru << "," << comp << ") to unknown ("
        << real_fru << "," << real_comp << ")" << endl;
    }
  }
}

void usage()
{
  cout << "USAGE: " << exec_name << " all|FRU --version [all|COMPONENT]" << endl;
  cout << "       " << exec_name << " FRU --update  [--]COMPONENT IMAGE_PATH" << endl;
  cout << left << setw(10) << "FRU" << " : Components" << endl;
  cout << "---------- : ----------" << endl;
  for (auto fkv : Component::fru_list) {
    string fru_name = fkv.first;
    cout << left << setw(10) << fru_name << " : ";
    for (auto ckv : fkv.second) {
      string comp_name = ckv.first;
      Component *c = ckv.second;
      cout << comp_name;
      if (c->component() != comp_name || c->fru() != fru_name) {
        cout << "(" << c->fru() << ":" << c->component() << ")";
      }
      cout << " ";
    }
    cout << endl;
  }
}

int main(int argc, char *argv[])
{
  int ret = 0;

  exec_name = argv[0];
  Component::populateFruList();
  
  if (argc < 3) {
    usage();
    return -1;
  }

  string fru(argv[1]);
  string action(argv[2]);
  string component("all");
  if (argc >= 4) {
    component.assign(argv[3]);
    if (component.compare(0, 2, "--") == 0) {
      component = component.substr(2);
    }
  }
  if (action == "--update") {
    uint8_t fru_id;

    if (argc < 5) {
      usage();
      return -1;
    }
    string image(argv[4]);

    if (Component::fru_list.find(fru) == Component::fru_list.end()) {
      cerr << "Invalid FRU: " << fru << endl;
      return -1;
    }
    if (Component::fru_list[fru].find(component) == Component::fru_list[fru].end()) {
      cerr << "Invalid component: " << component << endl;
      return -1;
    }
    {
      ifstream f(image);
      if (!f.good()) {
        cerr << "Cannot access: " << image << endl;
        return -1;
      }
    }
    if (pal_get_fru_id((char *)fru.c_str(), &fru_id)) {
      // Set to some default FRU which should be present
      // in the system.
      fru_id = 1;
    }
    pal_set_fw_update_ongoing(fru_id, 60 * 10);
    ret = Component::fru_list[fru][component]->update(image);
    pal_set_fw_update_ongoing(fru_id, 0);
    if (ret == 0) {
      cout << "Upgrade of " << component << " succeeded" << endl;
    } else if (ret == FW_STATUS_NOT_SUPPORTED) {
      cerr << "Upgrade of " << component << " not supported" << endl;
    } else {
      cerr << "Upgrade of " << component << " failed" << endl;
    }
  } else if (action == "--version") {
    unordered_map<Component *,bool> done_map;
    for (auto fkv : Component::fru_list) {
      if (fru == "all" || fru == fkv.first) {
        for (auto ckv : Component::fru_list[fkv.first]) {
          if (component == "all" || component == ckv.first) {
            Component *c = Component::fru_list[fkv.first][ckv.first];
            if (done_map.find(c) == done_map.end()) {
              ret = c->print_version();
              done_map[c] = true;
              if (ret != FW_STATUS_SUCCESS && ret != FW_STATUS_NOT_SUPPORTED) {
                cerr << "Error getting version of " << c->component() 
                  << " on fru: " << c->fru() << endl;
              }
            }
          }
        }
      }
    }
    // Since not all components supports getting a version.
    if (fru == "all" || component == "all") {
      ret = 0;
    }
  } else {
    cerr << "Invalid action: " << action << endl;
    usage();
    return -1;
  }
  return ret;
}
