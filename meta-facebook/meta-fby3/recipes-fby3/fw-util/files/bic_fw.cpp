#include <cstdio>
#include <cstring>
#include "bic_fw.h"

using namespace std;

int BicFwComponent::update(string image) { return FW_STATUS_NOT_SUPPORTED; }
int BicFwComponent::fupdate(string image) { return FW_STATUS_NOT_SUPPORTED; }
int BicFwComponent::print_version() { return FW_STATUS_NOT_SUPPORTED; }
int BicFwBlComponent::update(string image) { return FW_STATUS_NOT_SUPPORTED; }
int BicFwBlComponent::print_version() { return FW_STATUS_NOT_SUPPORTED; }

