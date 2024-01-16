#include <cstdio>
#include <cstring>
#include <stdlib.h>
#include "bic_cpld.h"

using namespace std;

int CpldComponent::get_version(json&) { return FW_STATUS_NOT_SUPPORTED; }
int CpldComponent::update(string) { return FW_STATUS_NOT_SUPPORTED; }
