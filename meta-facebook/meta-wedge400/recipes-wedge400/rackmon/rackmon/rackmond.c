/*
 * Copyright 2019-present Facebook. All Rights Reserved.
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

#include "modbus.h"
#include "rackmond.h"
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>
#include <stdarg.h>
#include <signal.h>
#include <linux/serial.h>
#include <openbmc/log.h>

#define RACKMOND_NAME "rackmond"
#define RACKMOND_PID_FILE "/var/run/rackmond.pid"
#define MAX_ACTIVE_ADDRS 24
#define REGISTER_PSU_STATUS 0x68

#define READ_ERROR_RESPONSE -2

struct _lock_holder
{
  pthread_mutex_t *lock;
  int held;
};

#define lock_holder(holder_name, lock_expr) \
  struct _lock_holder holder_name;          \
  holder_name.lock = lock_expr;             \
  holder_name.held = 0;

#define lock_take(holder_name)            \
  {                                       \
    pthread_mutex_lock(holder_name.lock); \
    holder_name.held = 1;                 \
  }

#define lock_release(holder_name)             \
  {                                           \
    if (holder_name.held)                     \
    {                                         \
      pthread_mutex_unlock(holder_name.lock); \
      holder_name.held = 0;                   \
    }                                         \
  }

int scanning = 0;

typedef struct _rs485_dev
{
  // hold this for the duration of a command
  pthread_mutex_t lock;
  int tty_fd;
} rs485_dev;

typedef struct _register_req
{
  uint16_t begin;
  int num;
} register_req;

typedef struct register_range_data
{
  monitor_interval *i;
  void *mem_begin;
  size_t mem_pos;
} register_range_data;

typedef struct monitoring_data
{
  uint8_t addr;
  uint32_t crc_errors;
  uint32_t timeout_errors;
  register_range_data range_data[1];
} monitoring_data;

typedef struct _rackmond_data
{
  // global rackmond lock
  pthread_mutex_t lock;
  // number of register read commands to send to each PSU
  int num_reqs;
  // register read commands (begin+length)
  register_req *reqs;
  monitoring_config *config;

  uint8_t num_active_addrs;
  uint8_t active_addrs[MAX_ACTIVE_ADDRS];
  monitoring_data *stored_data[MAX_ACTIVE_ADDRS];
  FILE *status_log;

  // timeout in nanosecs
  int modbus_timeout;

  // inter-command delay
  int min_delay;

  int paused;

  rs485_dev rs485;
} rackmond_data;

typedef struct _write_buffer
{
  char *buffer;
  size_t len;
  size_t pos;
  int fd;
} write_buffer;

int buf_open(write_buffer *buf, int fd, size_t len)
{
  int error = 0;
  char *bufmem = malloc(len);
  if (!bufmem)
  {
    BAIL("Couldn't allocate write buffer of len %zd for fd %d\n", len, fd);
  }
  buf->buffer = bufmem;
  buf->pos = 0;
  buf->len = len;
  buf->fd = fd;
cleanup:
  return error;
}

ssize_t buf_flush(write_buffer *buf)
{
  int ret;
  ret = write(buf->fd, buf->buffer, buf->pos);
  if (ret > 0)
  {
    memmove(buf->buffer, buf->buffer + ret, buf->pos - ret);
    buf->pos -= ret;
  }
  return ret;
}

ssize_t buf_write(write_buffer *buf, void *from, size_t len)
{
  int ret;
  // write will not fill buffer, only memcpy
  if ((buf->pos + len) < buf->len)
  {
    memcpy(buf->buffer + buf->pos, from, len);
    buf->pos += len;
    return len;
  }

  // write would exceed buffer, flush first
  ret = buf_flush(buf);
  if (buf->pos != 0)
  {
    if (ret < 0)
    {
      return ret;
    }
    // write() was interrupted but partially succeeded -- the buffer partially
    // flushed but no bytes of the requested buf_write went through
    return 0;
  }

  if (len > buf->len)
  {
    // write is larger than buffer, skip buffer
    return write(buf->fd, from, len);
  }
  else
  {
    return buf_write(buf, from, len);
  }
}

int bprintf(write_buffer *buf, const char *format, ...)
{
  // eh.
  char tmpbuf[512];
  int error = 0;
  int ret;
  va_list args;
  va_start(args, format);
  ret = vsnprintf(tmpbuf, sizeof(tmpbuf), format, args);
  CHECK(ret);
  if (ret > sizeof(tmpbuf))
  {
    BAIL("truncated bprintf (%d bytes truncated to %d)", ret, sizeof(tmpbuf));
  }
  CHECK(buf_write(buf, tmpbuf, ret));
cleanup:
  va_end(args);
  return error;
}

int buf_close(write_buffer *buf)
{
  int error = 0;
  int fret = buf_flush(buf);
  int cret = close(buf->fd);
  free(buf->buffer);
  buf->buffer = NULL;
  CHECK(fret);
  CHECKP(close, cret);
cleanup:
  return error;
}

rackmond_data world;

char psu_address(int rack, int shelf, int psu)
{
  int rack_a = ((rack & 3) << 3);
  int shelf_a = ((shelf & 1) << 2);
  int psu_a = (psu & 3);
  return 0xA0 | rack_a | shelf_a | psu_a;
}

int modbus_command(rs485_dev *dev, int timeout, char *command, size_t len, char *destbuf, size_t dest_limit, size_t expect)
{
  int error = 0;
  useconds_t delay = world.min_delay;
  lock_holder(devlock, &dev->lock);
  modbus_req req;
  req.tty_fd = dev->tty_fd;
  req.modbus_cmd = command;
  req.cmd_len = len;
  req.dest_buf = destbuf;
  req.dest_limit = dest_limit;
  req.timeout = timeout;
  req.expected_len = expect != 0 ? expect : dest_limit;
  req.scan = scanning;
  lock_take(devlock);
  int cmd_error = modbuscmd(&req);
  if (delay != 0)
  {
    usleep(delay);
  }
  CHECK(cmd_error);
cleanup:
  lock_release(devlock);
  if (error >= 0)
  {
    return req.dest_len;
  }

  return error;
}

int read_registers(rs485_dev *dev, int timeout, uint8_t addr, uint16_t begin, uint16_t num, uint16_t *out)
{
  int error = 0;
  // address, function, begin, length in # of regs
  char command[sizeof(addr) + 1 + sizeof(begin) + sizeof(num)];
  // address, function, length (1 byte), data (2 bytes per register), crc
  // (VLA)
  char response[sizeof(addr) + 1 + 1 + (2 * num) + 2];
  command[0] = addr;
  command[1] = MODBUS_READ_HOLDING_REGISTERS;
  command[2] = begin >> 8;
  command[3] = begin & 0xFF;
  command[4] = num >> 8;
  command[5] = num & 0xFF;

  int dest_len =
      modbus_command(
          dev, timeout,
          command, sizeof(addr) + 1 + sizeof(begin) + sizeof(num),
          response, sizeof(addr) + 1 + 1 + (2 * num) + 2, 0);
  CHECK(dest_len);

  if (dest_len >= 5)
  {
    memcpy(out, response + 3, num * 2);
  }
  else
  {
    log("Unexpected short but CRC correct response!\n");
    error = -1;
    goto cleanup;
  }
  if (response[0] != addr)
  {
    log("Got response for addr %02x when expected %02x\n", response[0], addr);
    error = -1;
    goto cleanup;
  }
  if (response[1] != MODBUS_READ_HOLDING_REGISTERS)
  {
    // got an error response instead of a read regsiters response
    // likely because the requested registers aren't available on this model (e.g. stingray)
    error = READ_ERROR_RESPONSE;
    goto cleanup;
  }
  if (response[2] != (num * 2))
  {
    log("Got %d register data bytes when expecting %d\n", response[2], (num * 2));
    error = -1;
    goto cleanup;
  }
cleanup:
  return error;
}

int sub_uint8s(const void *a, const void *b)
{
  return (*(uint8_t *)a) - (*(uint8_t *)b);
}

int check_active_psus()
{
  int error = 0;
  lock_holder(worldlock, &world.lock);
  lock_take(worldlock);
  if (world.paused == 1)
  {
    lock_release(worldlock);
    usleep(1000);
    goto cleanup;
  }
  if (world.config == NULL)
  {
    lock_release(worldlock);
    usleep(5000);
    goto cleanup;
  }
  world.num_active_addrs = 0;

  scanning = 1;
  for (int rack = 0; rack < 3; rack++)
  {
    for (int shelf = 0; shelf < 2; shelf++)
    {
      for (int psu = 0; psu < 3; psu++)
      {
        char addr = psu_address(rack, shelf, psu);
        uint16_t status = 0;
        for (int retry = 0; retry < 5; retry++) {
          int err = read_registers(&world.rs485, world.modbus_timeout, addr, REGISTER_PSU_STATUS, 1, &status);
          if (err == 0)
          {
            world.active_addrs[world.num_active_addrs] = addr;
            world.num_active_addrs++;
          }
          else
          {
            OBMC_INFO("check_active_psus error %02X - %d", addr, err);
            dbg("%02x - %d; ", addr, err);
          }
        }
      }
    }
  }
  //its the only stdlib sort
  qsort(world.active_addrs, world.num_active_addrs,
        sizeof(uint8_t), sub_uint8s);
cleanup:
  scanning = 0;
  lock_release(worldlock);
  return error;
}

monitoring_data *alloc_monitoring_data(uint8_t addr)
{
  size_t size = sizeof(monitoring_data) +
                sizeof(register_range_data) * world.config->num_intervals;
  for (int i = 0; i < world.config->num_intervals; i++)
  {
    monitor_interval *iv = &world.config->intervals[i];
    int pitch = sizeof(uint32_t) + (sizeof(uint16_t) * iv->len);
    int data_size = pitch * iv->keep;
    size += data_size;
  }
  monitoring_data *d = calloc(1, size);
  if (d == NULL)
  {
    log("Failed to allocate memory for sensor data.\n");
    return NULL;
  }
  d->addr = addr;
  d->crc_errors = 0;
  d->timeout_errors = 0;
  void *mem = d;
  mem = mem + (sizeof(monitoring_data) +
               sizeof(register_range_data) * world.config->num_intervals);
  for (int i = 0; i < world.config->num_intervals; i++)
  {
    monitor_interval *iv = &world.config->intervals[i];
    int pitch = sizeof(uint32_t) + (sizeof(uint16_t) * iv->len);
    int data_size = pitch * iv->keep;
    d->range_data[i].i = iv;
    d->range_data[i].mem_begin = mem;
    d->range_data[i].mem_pos = 0;
    mem = mem + data_size;
  }
  return d;
}

int sub_storeptrs(const void *va, const void *vb)
{
  //more *s than i like :/
  monitoring_data *a = *(monitoring_data **)va;
  monitoring_data *b = *(monitoring_data **)vb;
  //nulls to the end
  if (b == NULL && a == NULL)
  {
    return 0;
  }
  if (b == NULL)
  {
    return -1;
  }
  if (a == NULL)
  {
    return 1;
  }
  return a->addr - b->addr;
}

int alloc_monitoring_datas()
{
  int error = 0;
  lock_holder(worldlock, &world.lock);
  lock_take(worldlock);
  if (world.config == NULL)
  {
    goto cleanup;
  }
  qsort(world.stored_data, MAX_ACTIVE_ADDRS,
        sizeof(monitoring_data *), sub_storeptrs);
  int data_pos = 0;
  OBMC_INFO("alloc_monitoring_datas num_active : %d", world.num_active_addrs);
  for (int i = 0; i < world.num_active_addrs; i++)
  {
    uint8_t addr = world.active_addrs[i];
    while (world.stored_data[data_pos] != NULL &&
           world.stored_data[data_pos]->addr != addr)
    {
      data_pos++;
    }
    if (world.stored_data[data_pos] == NULL)
    {
      log("Detected PSU at address 0x%02x\n", addr);
      // this will only be logged once per address
      OBMC_INFO("Detected PSU at address 0x%02x", addr);
      world.stored_data[data_pos] = alloc_monitoring_data(addr);
      if (world.stored_data[data_pos] == NULL)
      {
        BAIL("allocation failed\n");
      }
      //reset search pos after alloc (post-sorted addrs may already be alloc'd, need to check again)
      data_pos = 0;
      continue;
    }
    if (world.stored_data[data_pos]->addr == addr)
    {
      continue;
    }
    BAIL("shouldn't get here!\n");
  }
cleanup:
  lock_release(worldlock);
  return error;
}

void record_data(register_range_data *rd, uint32_t time, uint16_t *regs)
{
  int n_regs = (rd->i->len);
  int pitch = sizeof(time) + (sizeof(uint16_t) * n_regs);
  int mem_size = pitch * rd->i->keep;

  memcpy(rd->mem_begin + rd->mem_pos, &time, sizeof(time));
  rd->mem_pos += sizeof(time);
  memcpy(rd->mem_begin + rd->mem_pos, regs, n_regs * sizeof(uint16_t));
  rd->mem_pos += n_regs * sizeof(uint16_t);
  rd->mem_pos = rd->mem_pos % mem_size;
}

int fetch_monitored_data()
{
  int error = 0;
  int data_pos = 0;
  lock_holder(worldlock, &world.lock);
  lock_take(worldlock);
  if (world.paused == 1)
  {
    lock_release(worldlock);
    usleep(1000);
    goto cleanup;
  }
  if (world.config == NULL)
  {
    goto cleanup;
  }
  lock_release(worldlock);

  usleep(1000); // wait a sec btween PSUs to not overload RT scheduling
                // threshold
  while (world.stored_data[data_pos] != NULL && data_pos < MAX_ACTIVE_ADDRS)
  {
    uint8_t addr = world.stored_data[data_pos]->addr;
    for (int r = 0; r < world.config->num_intervals; r++)
    {
      register_range_data *rd = &world.stored_data[data_pos]->range_data[r];
      monitor_interval *i = rd->i;
      uint16_t regs[i->len];
      int err = read_registers(&world.rs485,
                               world.modbus_timeout, addr, i->begin, i->len, regs);
      if (err)
      {
        if (err != READ_ERROR_RESPONSE)
        {
          log("Error %d reading %02x registers at %02x from %02x\n",
              err, i->len, i->begin, addr);
          if (err == MODBUS_BAD_CRC)
          {
            world.stored_data[data_pos]->crc_errors++;
          }
          if (err == MODBUS_RESPONSE_TIMEOUT)
          {
            world.stored_data[data_pos]->timeout_errors++;
          }
        }
        continue;
      }
      struct timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      uint32_t timestamp = ts.tv_sec;
      if (rd->i->flags & MONITOR_FLAG_ONLY_CHANGES)
      {
        int pitch = sizeof(timestamp) + (sizeof(uint16_t) * i->len);
        int lastpos = rd->mem_pos - pitch;
        if (lastpos < 0)
        {
          lastpos = (pitch * rd->i->keep) - pitch;
        }
        if (!memcmp(rd->mem_begin + lastpos + sizeof(timestamp),
                    regs, sizeof(uint16_t) * i->len) &&
            memcmp(rd->mem_begin, "\x00\x00\x00\x00", 4))
        {
          continue;
        }

        if (world.status_log)
        {
          time_t rawt;
          struct tm *ti;
          time(&rawt);
          ti = localtime(&rawt);
          char timestr[80];
          strftime(timestr, sizeof(timestr), "%b %e %T", ti);
          fprintf(world.status_log,
                  "%s: Change to status register %02x on address %02x. New value: %02x\n",
                  timestr, i->begin, addr, regs[0]);
          fflush(world.status_log);
        }
      }
      lock_take(worldlock);
      record_data(rd, timestamp, regs);
      lock_release(worldlock);
    }
    data_pos++;
  }
cleanup:
  lock_release(worldlock);
  return error;
}

int rackmon_print_info(write_buffer *wb, monitoring_data *data)
{
  bprintf(wb, "PSU Addr %02x\n", data->addr);
  for (int i = 0; i < world.config->num_intervals; i++)
  {
    uint32_t time;
    int value = 0, N = 0;
    float fvalue = 0.0;
    register_range_data *rd = &data->range_data[i];
    char *mem_pos = rd->mem_begin;

#define PRINTDESC(reg, desc)                       \
  case reg:                                        \
    bprintf(wb, "  <0x%04X> %-32s : ", reg, desc); \
    break
    switch (rd->i->begin)
    {
      PRINTDESC(0x00, "MFG_MODEL");
      PRINTDESC(0x10, "MFG_DATE");
      PRINTDESC(0x20, "FB part#");
      PRINTDESC(0x30, "HW Revision");
      PRINTDESC(0x38, "FW Revision");
      PRINTDESC(0x40, "MFR_SERIAL");
      PRINTDESC(0x60, "Workorder #");
      PRINTDESC(0x68, "PSU Status register");
      PRINTDESC(0x69, "Battery Status register");
      PRINTDESC(0x6A, "Reserved");
      PRINTDESC(0x80, "Input Voltage AC");
      PRINTDESC(0x81, "Reserved");
      PRINTDESC(0x82, "Input Current AC");
      PRINTDESC(0x83, "Reserved");
      PRINTDESC(0x84, "Battery Voltage");
      PRINTDESC(0x85, "Reserved");
      PRINTDESC(0x86, "Battery Current output");
      PRINTDESC(0x87, "Reserved");
      PRINTDESC(0x88, "Battery Current Input");
      PRINTDESC(0x89, "Reserved");
      PRINTDESC(0x8A, "Output Voltage (main converter)");
      PRINTDESC(0x8B, "Reserved");
      PRINTDESC(0x8C, "Output Current (main converter)");
      PRINTDESC(0x8D, "Reserved");
      PRINTDESC(0x8E, "IT load Voltage Output");
      PRINTDESC(0x8F, "Reserved");
      PRINTDESC(0x90, "IT load Current Output");
      PRINTDESC(0x91, "Reserved");
      PRINTDESC(0x92, "Bulk Cap Voltage");
      PRINTDESC(0x93, "Reserved");
      PRINTDESC(0x94, "Input Power");
      PRINTDESC(0x95, "Reserved");
      PRINTDESC(0x96, "Output Power");
      PRINTDESC(0x97, "Reserved");
      PRINTDESC(0x98, "RPM fan0");
      PRINTDESC(0x99, "Reserved");
      PRINTDESC(0x9A, "RPM fan1");
      PRINTDESC(0x9B, "Reserved");
      PRINTDESC(0x9C, "Set fan speed");
      PRINTDESC(0x9D, "Reserved");
      PRINTDESC(0x9E, "Temp0");
      PRINTDESC(0x9F, "Reserved");
      PRINTDESC(0xA0, "Temp1");
      PRINTDESC(0xD0, "General Alarm");
      PRINTDESC(0xD1, "PFC Alarm");
      PRINTDESC(0xD2, "LLC Alarm");
      PRINTDESC(0xD3, "Current Feed Alarm");
      PRINTDESC(0xD4, "Auxiliary Alarm");
      PRINTDESC(0xD5, "Battery Charger Alarm");
      PRINTDESC(0xD6, "Reserved");
      PRINTDESC(0xD7, "Temperature Alarm");
      PRINTDESC(0xD8, "Fan Alarm");
    default:
      bprintf(wb, "  <0x%04X>                                  : ", rd->i->begin);
      break;
    }
    memcpy(&time, mem_pos, sizeof(time));
    for (int j = 0; j < rd->i->keep && time != 0; j++)
    {
      mem_pos += sizeof(time);
      switch (rd->i->flags & 0xF000)
      {
      case 0x8000: //Ascii
        for (int c = 0; c < rd->i->len * 2; c++)
        {
          bprintf(wb, "%c", *mem_pos);
          mem_pos++;
        }
        bprintf(wb, "  "); // End data
        break;
      case 0x4000: //2's complement
        N = (rd->i->flags >> 8) & 0x0F;
        fvalue = 0;
        for (int c = 0; c < rd->i->len * 2; c++)
        {
          fvalue *= 256.0;
          fvalue += *mem_pos;
          mem_pos++;
        }
        for (int c = 0; c < N; c++)
        {
          fvalue /= 2.0;
        }
        bprintf(wb, "%.2f  ", fvalue);
        break;
      case 0x2000: //decimal
        value = 0;
        for (int c = 0; c < rd->i->len * 2; c++)
        {
          value <<= 8;
          value += *mem_pos;
          mem_pos++;
        }
        bprintf(wb, "%d  ", value);
        break;
      case 0x1000: // table
        for (int c = 0; c < rd->i->len * 2; c++)
        {
          // bprintf(wb, "%02x", *mem_pos);
          value <<= 8;
          value += *mem_pos;
          mem_pos++;
        }
        bprintf(wb, "%04x\n", value);
#define GETBIT(reg, bit) ((reg >> bit) & 0x1)
#define PRINTSTAT(reg, bit, des)                                                             \
  do                                                                                         \
  {                                                                                          \
    bprintf(wb, "       %c [%d] " des "\n", GETBIT(reg, bit) ? '+' : ' ', GETBIT(reg, bit)); \
  } while (0)
        if (rd->i->begin == 0x68)
        {
          PRINTSTAT(value, 8, "SoH Dicharge");
          PRINTSTAT(value, 7, "SoH Requested");
          PRINTSTAT(value, 6, "Battery Alarm Set for BBU Fail or BBU voltage =< 26 VDC");
          PRINTSTAT(value, 5, "Fan Alarm");
          PRINTSTAT(value, 4, "Temp Alarm   Alarm set at shutdown temp");
          PRINTSTAT(value, 3, "Current Feed (Boost Converter) Fail");
          PRINTSTAT(value, 2, "Battery Charger Fail");
          PRINTSTAT(value, 1, "Aux 54V Converter Fail");
          PRINTSTAT(value, 0, "Main Converter Fail");
        }
        else if (rd->i->begin == 0x69)
        {
          PRINTSTAT(value, 2, "End of Life");
          PRINTSTAT(value, 1, "Low Voltage  BBU Voltage =< 33.8");
          PRINTSTAT(value, 0, "BBU Fail");
        }
        else if (rd->i->begin == 0xD0)
        {
          PRINTSTAT(value, 7, "Com");
          PRINTSTAT(value, 6, "Fan");
          PRINTSTAT(value, 5, "Temp");
          PRINTSTAT(value, 4, "BC");
          PRINTSTAT(value, 3, "Auxiliary");
          PRINTSTAT(value, 2, "CF");
          PRINTSTAT(value, 1, "LLC");
          PRINTSTAT(value, 0, "PFC");
        }
        else if (rd->i->begin == 0xD1)
        {
          PRINTSTAT(value, 11, "LLC Enabled");
          PRINTSTAT(value, 10, "Input Relay on");
          PRINTSTAT(value, 9, "!(Bulk_OK)");
          PRINTSTAT(value, 8, "AC_OK");
          PRINTSTAT(value, 3, "LLC Enabled");
          PRINTSTAT(value, 1, "OVP (AC input over voltage protection asserted)");
          PRINTSTAT(value, 0, "UVP (AC input under voltage protection asserted)");
        }
        else if (rd->i->begin == 0xD2)
        {
          PRINTSTAT(value, 10, "Oring Fail");
          PRINTSTAT(value, 9, "2ndary DSP Failure");
          PRINTSTAT(value, 8, "DC/DC failure");
          PRINTSTAT(value, 2, "OCP (Output Over Current protection asserted)");
          PRINTSTAT(value, 1, "OVP (Output Over Voltage protection asserted)");
          PRINTSTAT(value, 0, "UVP (Output Under Voltage protection asserted)");
        }
        else if (rd->i->begin == 0xD3)
        {
          PRINTSTAT(value, 8, "CF (Failure Failure with CF)");
          PRINTSTAT(value, 4, "OPP (Over Power Protection [more than 4200W input to CF])");
          PRINTSTAT(value, 3, "Battery_UVP");
          PRINTSTAT(value, 2, "Battery_OVP");
          PRINTSTAT(value, 1, "Bulk_UVP");
          PRINTSTAT(value, 0, "Bulk_OVP");
        }
        else if (rd->i->begin == 0xD4)
        {
          PRINTSTAT(value, 8, "Aux alarm (Failure with auxiliary converter)");
          PRINTSTAT(value, 2, "OCP (Over Current Protection asserted)");
          PRINTSTAT(value, 1, "OVP (Output Over Voltage protection asserted)");
          PRINTSTAT(value, 0, "UVP (Output Under Voltage Protection asserted)");
        }
        else if (rd->i->begin == 0xD5)
        {
          PRINTSTAT(value, 8, "Charger alarm (Charger failure)");
          PRINTSTAT(value, 2, "Timeout alarm (Charger ON for more than 5hrs)");
          PRINTSTAT(value, 1, "UVP (Output Under Voltage Protection asserted)");
          PRINTSTAT(value, 0, "OVP (Output Overvoltage Protection asserted)");
        }
        else if (rd->i->begin == 0xD7)
        {
          PRINTSTAT(value, 7, "PFC temp alarm");
          PRINTSTAT(value, 6, "LLC temp alarm");
          PRINTSTAT(value, 5, "CF temp alarm");
          PRINTSTAT(value, 4, "Aux temp alarm");
          PRINTSTAT(value, 3, "Sync Rectifier temp alarm");
          PRINTSTAT(value, 2, "Oring temp alarm");
          PRINTSTAT(value, 1, "Inlet temp alarm");
          PRINTSTAT(value, 0, "Outlet temp alarm");
        }
        else if (rd->i->begin == 0xD8)
        {
          PRINTSTAT(value, 0, "Fan alarm (Fan failure)");
        }
        else if (rd->i->begin == 0xD9)
        {
          PRINTSTAT(value, 0, "Internal Communication alarm (Internal Communication fail)");
        }
        else
        {
          bprintf(wb, "Didn't implement parser yet.\n");
        }
        break;
      default:
        value = 0;
        for (int c = 0; c < rd->i->len * 2; c++)
        {
          bprintf(wb, "%02x", *mem_pos);
          mem_pos++;
        }
        bprintf(wb, "  "); // End data
        break;
      }
      memcpy(&time, mem_pos, sizeof(time));
      if (time == 0)
      {
        break;
      }
    }
    if ((rd->i->flags & 0xF000) != 0x1000)
    {
      bprintf(wb, "\n"); // End Register
    }
  }
  bprintf(wb, "\n"); // End PSU
  return 0;
}

// check for new psus every N seconds
static int search_at = 0;
#define SEARCH_PSUS_EVERY 120
#define MONITOR_INTERVAL 1
void *monitoring_loop(void *arg)
{
  (void)arg;
  world.status_log = fopen("/var/log/psu-status.log", "a+");
  while (1)
  {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    if (search_at < ts.tv_sec)
    {
      OBMC_INFO("rackmon scan");
      check_active_psus();
      alloc_monitoring_datas();
      fetch_monitored_data();
      clock_gettime(CLOCK_REALTIME, &ts);
      search_at = ts.tv_sec + SEARCH_PSUS_EVERY;
    }
    sleep(MONITOR_INTERVAL);
  }
  return NULL;
}

int open_rs485_dev(const char *tty_filename, rs485_dev *dev)
{
  OBMC_INFO("open_rs485_dev %s", tty_filename);
  int error = 0;
  int tty_fd;
  dbg("[*] Opening TTY\n");
  tty_fd = open(tty_filename, O_RDWR | O_NOCTTY);
  CHECK(tty_fd);

  struct serial_rs485 rs485conf = {};
  rs485conf.flags |= SER_RS485_ENABLED;
  dbg("[*] Putting TTY in RS485 mode\n");

  dev->tty_fd = tty_fd;
  pthread_mutex_init(&dev->lock, NULL);
cleanup:
  return error;
}

int do_command(int sock, rackmond_command *cmd)
{
  int error = 0;
  write_buffer wb;
  //128k write buffer
  buf_open(&wb, sock, 128 * 1000);
  lock_holder(worldlock, &world.lock);
  switch (cmd->type)
  {
  case COMMAND_TYPE_RAW_MODBUS:
  {
    uint16_t expected = cmd->raw_modbus.expected_response_length;
    int timeout = world.modbus_timeout;
    if (cmd->raw_modbus.custom_timeout)
    {
      //ms to us
      timeout = cmd->raw_modbus.custom_timeout * 1000;
    }
    if (expected == 0)
    {
      expected = 1024;
    }
    char response[expected];
    int response_len = modbus_command(
        &world.rs485, timeout,
        cmd->raw_modbus.data, cmd->raw_modbus.length,
        response, expected, expected);
    uint16_t response_len_wire = response_len;
    if (response_len < 0)
    {
      uint16_t error = -response_len;
      response_len_wire = 0;
      buf_write(&wb, &response_len_wire, sizeof(uint16_t));
      buf_write(&wb, &error, sizeof(uint16_t));
      break;
    }
    buf_write(&wb, &response_len_wire, sizeof(uint16_t));
    buf_write(&wb, response, response_len);
    break;
  }
  case COMMAND_TYPE_SET_CONFIG:
  {
    lock_take(worldlock);
    if (world.config != NULL)
    {
      OBMC_INFO("rackmond already configured");
      BAIL("rackmond already configured\n");
    }
    size_t config_size = sizeof(monitoring_config) +
                         (sizeof(monitor_interval) * cmd->set_config.config.num_intervals);
    world.config = calloc(1, config_size);
    memcpy(world.config, &cmd->set_config.config, config_size);
    OBMC_INFO("got configuration");
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    uint32_t now = ts.tv_sec;
    search_at = now;
    lock_release(worldlock);
    break;
  }
  case COMMAND_TYPE_DUMP_STATUS:
  {
    lock_take(worldlock);
    if (world.config == NULL)
    {
      bprintf(&wb, "Unconfigured\n");
    }
    else
    {
      struct timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      uint32_t now = ts.tv_sec;
      int data_pos = 0;
      bprintf(&wb, "Monitored PSUs:\n");
      while (world.stored_data[data_pos] != NULL && data_pos < MAX_ACTIVE_ADDRS)
      {
        bprintf(&wb, "PSU addr %02x - crc errors: %d, timeouts: %d\n",
                world.stored_data[data_pos]->addr,
                world.stored_data[data_pos]->crc_errors,
                world.stored_data[data_pos]->timeout_errors);
        data_pos++;
      }
      bprintf(&wb, "Active on last scan: ");
      for (int i = 0; i < world.num_active_addrs; i++)
      {
        bprintf(&wb, "%02x ", world.active_addrs[i]);
      }
      bprintf(&wb, "\n");
      bprintf(&wb, "Next scan in %d seconds.\n", search_at - now);
    }
    lock_release(worldlock);
    break;
  }
  case COMMAND_TYPE_FORCE_SCAN:
  {
    lock_take(worldlock);
    if (world.config == NULL)
    {
      bprintf(&wb, "Unconfigured\n");
    }
    else
    {
      struct timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      uint32_t now = ts.tv_sec;
      search_at = now;
      bprintf(&wb, "Triggering PSU scan...\n");
    }
    lock_release(worldlock);
    break;
  }
  case COMMAND_TYPE_DUMP_DATA_JSON:
  {
    lock_take(worldlock);
    if (world.config == NULL)
    {
      buf_write(&wb, "[]", 2);
    }
    else
    {
      struct timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      uint32_t now = ts.tv_sec;
      buf_write(&wb, "[", 1);
      int data_pos = 0;
      while (world.stored_data[data_pos] != NULL && data_pos < MAX_ACTIVE_ADDRS)
      {
        bprintf(&wb, "{\"addr\":%d,\"crc_fails\":%d,\"timeouts\":%d,"
                     "\"now\":%d,\"ranges\":[",
                world.stored_data[data_pos]->addr,
                world.stored_data[data_pos]->crc_errors,
                world.stored_data[data_pos]->timeout_errors, now);
        for (int i = 0; i < world.config->num_intervals; i++)
        {
          uint32_t time;
          register_range_data *rd = &world.stored_data[data_pos]->range_data[i];
          char *mem_pos = rd->mem_begin;
          bprintf(&wb, "{\"begin\":%d,\"readings\":[", rd->i->begin);
          // want to cut the list off early just before
          // the first entry with time == 0
          memcpy(&time, mem_pos, sizeof(time));
          for (int j = 0; j < rd->i->keep && time != 0; j++)
          {
            mem_pos += sizeof(time);
            bprintf(&wb, "{\"time\":%d,\"data\":\"", time);
            for (int c = 0; c < rd->i->len * 2; c++)
            {
              bprintf(&wb, "%02x", *mem_pos);
              mem_pos++;
            }
            buf_write(&wb, "\"}", 2);
            memcpy(&time, mem_pos, sizeof(time));
            if (time == 0)
            {
              break;
            }
            if ((j + 1) < rd->i->keep)
            {
              buf_write(&wb, ",", 1);
            }
          }
          buf_write(&wb, "]}", 2);
          if ((i + 1) < world.config->num_intervals)
          {
            buf_write(&wb, ",", 1);
          }
        }
        data_pos++;
        if (data_pos < MAX_ACTIVE_ADDRS && world.stored_data[data_pos] != NULL)
        {
          buf_write(&wb, "]},", 3);
        }
        else
        {
          buf_write(&wb, "]}", 2);
        }
      }
      buf_write(&wb, "]", 1);
    }
    lock_release(worldlock);
    break;
  }
  case COMMAND_TYPE_DUMP_DATA_INFO:
  {
    int data_pos = 0;

    lock_take(worldlock);
    if (world.config == NULL)
    {
      bprintf(&wb, "Unconfigured\n");
    }
    else
    {
      while (world.stored_data[data_pos] != NULL && data_pos < MAX_ACTIVE_ADDRS)
      {

        rackmon_print_info(&wb, world.stored_data[data_pos]);

        data_pos++;
      }
    }
    lock_release(worldlock);
    break;
  }
  case COMMAND_TYPE_PAUSE_MONITORING:
  {
    lock_take(worldlock);
    uint8_t was_paused = world.paused;
    world.paused = 1;
    buf_write(&wb, &was_paused, sizeof(was_paused));
    lock_release(worldlock);
    break;
  }
  case COMMAND_TYPE_START_MONITORING:
  {
    lock_take(worldlock);
    uint8_t was_started = !world.paused;
    world.paused = 0;
    buf_write(&wb, &was_started, sizeof(was_started));
    lock_release(worldlock);
    break;
  }
  default:
    BAIL("Unknown command type %d\n", cmd->type);
  }
cleanup:
  lock_release(worldlock);
  buf_close(&wb);
  return error;
}

typedef enum
{
  CONN_WAITING_LENGTH,
  CONN_WAITING_BODY
} rackmond_connection_state;

// receive the command as a length prefixed block
// (uint16_t, followed by data)
// this is all over a local socket, won't be doing
// endian flipping, clients should only be local procs
// compiled for the same arch
int handle_connection(int sock)
{
  int error = 0;
  rackmond_connection_state state = CONN_WAITING_LENGTH;
  char bodybuf[1024];
  uint16_t expected_len = 0;
  struct pollfd pfd;
  int recvret = 0;
  pfd.fd = sock;
  pfd.events = POLLIN | POLLERR | POLLHUP;
  // if you don't do anything for a whole second we bail
next:
  CHECKP(poll, poll(&pfd, 1, 1000));
  if (pfd.revents & (POLLERR | POLLHUP))
  {
    goto cleanup;
  }
  switch (state)
  {
  case CONN_WAITING_LENGTH:
    recvret = recv(sock, &expected_len, 2, MSG_DONTWAIT);
    if (recvret == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
    {
      goto next;
    }
    if (expected_len == 0 || expected_len > sizeof(bodybuf))
    {
      // bad length; bail
      goto cleanup;
    }
    state = CONN_WAITING_BODY;
    goto next;
    break;
  case CONN_WAITING_BODY:
    recvret = recv(sock, &bodybuf, expected_len, MSG_DONTWAIT);
    if (recvret == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
    {
      goto next;
    }
    CHECK(do_command(sock, (rackmond_command *)bodybuf));
  }
cleanup:
  close(sock);
  if (error != 0)
  {
    fprintf(stderr, "Warning: possible error handling user connection (%d)\n", error);
  }
  return 0;
}

int main(int argc, char **argv)
{
  if (getenv("RACKMOND_FOREGROUND") == NULL)
  {
    if (daemon(0, 0))
    {
      fprintf(stderr, "%s: Failed to run as daemon\n", RACKMOND_NAME);
      return -1;
    }
  }
  signal(SIGPIPE, SIG_IGN);

  int pid_file;
  int error = 0;

  pid_file = open(RACKMOND_PID_FILE, O_CREAT | O_RDWR, 0666);
  if (pid_file < 0)
  {
    fprintf(stderr, "%s: failed to open %s: %s\n",
            RACKMOND_NAME, RACKMOND_PID_FILE, strerror(errno));
    return -1;
  }
  if (flock(pid_file, LOCK_EX | LOCK_NB) != 0)
  {
    if (EWOULDBLOCK == errno)
    {
      fprintf(stderr, "%s: another instance is running. Exiting..\n",
              RACKMOND_NAME);
    }
    else
    {
      fprintf(stderr, "%s: failed to lock %s: %s\n",
              RACKMOND_NAME, RACKMOND_PID_FILE, strerror(errno));
    }
    close(pid_file);
    return -1;
  }

  // initialize logging
  if (obmc_log_init(RACKMOND_NAME, LOG_INFO, 0) != 0)
  {
    fprintf(stderr, "%s: failed to initialize logger: %s\n",
            RACKMOND_NAME, strerror(errno));
    return -1;
  }
  obmc_log_set_syslog(LOG_CONS, LOG_DAEMON);
  obmc_log_unset_std_stream();

  world.paused = 0;
  world.min_delay = 0;
  world.modbus_timeout = 300000;
  if (getenv("RACKMOND_TIMEOUT") != NULL)
  {
    world.modbus_timeout = atoll(getenv("RACKMOND_TIMEOUT"));
    fprintf(stderr, "Timeout from env: %dms\n",
            (world.modbus_timeout / 1000));
  }
  if (getenv("RACKMOND_MIN_DELAY") != NULL)
  {
    world.min_delay = atoll(getenv("RACKMOND_MIN_DELAY"));
    fprintf(stderr, "Mindelay from env: %dus\n", world.min_delay);
  }
  world.config = NULL;
  pthread_mutex_init(&world.lock, NULL);
  verbose = getenv("RACKMOND_VERBOSE") != NULL ? 1 : 0;

  OBMC_INFO("rackmon/modbus service starting");
  CHECK(open_rs485_dev(DEFAULT_TTY, &world.rs485));
  pthread_t monitoring_thread;
  pthread_create(&monitoring_thread, NULL, monitoring_loop, NULL);
  struct sockaddr_un local, client;
  int sock = socket(AF_UNIX, SOCK_STREAM, 0);
  strcpy(local.sun_path, "/var/run/rackmond.sock");
  local.sun_family = AF_UNIX;
  int socknamelen = sizeof(local.sun_family) + strlen(local.sun_path);
  unlink(local.sun_path);
  CHECKP(bind, bind(sock, (struct sockaddr *)&local, socknamelen));
  CHECKP(listen, listen(sock, 20));
  OBMC_INFO("rackmon/modbus service listening");
  while (1)
  {
    socklen_t clisocklen = sizeof(struct sockaddr_un);
    int clisock = accept(sock, (struct sockaddr *)&client, &clisocklen);
    CHECKP(accept, clisock);
    CHECK(handle_connection(clisock));
  }

cleanup:
  if (error != 0)
  {
    error = 1;
  }
  flock(pid_file, LOCK_UN);
  close(pid_file);
  return error;
}
