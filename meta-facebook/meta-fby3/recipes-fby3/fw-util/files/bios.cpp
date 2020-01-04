#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include <syslog.h>
#include <unistd.h>
#include "bios.h"
#include <openbmc/pal.h>
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

#define MAX_RETRY 5
#define MUX_SWITCH_CPLD 0x07
#define MUX_SWITCH_PCH 0x03
#define GPIO_RST_USB_HUB 0x10
#define GPIO_HIGH 1
#define GPIO_LOW 0

int BiosComponent::update(string image) {
  int ret;
  uint8_t status;
  int retry_count = 0;

  try {
    server.ready();
    pal_set_server_power(slot_id, SERVER_GRACEFUL_SHUTDOWN);

    //Checking Server Power Status to make sure Server is really Off
    while (retry_count < MAX_RETRY) {
      ret = pal_get_server_power(slot_id, &status);
      if ( (ret == 0) && (status == SERVER_POWER_OFF) ){
        break;
      } else {
        retry_count++;
        sleep(1);
      }
    }
    if (retry_count == MAX_RETRY) {
      cerr << "Failed to Power Off Server " << slot_id << ". Stopping the update!" << endl;
      return -1;
    }

    me_recovery(slot_id, RECOVERY_MODE);
    if (slot_id == FRU_SLOT1) {
      bic_set_gpio(FRU_SLOT2, GPIO_RST_USB_HUB, GPIO_LOW);
      bic_set_gpio(FRU_SLOT3, GPIO_RST_USB_HUB, GPIO_LOW);
      bic_set_gpio(FRU_SLOT4, GPIO_RST_USB_HUB, GPIO_LOW);
      sleep(3);
    } else if (slot_id == FRU_SLOT2) {
      bic_set_gpio(FRU_SLOT1, GPIO_RST_USB_HUB, GPIO_LOW);
      bic_set_gpio(FRU_SLOT3, GPIO_RST_USB_HUB, GPIO_LOW);
      bic_set_gpio(FRU_SLOT4, GPIO_RST_USB_HUB, GPIO_LOW);
      sleep(3);
    } else if (slot_id == FRU_SLOT3) {
      bic_set_gpio(FRU_SLOT1, GPIO_RST_USB_HUB, GPIO_LOW);
      bic_set_gpio(FRU_SLOT2, GPIO_RST_USB_HUB, GPIO_LOW);
      bic_set_gpio(FRU_SLOT4, GPIO_RST_USB_HUB, GPIO_LOW);
      sleep(3);
    } else if (slot_id == FRU_SLOT4) {
      bic_set_gpio(FRU_SLOT1, GPIO_RST_USB_HUB, GPIO_LOW);
      bic_set_gpio(FRU_SLOT2, GPIO_RST_USB_HUB, GPIO_LOW);
      bic_set_gpio(FRU_SLOT3, GPIO_RST_USB_HUB, GPIO_LOW);
      sleep(3);
    }
    bic_switch_mux_for_bios_spi(slot_id, MUX_SWITCH_CPLD);
    sleep(1);
    ret = bic_update_fw(slot_id, UPDATE_BIOS, intf, (char *)image.c_str(), 1);
    sleep(1);
    pal_set_server_power(slot_id, SERVER_12V_CYCLE);
    sleep(5);
    pal_set_server_power(slot_id, SERVER_POWER_ON);

    if (slot_id == FRU_SLOT1) {
      bic_set_gpio(FRU_SLOT2, GPIO_RST_USB_HUB, GPIO_HIGH);
      bic_set_gpio(FRU_SLOT3, GPIO_RST_USB_HUB, GPIO_HIGH);
      bic_set_gpio(FRU_SLOT4, GPIO_RST_USB_HUB, GPIO_HIGH);
      sleep(3);
    } else if (slot_id == FRU_SLOT2) {
      bic_set_gpio(FRU_SLOT1, GPIO_RST_USB_HUB, GPIO_HIGH);
      bic_set_gpio(FRU_SLOT3, GPIO_RST_USB_HUB, GPIO_HIGH);
      bic_set_gpio(FRU_SLOT4, GPIO_RST_USB_HUB, GPIO_HIGH);
      sleep(3);
    } else if (slot_id == FRU_SLOT3) {
      bic_set_gpio(FRU_SLOT1, GPIO_RST_USB_HUB, GPIO_HIGH);
      bic_set_gpio(FRU_SLOT2, GPIO_RST_USB_HUB, GPIO_HIGH);
      bic_set_gpio(FRU_SLOT4, GPIO_RST_USB_HUB, GPIO_HIGH);
      sleep(3);
    } else if (slot_id == FRU_SLOT4) {
      bic_set_gpio(FRU_SLOT1, GPIO_RST_USB_HUB, GPIO_HIGH);
      bic_set_gpio(FRU_SLOT2, GPIO_RST_USB_HUB, GPIO_HIGH);
      bic_set_gpio(FRU_SLOT3, GPIO_RST_USB_HUB, GPIO_HIGH);
      sleep(3);
    }

  } catch(string err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int BiosComponent::fupdate(string image) {
  return FW_STATUS_NOT_SUPPORTED;
}

int BiosComponent::print_version() {
  uint8_t ver[32];
  uint8_t fruid = 0;
  int ret = 0;
  try {
    server.ready();
    ret = pal_get_fru_id((char *)_fru.c_str(), &fruid);
    if ( ret < 0 ) {
      throw "get " + _fru + " fru id failed";
    }

    ret = pal_get_sysfw_ver(fruid, ver);
    // Print BIOS Version
    if ( ret < 0 ) {
      cout << "BIOS Version: NA" << endl;
    } else {
      printf("BIOS Version: ");
      cout << &ver[3] << endl;
    }
  } catch(string err) {
    printf("BIOS Version: NA (%s)\n", err.c_str());
  }

  return ret;
}

#endif
