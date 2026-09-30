/*
 * mdio_us_mac.c - Read upstream MAC address from BCM switch via MDIO
 *
 * Compile: gcc -O3 -o mdio_us_mac mdio_us_mac.c
 * Usage: ./mdio_us_mac
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

#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

/* Hardware Addresses */
#define MDIO0_BASE 0x1e650000
#define SCU_RESET_SET2 0x1e6e2050
#define SCU_RESET_CLEAR2 0x1e6e2054
#define SCU_PIN_CONTROL8 0x1e6e2430

#define SCU_RESET_MII (1 << 3)
#define SCU_MDCMDIO_EN ((1 << 16) | (1 << 17))

/* MDIO Control Bits */
#define FIRE_BUSY (1U << 31)
#define CLAUSE_22 (1U << 28)
#define READ_REQ (1U << 27)
#define WRITE_REQ (1U << 26)
#define CTRL_IDLE (1U << 16)

#define PSEUDO_PHY_ADDR 0x1e
#define MDIO_CMD(op, reg) \
  (FIRE_BUSY | CLAUSE_22 | (op) | (PSEUDO_PHY_ADDR << 21) | ((reg) << 16))

/* OOB Access */
#define OOB_PAGE 16
#define OOB_ADDR 17
#define OOB_DATA_BASE 24
#define OOB_OP_READ 0x02
#define OOB_OP_WRITE 0x01
#define OOB_ENABLE 0x01

/* Extract 16-bit chunk at given byte offset from value */
#define CHUNK16(val, byte_pos) (((val) >> ((byte_pos) * 8)) & 0xFFFF)

/* Control Page (0x00) */
#define CTRL_PAGE 0x00
#define RES_MUL_CTRL 0x2f
#define EN_RES_MUL_LEARN (1 << 7)

/* A directly-attached uServer can only ever source one MAC on its port. Seeing
   this many distinct MACs on USERVER_PORT means it is acting as a network
   uplink, i.e. the mgmt cable is swapped. The threshold (vs >1) absorbs
   transient/stale reads without masking a real swap, which floods the port with
   many MACs. */
#define MAX_DISTINCT 4

/* ARL/VTABLE Page (0x05) */
#define ARL_PAGE 0x05
#define ARLA_SRCH_CTL 0x50
#define ARLA_SRCH_ADR 0x51
#define ARLA_SRCH_RSLT_MACVID 0x60 /* +0x10 for result 1 */
#define ARLA_SRCH_RSLT 0x68 /* +0x10 for result 1 */
#define ARLA_SRCH_STDN 0x80
#define ARLA_SRCH_VLID 0x01

#define RSLT_VALID (1 << 16)
#define RSLT_PORT_MASK 0x1FF
#define USERVER_PORT 1

/* Memory Mapping */
#define PAGE_SIZE 4096
#define PAGE_MASK (~(PAGE_SIZE - 1))

typedef struct {
  int fd;
  volatile uint32_t* mdio_ctrl;
  volatile uint32_t* mdio_data;
  volatile uint32_t* scu_reset_set;
  volatile uint32_t* scu_reset_clear;
  volatile uint32_t* scu_pin_ctrl;
} MdioCtx;

/* Map a single hardware register into user space via /dev/mem */
static volatile uint32_t* map_reg(int fd, uint32_t addr) {
  uint32_t page_base = addr & PAGE_MASK;
  uint32_t offset = addr & ~PAGE_MASK;

  void* p =
      mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, page_base);
  if (p == MAP_FAILED) {
    return NULL;
  }
  return (volatile uint32_t*)((char*)p + offset);
}

/* Initialize MDIO controller: map registers and enable the interface */
static int mdio_init(MdioCtx* ctx) {
  ctx->fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (ctx->fd < 0) {
    return -1;
  }

  /* Map MDIO and SCU control registers */
  ctx->mdio_ctrl = map_reg(ctx->fd, MDIO0_BASE);
  ctx->mdio_data = map_reg(ctx->fd, MDIO0_BASE + 4);
  ctx->scu_reset_set = map_reg(ctx->fd, SCU_RESET_SET2);
  ctx->scu_reset_clear = map_reg(ctx->fd, SCU_RESET_CLEAR2);
  ctx->scu_pin_ctrl = map_reg(ctx->fd, SCU_PIN_CONTROL8);

  if (!ctx->mdio_ctrl || !ctx->mdio_data || !ctx->scu_reset_set ||
      !ctx->scu_reset_clear || !ctx->scu_pin_ctrl) {
    return -1;
  }

  /* Deassert MII reset and enable MDC/MDIO pins */
  *ctx->scu_reset_clear = SCU_RESET_MII;
  *ctx->scu_pin_ctrl |= SCU_MDCMDIO_EN;
  return 0;
}

/* Cleanup: put MII back into reset, disable MDC/MDIO pins, close /dev/mem */
static void mdio_cleanup(MdioCtx* ctx) {
  *ctx->scu_reset_set = SCU_RESET_MII;
  *ctx->scu_pin_ctrl &= ~SCU_MDCMDIO_EN;
  close(ctx->fd);
}

/* Issue MDIO clause 22 read: fire command, wait for idle, return data */
static uint16_t mdio_read(MdioCtx* ctx, uint8_t reg) {
  *ctx->mdio_ctrl = MDIO_CMD(READ_REQ, reg);
  while (!(*ctx->mdio_data & CTRL_IDLE)) {
    usleep(100);
  }
  return *ctx->mdio_data & 0xFFFF;
}

/* Issue MDIO clause 22 write: fire command with data, wait for completion */
static void mdio_write(MdioCtx* ctx, uint8_t reg, uint16_t val) {
  *ctx->mdio_ctrl = MDIO_CMD(WRITE_REQ, reg) | val;
  while (*ctx->mdio_ctrl & FIRE_BUSY) {
    usleep(100);
  }
}

