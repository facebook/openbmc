/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specificaton available @
 * http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-specifications.html
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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <openbmc/libgpio.h>

#define SCU_BASE        0x1E6E2000
#define REG_SCU08       0x08
#define REG_SCU10       0x10
#define REG_SCU70       0x70       
#define REG_SCU80       0x80
#define REG_SCU84       0x84
#define REG_SCU88       0x88
#define REG_SCU8C       0x8C
#define REG_SCU90       0x90
#define REG_SCU94       0x94
#define REG_SCU9C       0x9C
#define REG_SCU2C       0x2C
#define REG_SCUA0       0xA0
#define REG_SCUA4       0xA4
#define REG_SCUA8       0xA8

#define WDT_BASE        0x1E785000
#define REG_WDT1_RESET  0x1C

#define JTAG_BASE       0x1E6E4000
#define JTAG_ENG_CTRL   0x08

#define GPIO_BASE               0x1E780000
#define GPIO_DEBOUNCE_TIMER1    0x50
#define GPIO_QRST_DEBOUNCE_REG1 0x130
#define GPIO_QRST_DEBOUNCE_REG2 0x134
#define PAGE_SIZE   	0x1000

uint32_t
get_register(void *addr) {
  return *(volatile uint32_t*) addr;
}

void
set_register(void *addr, uint32_t value) {
  *(volatile uint32_t*) addr = value;
}

int open_mem()  {
  int fd = 0;

  fd = open("/dev/mem", O_RDWR | O_SYNC );
  if (fd < 0) {
    syslog(LOG_ERR, "%s - cannot open /dev/mem", __FILE__);
  }
  return fd;
}


void *mapping_addr(int fd, uint32_t base) {
  return mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, base);
}

void close_mem(int fd) { 
  if ( fd > 0 ) close(fd);
}

