#include <cstdio>
#include <cstring>
#include "bic_fw.h"

using namespace std;

int BicFwComponent::update(string) { return FW_STATUS_NOT_SUPPORTED; }
int BicFwComponent::fupdate(string) { return FW_STATUS_NOT_SUPPORTED; }
int BicFwComponent::get_version(json&) { return FW_STATUS_NOT_SUPPORTED; }
int BicFwBlComponent::update(string) { return FW_STATUS_NOT_SUPPORTED; }
int BicFwBlComponent::get_version(json&) { return FW_STATUS_NOT_SUPPORTED; }