static void reg_wait_ack(MdioCtx* ctx) {
  while (mdio_read(ctx, OOB_ADDR) & 0x3) {
    usleep(1000);
  }
}

static void oob_select_page(MdioCtx* ctx, uint8_t page) {
  mdio_write(ctx, OOB_PAGE, (page << 8) | OOB_ENABLE);
}

static void oob_deselect_page(MdioCtx* ctx) {
  mdio_write(ctx, OOB_PAGE, 0);
}

static uint64_t reg_read(MdioCtx* ctx, uint8_t page, uint8_t addr, int bytes) {
  oob_select_page(ctx, page);
  mdio_write(ctx, OOB_ADDR, (addr << 8) | OOB_OP_READ);
  reg_wait_ack(ctx);

  uint64_t val = 0;
  for (int b = 0; b < bytes; b += 2) {
    val |= (uint64_t)mdio_read(ctx, OOB_DATA_BASE + b / 2) << (b * 8);
  }

  oob_deselect_page(ctx);
  return val;
}

static void
reg_write(MdioCtx* ctx, uint8_t page, uint8_t addr, uint64_t val, int bytes) {
  oob_select_page(ctx, page);
  for (int b = 0; b < bytes; b += 2) {
    mdio_write(ctx, OOB_DATA_BASE + b / 2, CHUNK16(val, b));
  }
  mdio_write(ctx, OOB_ADDR, (addr << 8) | OOB_OP_WRITE);
  reg_wait_ack(ctx);
  oob_deselect_page(ctx);
}

static int find_userver_mac(MdioCtx* ctx, int idx, uint64_t* mac) {
  // Result registers 0 and 1 are 0x10 apart
  uint8_t base = ARLA_SRCH_RSLT_MACVID + idx * 0x10;

  /* Read MACVID (0x60/0x70) BEFORE the result register (0x68/0x78) */
  uint64_t macvid = reg_read(ctx, ARL_PAGE, base, 8);
  uint32_t rslt = reg_read(ctx, ARL_PAGE, base + 8, 4);

  /* Check if the port we are after was found in the table */
  if ((rslt & RSLT_VALID) && (rslt & RSLT_PORT_MASK) == USERVER_PORT) {
    *mac = macvid;
    return 1;
  }
  return 0;
}

static void print_mac(uint64_t mac) {
  printf(
      "%02x:%02x:%02x:%02x:%02x:%02x\n",
      (unsigned)(mac >> 40) & 0xFF,
      (unsigned)(mac >> 32) & 0xFF,
      (unsigned)(mac >> 24) & 0xFF,
      (unsigned)(mac >> 16) & 0xFF,
      (unsigned)(mac >> 8) & 0xFF,
      (unsigned)mac & 0xFF);
}

int main(void) {
  MdioCtx ctx;
  uint64_t mac;
  uint64_t seen[MAX_DISTINCT];
  int nseen = 0;

  /* Failing to init MDIO => exit immediately */
  if (mdio_init(&ctx) < 0) {
    fprintf(stderr, "Failed to initialize MDIO\n");
    return 1;
  }

  /* Rewind the search pointer to the top of the table. The STDN start bit
     does not reset it, so without this a search left mid-table (or at the
     end) by a previous run completes immediately and finds nothing. */
  reg_write(&ctx, ARL_PAGE, ARLA_SRCH_ADR, 0, 2);

  /* Initiate a sequential search of the internal MAC table */
  reg_write(&ctx, ARL_PAGE, ARLA_SRCH_CTL, ARLA_SRCH_STDN | ARLA_SRCH_VLID, 2);

  for (;;) {
    uint8_t ctl = reg_read(&ctx, ARL_PAGE, ARLA_SRCH_CTL, 1);

    /* Reading is DONE when bit 7 goes low */
    if (!(ctl & ARLA_SRCH_STDN)) {
      break;
    }

    /* Result is VALID if bit 0 is 1 */
    if (!(ctl & ARLA_SRCH_VLID)) {
      continue;
    }

    /* Check both result registers. The search can report the same entry in
       both result bins, so collect only genuinely distinct MACs. Stop once we
       have enough to declare a miscable, no need to scan further. */
    for (int i = 0; i < 2 && nseen < MAX_DISTINCT; i++) {
      if (find_userver_mac(&ctx, i, &mac)) {
        bool dup = false;

        for (int k = 0; k < nseen; k++) {
          if (seen[k] == mac) {
            dup = true;
            break;
          }
        }

        if (!dup) {
          seen[nseen++] = mac;
        }
      }
    }
  }

  /* Many distinct MACs on the uServer port means it is acting as an uplink:
     the mgmt cable is swapped. */
  if (nseen >= MAX_DISTINCT) {
    fprintf(stderr, "Multiple MACs found, likely bmc mgmt cable swapped!\n");
    mdio_cleanup(&ctx);
    return 1;
  }

  /* First valid MAC we encounter should be the microserver MAC */
  if (nseen > 0) {
    print_mac(seen[0]);
    mdio_cleanup(&ctx);
    return 0;
  }

  /* Nothing was found. This can happen if multicast learning and forwarding not
     set up properly. We'll enable it and exit with an error, and by the next
     LLDP packet the MAC will be learned correctly. */

  if (reg_read(&ctx, CTRL_PAGE, RES_MUL_CTRL, 1) != EN_RES_MUL_LEARN) {
    reg_write(&ctx, CTRL_PAGE, RES_MUL_CTRL, EN_RES_MUL_LEARN, 2);
  }

  print_mac(0);
  mdio_cleanup(&ctx);
  return 1;
}