struct gpio_attr{
  char *name;
  char *shadow;
  gpio_direction_t dir;
  gpio_value_t val;
} gpio_list[] = {
  {"GPIOD1", "FM_BOARD_REV_ID0",    GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
  {"GPIOD3", "FM_BOARD_REV_ID1",    GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
  {"GPIOD5", "FM_BOARD_REV_ID2",    GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
  {"GPIOE2", "FM_PWR_BTN_N",        GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
  {"GPIOE3", "FM_BMC_PWRBTN_OUT_N", GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
  {"GPIOB6", "PWRGD_SYS_PWROK", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOH0", "LED_POST_CODE_0", GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
  {"GPIOH1", "LED_POST_CODE_1", GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
  {"GPIOH2", "LED_POST_CODE_2", GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
  {"GPIOH3", "LED_POST_CODE_3", GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
  {"GPIOH4", "LED_POST_CODE_4", GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
  {"GPIOH5", "LED_POST_CODE_5", GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
  {"GPIOH6", "LED_POST_CODE_6", GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
  {"GPIOH7", "LED_POST_CODE_7", GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
  {"GPIOQ6", "FM_POST_CARD_PRES_BMC_N", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOU5", "BMC_FAULT_N",       GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
  {"GPIOAA2", "SERVER_POWER_LED", GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
  {"GPIOS1", "BMC_READY_N",       GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
  {"GPIOF6", "FM_BATTERY_SENSE_EN_N",       GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
  {"GPIOD0", "FM_BIOS_MRC_DEBUG_MSG_DIS_N", GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
  {"GPIOP0", "FM_BOARD_SKU_ID0", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOP1", "FM_BOARD_SKU_ID1", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOP2", "FM_BOARD_SKU_ID2", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOP3", "FM_BOARD_SKU_ID3", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOP4", "FM_BOARD_SKU_ID4", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOR6", "FM_BOARD_SKU_ID5", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOR7", "FM_BOARD_SKU_ID6", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOS3", "HSC_SMBUS_SWITCH_EN",  GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
  {"GPIOV3", "FM_FAST_PROCHOT_EN_N", GPIO_DIRECTION_OUT, GPIO_VALUE_LOW},
  {"GPIOM3", "FM_OC_DETECT_EN_N",    GPIO_DIRECTION_OUT, GPIO_VALUE_LOW},
  {"GPIOAA5", "UV_HIGH_SET",         GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
  {"GPIOG2", "FM_PCH_BMC_THERMTRIP_N",  GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
  {"GPIOG7", "FM_BIOS_SMI_ACTIVE_N",    GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
  {"GPIOL5", "FM_PMBUS_ALERT_BUF_EN_N", GPIO_DIRECTION_OUT, GPIO_VALUE_LOW},
  {"GPIOE6", "FM_CPU0_PROCHOT_LVT3_BMC_N", GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
  {"GPIOE7", "FM_CPU1_PROCHOT_LVT3_BMC_N", GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
  {"GPIOE1", "RST_BMC_SYSRST_BTN_OUT_N",   GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
  {"GPION3", "FM_CPU_MSMI_LVT3_N",   GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOG1", "FM_CPU_CATERR_LVT3_N", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOX3", "FM_GLOBAL_RST_WARN_N", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOX4", "H_CPU0_MEMABC_MEMHOT_LVT3_BMC_N", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOX5", "H_CPU0_MEMDEF_MEMHOT_LVT3_BMC_N", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOX6", "H_CPU1_MEMGHJ_MEMHOT_LVT3_BMC_N", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOX7", "H_CPU1_MEMKLM_MEMHOT_LVT3_BMC_N", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOP5", "BMC_PREQ_N",      GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
  {"GPIOP6", "BMC_PWR_DEBUG_N", GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
  {"GPIOP7", "RST_RSMRST_N",    GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
  {"GPIOB1", "BMC_DEBUG_EN_N",  GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
  {"GPIOR2", "BMC_TCK_MUX_SEL", GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
  {"GPIOR3", "BMC_PRDY_N",      GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOR4", "BMC_XDP_PRSNT_IN_N",     GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
  {"GPIOR5", "RST_BMC_PLTRST_BUF_N",   GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
  {"GPIOY2", "BMC_JTAG_SEL",           GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
  {"GPIOG3", "FM_CPU0_SKTOCC_LVT3_N",  GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
  {"GPIOAA0", "FM_CPU1_SKTOCC_LVT3_N", GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
  {"GPIOAA7", "FM_BIOS_POST_CMPLT_N",  GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
  {"GPIOA0", "BMC_CPLD_FPGA_SEL",      GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
  {"GPIOY1", "FM_SLPS4_N",             GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
  {"GPIOI3", "FM_UV_ADR_TRIGGER_EN", GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
  {"GPIOI2", "FM_FORCE_ADR_N",       GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
  {"GPIOD4", "IRQ_DIMM_SAVE_LVT3_N", GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
  {"GPIOAB1", "FM_OCP_MEZZA_PRES",   GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
  {"GPIOQ4", "FM_UARTSW_LSB_N",      GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
  {"GPIOQ5", "FM_UARTSW_MSB_N",      GPIO_DIRECTION_IN,  GPIO_VALUE_INVALID},
  {"GPIOB5", "BMC_PPIN",             GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH},
  {"GPIOB7", "IRQ_PVDDQ_GHJ_VRHOT_LVT3_N",   GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOD6", "FM_CPU_ERR0_LVT3_BMC_N",       GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOD7", "FM_CPU_ERR1_LVT3_BMC_N",       GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOE0", "RST_SYSTEM_BTN_N",             GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOE4", "FP_NMI_BTN_N",                 GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOF0", "IRQ_PVDDQ_ABC_VRHOT_LVT3_N",   GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOF2", "IRQ_PVCCIN_CPU0_VRHOT_LVC3_N", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOF3", "IRQ_PVCCIN_CPU1_VRHOT_LVC3_N", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOF4", "IRQ_PVDDQ_KLM_VRHOT_LVT3_N",   GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOG0", "FM_CPU_ERR2_LVT3_N",           GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOI0", "FM_CPU0_FIVR_FAULT_LVT3_N",    GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOI1", "FM_CPU1_FIVR_FAULT_LVT3_N",    GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOL0", "IRQ_UV_DETECT_N",              GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOL1", "IRQ_OC_DETECT_N",              GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOL2", "FM_HSC_TIMER_EXP_N",           GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOL4", "FM_MEM_THERM_EVENT_PCH_N",     GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOM0", "FM_CPU0_RC_ERROR_N",           GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOM1", "FM_CPU1_RC_ERROR_N",           GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOM4", "FM_CPU0_THERMTRIP_LATCH_LVT3_N", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOM5", "FM_CPU1_THERMTRIP_LATCH_LVT3_N", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOS0", "FM_THROTTLE_N",                  GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOZ2", "IRQ_PVDDQ_DEF_VRHOT_LVT3_N",     GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOAA1", "IRQ_SML1_PMBUS_ALERT_N",        GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPIOAB0", "IRQ_HSC_FAULT_N",               GPIO_DIRECTION_IN, GPIO_VALUE_INVALID},
  {"GPION5", "BMC_BIOS_FLASH_CTRL",            GPIO_DIRECTION_IN, GPIO_VALUE_INVALID}};
int ngpios = (sizeof(gpio_list)/sizeof(struct gpio_attr));

static int
get_gpio_shadow_array(const char **shadows, int num, uint8_t *mask) {
  int i;
  *mask = 0;
  for (i = 0; i < num; i++) {
    int ret;
    gpio_value_t value;
    gpio_desc_t *gpio = gpio_open_by_shadow(shadows[i]);
    if (!gpio) {
      return -1;
    }
    ret = gpio_get_value(gpio, &value);
    gpio_close(gpio);
    if (ret != 0) {
      return -1;
    }
    *mask |= (value == GPIO_VALUE_HIGH ? 1 : 0) << i;
  }
  return 0;
}

int
get_board_rev(uint8_t *board_rev) {
  const char *shadows[] = {
    "FM_BOARD_REV_ID0",
    "FM_BOARD_REV_ID1",
    "FM_BOARD_REV_ID2",
  };

  if ( get_gpio_shadow_array(shadows, 3, board_rev) ) {
    return -1;
  }
}

int
main(int argc, char **argv) {
  int ret = 0;
  int fd = 0;
  int i = 0;
  void *reg = NULL;
  void *base_addr = NULL;

  if ( (fd = open_mem()) < 0 ) {
    return -1;
  }
  
  printf("Set up GPIO pins.....\n");
  /* Disable PWM reset during WDT1 reset */
  base_addr = mapping_addr(fd, WDT_BASE);
  reg = base_addr + REG_WDT1_RESET;
  set_register(reg, get_register(reg) & ~(0x20000));
  munmap(base_addr, PAGE_SIZE);

  /* Setup debounce for GPIOQ6*/
  base_addr = mapping_addr(fd, GPIO_BASE);

  //Set debounce timer #1 value to 0x12E1FC ~= 100ms
  reg = base_addr + GPIO_DEBOUNCE_TIMER1;
  set_register(reg, 0x12E1FC);
  
  //Select debounce timer#1
  reg = base_addr + GPIO_QRST_DEBOUNCE_REG1;
  set_register(reg, get_register(reg) & ~(0x40));

  reg = base_addr + GPIO_QRST_DEBOUNCE_REG2;
  set_register(reg, get_register(reg) | (0x40));
  munmap(base_addr, PAGE_SIZE);

  /* Disable JTAG engine*/
  base_addr = mapping_addr(fd, JTAG_BASE);
  reg = base_addr + JTAG_ENG_CTRL;
  set_register(reg, get_register(reg) & ~(0xC0000000));
  munmap(base_addr, PAGE_SIZE);

  /* Setup SCU*/
  base_addr = mapping_addr(fd, SCU_BASE);

  //SCU10 - clear bits
  reg = base_addr + REG_SCU10;
  set_register(reg, get_register(reg) & ~(0x100));

  //SCU70 - clear bits(HWStrap 70h)
  reg = base_addr + REG_SCU70;
  set_register(reg, get_register(reg) & ~(0x600000));

  //SCU80 - clear bits
  reg = base_addr + REG_SCU80;
  set_register(reg, get_register(reg) & ~(0x40ce6001));

  //SCU84 - clear bits
  reg = base_addr + REG_SCU84;
  set_register(reg, get_register(reg) & ~(0x820000e));

  //SCU88 - clear bits
  reg = base_addr + REG_SCU88;
  set_register(reg, get_register(reg) & ~(0xfcff0008));

  //SCU8C - clear bits
  reg = base_addr + REG_SCU8C;
  set_register(reg, get_register(reg) & ~(0xa70a));

  //SCU90 - clear bits
  reg = base_addr + REG_SCU90;
  set_register(reg, get_register(reg) & ~(0x180000e2));

  //SCU94 - clear bits
  reg = base_addr + REG_SCU94;
  set_register(reg, get_register(reg) & ~(0xce0));

  //SCU9C - clear bits
  reg = base_addr + REG_SCU9C;
  set_register(reg, get_register(reg) & ~(0x20000));

  //SCUA0 - set bits
  reg = base_addr + REG_SCUA0;
  set_register(reg, get_register(reg) | (0x82000));

  //SCUA4 - clear bits and then set bits
  reg = base_addr + REG_SCUA4;
  set_register(reg, get_register(reg) & ~(0xa5000600));
  set_register(reg, get_register(reg) | (0xf8));

  //SCUA8 - clear bits
  reg = base_addr + REG_SCUA8;
  set_register(reg, get_register(reg) & ~(0x2));

  gpio_desc_t *gdesc = NULL;
  char *gname = NULL;
  char *gshadow = NULL;
  for ( i = 0; i < ngpios; i++ ) {
    gname = gpio_list[i].name;
    gshadow = gpio_list[i].shadow;
    gpio_export_by_name(GPIO_CHIP_ASPEED, gname, gshadow);    
    if ( gpio_list[i].dir == GPIO_DIRECTION_OUT ) {
      if ( (gdesc = gpio_open_by_shadow(gpio_list[i].shadow)) == NULL) {
        syslog(LOG_WARNING, "setup-gpio: Failed to gpio_open_by_shadow %s-%s", gname, gshadow);
      } else {
        gpio_set_init_value(gdesc, gpio_list[i].val);
        gpio_close(gdesc);
      }
    }
  }

  //check board rev id
  uint8_t board_rev = 0;
  if ( get_board_rev(&board_rev) < 0 ) {
    syslog(LOG_WARNING, "setup-gpio: Failed to get board rev");
  } else {
    if ( board_rev <= 2 ) {
      reg = base_addr + REG_SCUA8;
      set_register(reg, get_register(reg) & ~(0x4));
      gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOAB2", "PECI_MUX_SELECT");
    } else {
      reg = base_addr + REG_SCUA4;
      set_register(reg, get_register(reg) & ~(0x10000000));
      gpio_export_by_name(GPIO_CHIP_ASPEED, "GPIOAA4", "PECI_MUX_SELECT");
    } 
  }

  munmap(base_addr, PAGE_SIZE);
  close_mem(fd);
  return 0;
}
