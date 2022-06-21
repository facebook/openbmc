#include "rackmond.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/serial.h>
#include <poll.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <openbmc/misc-utils.h>

#include "modbus.h"
#include "rackmon_platform.h"
#include "rackmon_parser.h"
#include "modbus.h"

#define DAEMON_NAME  "rackmond"

#define MAX_ACTIVE_ADDRS 24
#define MAX_RACKS        3
#define MAX_SHELVES      2
#define MAX_PSUS         3

#define MAX_PSUS_V3      6

/*
 * The PSUs understand baudrate as an integer.
 * 0: unable to set baudrate (uses B19200 for comms)
 * 1: B19200
 * 2: B38400
 * 3: B57600
 * 4: B115200
 *
 * If possible, increase the baudrate of each PSU to
 * the desired baudrate. If any PSUs are unable to set the
 * baudrate, then keep them all at B19200 (the default).
 */
const static speed_t BAUDRATE_VALUES[] = {
  B19200, B19200, B38400, B57600, B115200
};
#define DEFAULT_BAUDRATE_VALUE 1
#define DEFAULT_BAUDRATE B19200

// If a PSU has experience more than ALLOWABLE_CONSECUTIVE_BLIPS consecutive communication failures,
// then put it in timeout for NON_COMMUNICATION_TIMEOUT seconds
#define ALLOWABLE_CONSECUTIVE_BLIPS 10
#define NON_COMMUNICATION_TIMEOUT 630

#define REGISTER_PSU_STATUS 0x68
#define REGISTER_PSU_BAUDRATE 0xA3
#define REGISTER_PSU_TIMESTAMP 0x12A

#define REGISTER_PSU_STATUS_V3 0x3C
#define REGISTER_PSU_BAUDRATE_V3 0x5F
#define REGISTER_PSU_TIMESTAMP_V3 0x62

// This list overlaps with Modbus constants in modbus.h; ensure there are no duplicates
#define READ_ERROR_RESPONSE -2
#define WRITE_ERROR_RESPONSE -3
#define PSU_TIMEOUT_RESPONSE -6

// baudrate-changing commands need a higher timeout (half a second)
#define BAUDRATE_CMD_TIMEOUT 500000

// timeout can take up to 100ms to set
#define TIMESTAMP_CMD_TIMEOUT  100000

/*
 * Check for new PSUs every "PSU_SCAN_INTERVAL" seconds.
 */
#define PSU_SCAN_INTERVAL 120

/*
 * Write timestamp every 1 hour
 */
#define PSU_TIMESTAMP_UPDATE_INTERVAL (3600)

/*
 * A small amount of delay (in seconds) during PSU access, mainly for
 * reducing the average CPU usage.
 */
#define PSU_ACCESS_NICE_DELAY 3

/*
 * REG_INT_DATA_SIZE defines the memory size required to store a specific
 * "Register Interval": uint32_t is reserved for timestamp, while the
 * remaining is to store register values (per_register_size=sizeof(u16),
 * register_count=iv->len).
 */
#define REG_INT_DATA_SIZE(_iv) (sizeof(uint32_t) + \
                                (sizeof(uint16_t) * (_iv)->len))

#define TIME_UPDATE(_t)  do {                        \
  struct timespec _ts;                               \
  if (clock_gettime(CLOCK_REALTIME, &_ts) != 0) {    \
    OBMC_ERROR(errno, "failed to get current time"); \
    (_t) = 0;                                        \
  } else {                                           \
    (_t) = _ts.tv_sec;                               \
  }                                                  \
} while (0)

static int scanning = 0;

typedef struct {
  // hold this for the duration of a command
  pthread_mutex_t lock;
#define dev_lock(_d)   mutex_lock_helper(&((_d)->lock), "dev_lock")
#define dev_unlock(_d) mutex_unlock_helper(&((_d)->lock), "dev_lock")
  int tty_fd;
} rs485_dev_t;

typedef struct {
  uint16_t begin; /* starting register address */
  int num;        /* number of registers */
} reg_req_t;

/*
 * Global rackmond config structure, protected by its mutex lock.
 */
static struct {
  // global rackmond lock
  pthread_mutex_t lock;
#define global_lock()   mutex_lock_helper(&rackmond_config.lock, "global_lock")
#define global_unlock() mutex_unlock_helper(&rackmond_config.lock, "global_lock")

  // number of register read commands to send to each PSU
  int num_reqs;
  // register read commands (begin+length)
  reg_req_t *reqs;
  monitoring_config *config;

  uint8_t num_active_addrs;
  bool first_scan_done[MAX_RACKS][MAX_SHELVES][MAX_PSUS_V3];
  bool ignored_psus[MAX_RACKS][MAX_SHELVES][MAX_PSUS_V3];
  uint8_t active_addrs[MAX_ACTIVE_ADDRS];
  psu_datastore_t* stored_data[MAX_ACTIVE_ADDRS];
  FILE *status_log;

  // timeout in nanosecs
  int modbus_timeout;

  // inter-command delay
  int min_delay;

  int paused;

  // multi port for ft4232
  bool multi_port;
  // swap address for port2 and port3 in W400 MP Respin
  bool swap_addr;
  //ORV2:version=2;ORV3:version=3
  int rack_version;

  // the value we will auto-adjust to if possible
  speed_t desired_baudrate;

  rs485_dev_t rs485[3];
} rackmond_config = {
  .lock = PTHREAD_MUTEX_INITIALIZER,
  .modbus_timeout = 300000,
  .desired_baudrate = DEFAULT_BAUDRATE,
};

static int mutex_lock_helper(pthread_mutex_t *lock, const char *name)
{
  int error = pthread_mutex_lock(lock);
  if (error != 0) {
    OBMC_ERROR(error, "failed to acquire %s", name);
    return -1;
  }

  return 0;
}

static int mutex_unlock_helper(pthread_mutex_t *lock, const char *name)
{
  int error = pthread_mutex_unlock(lock);
  if (error != 0) {
    OBMC_ERROR(error, "failed to release %s", name);
    return -1;
  }

  return 0;
}

static int buf_open(write_buf_t* buf, int fd, size_t len)
{
  int error = 0;
  char* bufmem;

  bufmem = malloc(len);
  if(!bufmem) {
    BAIL("Couldn't allocate write buffer of len %zd for fd %d\n", len, fd);
  }
  buf->buffer = bufmem;
  buf->pos = 0;
  buf->len = len;
  buf->fd = fd;

cleanup:
  return error;
}

static ssize_t buf_flush(write_buf_t* buf)
{
  int ret;

  ret = write(buf->fd, buf->buffer, buf->pos);
  if(ret > 0) {
    memmove(buf->buffer, buf->buffer + ret, buf->pos - ret);
    buf->pos -= ret;
  }

  return ret;
}

static ssize_t buf_write(write_buf_t* buf, void* from, size_t len) {
  int ret;

  // write will not fill buffer, only memcpy
  if((buf->pos + len) < buf->len) {
    memcpy(buf->buffer + buf->pos, from, len);
    buf->pos += len;
    return len;
  }

  // write would exceed buffer, flush first
  ret = buf_flush(buf);
  if (ret < 0) {
    return ret;
  } else if (buf->pos != 0) {
    // write() was interrupted but partially succeeded -- the buffer partially
    // flushed but no bytes of the requested buf_write went through
    return 0;
  }

  if (len > buf->len) {
    // write is larger than buffer, skip buffer
    return write(buf->fd, from, len);
  } else {
    return buf_write(buf, from, len);
  }
}

int buf_printf(write_buf_t* buf, const char* format, ...)
{
  char tmpbuf[512];
  int error = 0;
  int ret;
  va_list args;

  va_start(args, format);
  ret = vsnprintf(tmpbuf, sizeof(tmpbuf), format, args);
  ERR_EXIT(ret);

  if(ret > sizeof(tmpbuf)) {
    BAIL("truncated buf_printf (%d bytes truncated to %d)",
         ret, sizeof(tmpbuf));
  }
  ERR_EXIT(buf_write(buf, tmpbuf, ret));

cleanup:
  va_end(args);
  return error;
}

static int buf_close(write_buf_t* buf)
{
  int error = 0;
  int fret = buf_flush(buf);
  int cret = close(buf->fd);

  free(buf->buffer);
  buf->buffer = NULL;
  ERR_EXIT(fret);
  ERR_LOG_EXIT(cret, "failed to close buf->fd");

cleanup:
  return error;
}

static speed_t int_to_baudrate(int val)
{
  speed_t baudrate = DEFAULT_BAUDRATE;
  switch (val) {
    case 19200:
      baudrate = B19200;
      break;
    case 38400:
      baudrate = B38400;
      break;
    case 57600:
      baudrate = B57600;
      break;
    case 115200:
      baudrate = B115200;
      break;
    default:
      OBMC_WARN("Baudrate %d unknown", val);
      break;
  }
  return baudrate;
}

static uint16_t baudrate_to_value(speed_t baudrate)
{
  uint16_t value = 0;
  switch (baudrate) {
    case B19200:
      value = 1;
      break;
    case B38400:
      value = 2;
      break;
    case B57600:
      value = 3;
      break;
    case B115200:
      value = 4;
      break;
    default:
      OBMC_WARN("Baudrate %s has no corresponding value", baud_to_str(baudrate));
      break;
  }
  return value;
}

static int lookup_data_slot(uint8_t addr)
{
  int i;

  for (i = 0; i < ARRAY_SIZE(rackmond_config.stored_data); i++) {
    if (rackmond_config.stored_data[i] == NULL ||
        rackmond_config.stored_data[i]->addr == addr) {
      return i; /* Found the slot */
    }
  }

  return -1; /* No slot available. */
}

static char psu_address(int rack, int shelf, int psu)
{
  if (rackmond_config.swap_addr) {
    if ( rack == 1) {
      rack = 2;
    } else if ( rack == 2) {
      rack = 1;
    }
  }
  int rack_a = ((rack & 3) << 3);
  int shelf_a = ((shelf & 1) << 2);
  int psu_a = (psu & 3);

  return 0xA0 | rack_a | shelf_a | psu_a;
}

/*
 * PSU/BBU(2 bit)|shelf(1 bit)|rack(2 bit)|PSU(3 bit)
 */
static char psu_address_V3(int rack, int shelf, int psu)
{
  int type_a = ((3 & 3) << 6);
  int shelf_a = ((shelf & 1) << 5);
  int rack_a = ((rack & 3) <<3);
  int psu_a = (psu & 7);

  return type_a | shelf_a | rack_a | psu_a;
}

static int psu_location(int addr, int *rack, int *shelf, int *psu)
{
  if ((addr & 0xA0)) {
    *psu = addr & 3;
    *shelf = (addr >> 2) & 1;
    *rack = (addr >> 3) & 3;
	  if (rackmond_config.swap_addr) {
      if ( *rack == 1) {
        *rack = 2;
      } else if ( *rack == 2) {
        *rack = 1;
      }
	  }
    return 0;
  }
  return -1;
}

static int psu_location_V3(int addr, int *rack, int *shelf, int *psu)
{
  if ((addr & 0xC0)) {
    *psu = addr & 7;
    *shelf = (addr >> 5) & 1;
    *rack = (addr >> 3) & 3;
    return 0;
  }
  return -1;
}

static int check_psu_comms(psu_datastore_t *psu)
{
  if (psu == NULL) {
    return 0;
  }

  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);

  if (now.tv_sec > psu->last_comms + NON_COMMUNICATION_TIMEOUT) {
    psu->baudrate = DEFAULT_BAUDRATE;
    psu->consecutive_failures = 0;
    psu->timeout_mode = false;
  }

  if (psu->timeout_mode) {
    return -1;
  }

  if (psu->consecutive_failures > ALLOWABLE_CONSECUTIVE_BLIPS) {
    OBMC_INFO("Putting PSU %02x in timeout for the next %d seconds after %d consecutive communication failures",
              psu->addr, NON_COMMUNICATION_TIMEOUT, psu->consecutive_failures);
    psu->consecutive_failures = 0;
    psu->baudrate = DEFAULT_BAUDRATE;
    psu->timeout_mode = true;
    psu->last_comms = now.tv_sec;
    return -1;
  }

  return 0;
}

static void update_psu_comms(psu_datastore_t *psu, bool success)
{
  if (psu == NULL) {
    return;
  }

  TIME_UPDATE(psu->last_comms);

  if (success) {
    psu->consecutive_failures = 0;
  } else {
    psu->consecutive_failures++;
  }
}

/*
 * Note: Some platform like Wedge400 has a lot of modbus timeout errors and
 * sometimes BMC can not detect one of the Rack PSUs. This method allows
 * us to conditionally set the number of retries based on whether the
 * PSU is previously known and/or is it the first time we are trying to
 * detect a PSU.
 */
static int psu_retry_limit(psu_datastore_t *info, int addr)
{
  int rack, shelf, psu;
  int (*get_psu_location)(int, int*, int*, int*);

  if (rackmond_config.rack_version==2) {
    get_psu_location=psu_location;
  } else if (rackmond_config.rack_version==3) {
    get_psu_location=psu_location_V3;
  } else {
    OBMC_INFO("rackmon version error: current:%d, set by default vertion 2", rackmond_config.rack_version);
    get_psu_location=psu_location;
  }

  /* If we are communicating with a previously known PSU,
   * then go ahead and retry MAX_RETRY times on failure */
  if (info) {
    return MAX_RETRY;
  }

  /* Do not waste time retrying on invalid PSU addresses */
  if (get_psu_location(addr, &rack, &shelf, &psu)) {
    return 1;
  }

  /* Do not waste time retrying commands to PSU addresses
   * which are most probably never going to exist. This
   * flag being set means we already have retried detecting
   * this PSU in the past  */
  if (rackmond_config.first_scan_done[rack][shelf][psu]) {
    return 1;
  }

  /* This is an unknown PSU but also the first time we are
   * trying to communicate with this address. Retry to prevent
   * missing detection of a PSU. Set the flag so that the
   * next scan we do not waste time retrying on this address.
   */
  rackmond_config.first_scan_done[rack][shelf][psu] = true;
  return MAX_RETRY;
}

static int modbus_command(rs485_dev_t* dev, int timeout, char* cmd_buf,
                          size_t cmd_size, char* resp_buf, size_t resp_size,
                          size_t exp_resp_len, speed_t baudrate, const char *caller) {
  modbus_req req;
  int error = 0;
  useconds_t delay = rackmond_config.min_delay;
  psu_datastore_t* psu = NULL;
  int max_retry;

  int slot = lookup_data_slot(cmd_buf[0]);
  if (slot >= 0) {
    psu = rackmond_config.stored_data[slot];
  }

  if (check_psu_comms(psu) < 0) {
    return PSU_TIMEOUT_RESPONSE;
  }

  req.tty_fd = dev->tty_fd;
  req.modbus_cmd = cmd_buf;
  req.cmd_len = cmd_size;
  req.dest_buf = resp_buf;
  req.dest_limit = resp_size;
  req.timeout = timeout;
  req.expected_len = (exp_resp_len != 0 ? exp_resp_len : resp_size);
  req.scan = scanning;

  if (dev_lock(dev) != 0) {
    return -1;
  }

  max_retry = psu_retry_limit(psu, (int)cmd_buf[0]);

  for (int retry = 0; retry < max_retry; retry++) {
    CHECK_LATENCY_START();
    error = modbuscmd(&req, baudrate, caller);
    CHECK_LATENCY_END(
        "rackmond::%s::modbuscmd cmd=(%#x,%#x,%#x,%#x,%#x,%#x), cmd_len=%lu, resp_len=%lu",
        caller,
        cmd_buf[0], cmd_buf[1],
        cmd_size > 2 ? cmd_buf[2] : 0,
        cmd_size > 3 ? cmd_buf[3] : 0,
        cmd_size > 4 ? cmd_buf[4] : 0,
        cmd_size > 5 ? cmd_buf[5] : 0,
        (unsigned long)req.cmd_len, (unsigned long)req.expected_len);

    /* If this is an active PSU, then record the errors we see
     * communicating to this PSU */
    if (psu) {
      if (error == MODBUS_BAD_CRC) {
        psu->crc_errors++;
      } else if (error == MODBUS_RESPONSE_TIMEOUT) {
        psu->timeout_errors++;
      }
    }

    if (delay != 0) {
      usleep(delay);
    }
    if (error >= 0) {
      break;
    }
  }

  dev_unlock(dev);
  update_psu_comms(psu, (error >= 0));

  if (error < 0) {
    return error;
  }

  return req.dest_len;
}

/*
 * Read Holding Register (Function 3) Request Header Size:
 *   Slave_Addr (1-byte) + Function (1-byte) + Starting_Reg_Addr (2-bytes) +
 *   Reg_Count (2-bytes).
 */
#define MODBUS_FUN3_REQ_HDR_SIZE        6

/*
 * Read Holding Register (Function 3) Response packet size:
 *   Slave_Addr (1-byte) + Function (1-byte) + Data_Byte_Count (1-byte) +
 *   Reg_Value_Array (2 * Reg_Count) + CRC (2-byte)
 */
#define MODBUS_FUN3_RESP_PKT_SIZE(nreg) (2 * (nreg) + 5)

static int read_holding_reg(rs485_dev_t *dev, int timeout, uint8_t slave_addr,
                            uint16_t reg_start_addr, uint16_t reg_count,
                            uint16_t *out, speed_t baudrate)
{
  int error = 0;
  int dest_len;
  char command[MODBUS_FUN3_REQ_HDR_SIZE];
  size_t resp_len = MODBUS_FUN3_RESP_PKT_SIZE(reg_count);
  char *resp_buf;

  resp_buf = malloc(resp_len);
  if (resp_buf == NULL) {
    OBMC_WARN("failed to allocate response buffer");
    return -1;
  }

  command[0] = slave_addr;
  command[1] = MODBUS_READ_HOLDING_REGISTERS;
  command[2] = reg_start_addr >> 8;
  command[3] = reg_start_addr & 0xFF;
  command[4] = reg_count >> 8;
  command[5] = reg_count & 0xFF;

  CHECK_LATENCY_START();
  dest_len = modbus_command(dev, timeout, command, MODBUS_FUN3_REQ_HDR_SIZE,
                            resp_buf, resp_len, 0, baudrate, "read_holding_reg");
  CHECK_LATENCY_END("rackmond::read_holding_reg::modbus_command cmd_len=%lu, resp_len=%lu status=%d",
      (unsigned long)MODBUS_FUN3_REQ_HDR_SIZE, (unsigned long)resp_len, dest_len);
  ERR_EXIT(dest_len);

  if (dest_len >= 5) {
    memcpy(out, resp_buf + 3, reg_count * 2);
  } else {
    OBMC_WARN("Unexpected short but CRC correct response!\n");
    error = -1;
    goto cleanup;
  }
  if (resp_buf[0] != slave_addr) {
    OBMC_WARN("Got response from addr %02x when expected %02x\n",
              resp_buf[0], slave_addr);
    error = -1;
    goto cleanup;
  }
  if (resp_buf[1] != MODBUS_READ_HOLDING_REGISTERS) {
    // got an error response instead of a read registers response
    // likely because the requested registers aren't available on
    // this model (e.g. stingray)
    error = READ_ERROR_RESPONSE;
    goto cleanup;
  }
  if (resp_buf[2] != (reg_count * 2)) {
    OBMC_WARN("Got %d register data bytes when expecting %d\n",
              resp_buf[2], (reg_count * 2));
    error = -1;
    goto cleanup;
  }

cleanup:
  free(resp_buf);
  return error;
}

/*
 * Write Holding Register (Function 6) Request Header Size:
 *   Slave_Addr (1-byte) + Function (1-byte) + Starting_Reg_Addr (2-bytes) +
 *   Reg_Value (2-bytes).
 */
#define MODBUS_FUN6_REQ_HDR_SIZE  (6)

/*
 * Write Holding Register (Function 6) Response packet size:
 *   Same as request header size, plus CRC (2-bytes).
 */
#define MODBUS_FUN6_RESP_PKT_SIZE (8)

static int write_holding_reg(rs485_dev_t *dev, int timeout, uint8_t slave_addr,
                             uint16_t reg_start_addr, uint16_t reg_value,
                             uint16_t *written_reg_value, speed_t baudrate)
{
  int dest_len;
  char command[MODBUS_FUN6_REQ_HDR_SIZE];
  char resp_buf[MODBUS_FUN6_RESP_PKT_SIZE];

  command[0] = slave_addr;
  command[1] = MODBUS_WRITE_HOLDING_REGISTER_SINGLE;
  command[2] = reg_start_addr >> 8;
  command[3] = reg_start_addr & 0xFF;
  command[4] = reg_value >> 8;
  command[5] = reg_value & 0xFF;

  CHECK_LATENCY_START();
  dest_len = modbus_command(dev, timeout, command, MODBUS_FUN6_REQ_HDR_SIZE,
                            resp_buf, MODBUS_FUN6_RESP_PKT_SIZE, 0,
                            baudrate, "write_holding_reg");
  CHECK_LATENCY_END("rackmond::write_holding_reg::modbus_command cmd_len=%lu, resp_len=%lu status=%d",
      (unsigned long)MODBUS_FUN6_REQ_HDR_SIZE, (unsigned long)MODBUS_FUN6_RESP_PKT_SIZE, dest_len);
  if (dest_len < 0) {
    OBMC_WARN("modbuscmd for addr 0x%02x returned %d\n", slave_addr, dest_len);
    return dest_len;
  }
  if (dest_len < 4) {
    OBMC_WARN("Unexpected short but CRC correct response!\n");
    return -1;
  }
  if (resp_buf[0] != slave_addr) {
    OBMC_WARN("Got response from addr %02x when expected %02x\n",
              resp_buf[0], slave_addr);
    return -1;
  }
  if (resp_buf[1] != MODBUS_WRITE_HOLDING_REGISTER_SINGLE) {
    // got an error response instead of a write registers response
    OBMC_WARN("Unexpected function %d when expected %d\n",
        (int)resp_buf[1], (int)MODBUS_WRITE_HOLDING_REGISTER_SINGLE);
    return WRITE_ERROR_RESPONSE;
  }
  if (written_reg_value) {
    *written_reg_value = (uint16_t)resp_buf[4] << 8 | resp_buf[5];
  }
  return 0;
}

/*
 * Write Holding Register multiple (Function 16) Request Header Size:
 *   Slave_Addr (1-byte) + Function (1-byte) + Starting_Reg_Addr (2-bytes) +
 *   Reg_Count (2-bytes) + Bytes (1-bytes) +
 *   Reg_Value_Arr (Reg_Count * 2 bytes),
 */
#define MODBUS_FUN16_REQ_HDR_SIZE(regs)  (7 + (2 * (regs)))

/*
 * Write Holding Register multiple (Function 16) Response packet size:
 *   Slave_Addr (1-byte) + Function (1-byte) + Starting_Reg_Addr (2-bytes) +
 *   Reg_Count (2-bytes) + CRC (2-bytes)
 */
#define MODBUS_FUN16_RESP_PKT_SIZE (8)

int write_holding_regs(rs485_dev_t *dev, int timeout, uint8_t slave_addr,
                             uint16_t reg_start_addr, uint16_t reg_count,
                             uint16_t *reg_value, speed_t baudrate)
{
  int dest_len, i;
  size_t cmd_len = MODBUS_FUN16_REQ_HDR_SIZE(reg_count);
  char command[cmd_len];
  char resp_buf[MODBUS_FUN16_RESP_PKT_SIZE];
  uint16_t written_reg;

  command[0] = slave_addr;
  command[1] = MODBUS_WRITE_HOLDING_REGISTER_MULTIPLE;
  command[2] = reg_start_addr >> 8;
  command[3] = reg_start_addr & 0xFF;
  command[4] = reg_count >> 8;
  command[5] = reg_count & 0xFF;
  command[6] = reg_count * 2;
  for (i = 0; i < reg_count; i++) {
    command[7 + (i * 2)] = reg_value[i] >> 8;
    command[8 + (i * 2)] = reg_value[i] & 0xFF;
  }

  CHECK_LATENCY_START();
  dest_len = modbus_command(dev, timeout, command, cmd_len,
                            resp_buf, MODBUS_FUN16_RESP_PKT_SIZE, 0,
                            baudrate, "write_holding_regs");
  CHECK_LATENCY_END("rackmond::write_holding_regs::modbus_command cmd_len=%lu, resp_len=%lu status=%d",
      (unsigned long)cmd_len, (unsigned long)MODBUS_FUN16_RESP_PKT_SIZE, dest_len);
  if (dest_len < 0) {
    OBMC_WARN("modbuscmd for addr 0x%02x returned %d\n", slave_addr, dest_len);
    return dest_len;
  }
  if (dest_len < 4) {
    OBMC_WARN("Unexpected short but CRC correct response!\n");
    return -1;
  }
  if (resp_buf[0] != slave_addr) {
    OBMC_WARN("Got response from addr %02x when expected %02x\n",
              resp_buf[0], slave_addr);
    return -1;
  }
  if (resp_buf[1] != MODBUS_WRITE_HOLDING_REGISTER_MULTIPLE) {
    // got an error response instead of a write registers response
    OBMC_WARN("Unexpected function %d when expected %d\n",
        (int)resp_buf[1], (int)MODBUS_WRITE_HOLDING_REGISTER_SINGLE);
    return WRITE_ERROR_RESPONSE;
  }
  written_reg = (uint16_t)resp_buf[4] << 8 | resp_buf[5];
  if (written_reg != reg_count) {
    OBMC_WARN("Unexpected number of registers written. Expected: %d Actual: %d\n",
        reg_count, written_reg);
    return WRITE_ERROR_RESPONSE;
  }
  return 0;
}

int set_psu_timestamp(psu_datastore_t *psu, uint32_t unixtime)
{
  uint16_t values[2];
  int rack=0,shelf=0,psu_no=0;
  int (*get_psu_location)(int, int*, int*, int*);
  int psu_timestamp_register;

  if (rackmond_config.rack_version==2) {
    get_psu_location=psu_location;
    psu_timestamp_register=REGISTER_PSU_TIMESTAMP;
  } else if (rackmond_config.rack_version==3) {
    get_psu_location=psu_location_V3;
    psu_timestamp_register=REGISTER_PSU_TIMESTAMP_V3;
  } else {
    OBMC_INFO("rackmon version error: current:%d, set by default 2", rackmond_config.rack_version);
    get_psu_location=psu_location;
    psu_timestamp_register=REGISTER_PSU_TIMESTAMP;
  }

  if (get_psu_location(psu->addr,&rack,&shelf,&psu_no)) {
    return -1;
  }
  if (psu_location(psu->addr,&rack,&shelf,&psu_no)) {
    return -1;
  }
  if (unixtime == 0) {
    unixtime = time(NULL);
  }
  OBMC_INFO("Writing timestamp 0x%08x to PSU: 0x%02x", unixtime, psu->addr);
  values[0] = unixtime >> 16;
  values[1] = unixtime & 0xFFFF;
  if (rackmond_config.multi_port) {
    return write_holding_regs(&rackmond_config.rs485[rack], TIMESTAMP_CMD_TIMEOUT,
        psu->addr, psu_timestamp_register, 2, values, psu->baudrate);
  } else {
    return write_holding_regs(&rackmond_config.rs485[0], TIMESTAMP_CMD_TIMEOUT,
        psu->addr, psu_timestamp_register, 2, values, psu->baudrate);
  }
}

static int sub_uint8s(const void* a, const void* b)
{
  return (*(uint8_t*)a) - (*(uint8_t*)b);
}

/*
 * Check the baudrate used by this PSU.
 * If a higher baudrate is desired, and all PSUs in the rack support it,
 * then raise the baudrate to the desired value for just this PSU.
 */
static int check_psu_baudrate(psu_datastore_t *psu, speed_t *baudrate_out)
{
  int pos, err;
  bool supported = true;
  uint16_t value;
  uint16_t written_value;
  psu_datastore_t *mdata;
  int rack=0,shelf=0,psu_no=0;
  
  int (*get_psu_location)(int, int*, int*, int*);
  int psu_baudrate_register;

  if (rackmond_config.rack_version==2) {
    get_psu_location=psu_location;
    psu_baudrate_register=REGISTER_PSU_BAUDRATE;
  } else if (rackmond_config.rack_version==3) {
    get_psu_location=psu_location_V3;
    psu_baudrate_register=REGISTER_PSU_BAUDRATE_V3;
  } else {
    OBMC_INFO("rackmon version error: current:%d, set by default version 2", rackmond_config.rack_version);
    get_psu_location=psu_location;
    psu_baudrate_register=REGISTER_PSU_BAUDRATE;
  }
  
  if (get_psu_location(psu->addr,&rack,&shelf,&psu_no)) {
    return -1;
  }

  if (psu == NULL) {
    OBMC_WARN("check_psu_baudrate received a null PSU argument, assuming default baudrate");
    *baudrate_out = DEFAULT_BAUDRATE;
    return 0;
  }
  if (psu_location(psu->addr,&rack,&shelf,&psu_no)) {
    return -1;
  }


  // only attempt to raise the baudrate if it doesn't already match the desired baudrate
  if (psu->baudrate == rackmond_config.desired_baudrate) {
    *baudrate_out = psu->baudrate;
    return 0;
  }

  // only attempt to raise the baudrate if all connected PSUs support it
  for (pos = 0; pos < ARRAY_SIZE(rackmond_config.stored_data); pos++) {
    mdata = rackmond_config.stored_data[pos];
    if (mdata == NULL) {
      continue;
    }
    if (!mdata->supports_baudrate) {
      supported = false;
    }
  }

  if (!supported) {
    *baudrate_out = psu->baudrate;
    return 0;
  }

  // now attempt to raise the baudrate for this PSU
  value = baudrate_to_value(rackmond_config.desired_baudrate);
  if (rackmond_config.multi_port) {
    err = write_holding_reg(&rackmond_config.rs485[rack], BAUDRATE_CMD_TIMEOUT,
                            psu->addr, psu_baudrate_register, value,
                            &written_value, psu->baudrate);
  } else {
    err = write_holding_reg(&rackmond_config.rs485[0], BAUDRATE_CMD_TIMEOUT,
                            psu->addr, psu_baudrate_register, value,
                            &written_value, psu->baudrate);
  }

  // if unsuccessful, assume that the unit's baudrate didn't change
  if (err != 0) {
    OBMC_WARN("Could not set PSU at addr %02x to desired baudrate", psu->addr);
  } else {
    pos = written_value;
    psu->baudrate = BAUDRATE_VALUES[pos];
  }

  *baudrate_out = psu->baudrate;
  return err;
}

/*
 * Scan connected PSUs. Executed in monitoring thread.
 */
static int check_active_psus(void)
{
  int num_psus;
  int rack, shelf, psu, offset;

  char (*get_psu_address)(int, int, int);
  int max_psu_num, psu_status_register;

  if (rackmond_config.rack_version==2) {
    get_psu_address=psu_address;
    max_psu_num=MAX_PSUS;
    psu_status_register=REGISTER_PSU_STATUS;
  } else if (rackmond_config.rack_version==3) {
    get_psu_address=psu_address_V3;
    max_psu_num=MAX_PSUS_V3;
    psu_status_register=REGISTER_PSU_STATUS_V3;
  } else {
    OBMC_INFO("rackmon version error: current:%d, set by default vertion 2", rackmond_config.rack_version);
    get_psu_address=psu_address;
    max_psu_num=MAX_PSUS;
    psu_status_register=REGISTER_PSU_STATUS;
  }

  if (global_lock() != 0) {
    return -1;
  }
  if (rackmond_config.paused == 1 || rackmond_config.config == NULL) {
    global_unlock();
    return 0;
  }

  offset = 0;
  scanning = 1;
  for (rack = 0; rack < MAX_RACKS; rack++) {
    for (shelf = 0; shelf < MAX_SHELVES; shelf++) {
      for (psu = 0; psu < max_psu_num; psu++) {
        int err, slot;
        uint16_t output = 0;
        speed_t baudrate;
        char addr = get_psu_address(rack, shelf, psu);

        if (rackmond_config.ignored_psus[rack][shelf][psu]) {
          continue;
        }

        slot = lookup_data_slot((uint8_t) addr);
        if (slot < 0 || rackmond_config.stored_data[slot] == NULL) {
          // No allocated slot exists for psu at this addr yet, not a real error
          baudrate = DEFAULT_BAUDRATE;
        } else {
          err = check_psu_baudrate(rackmond_config.stored_data[slot], &baudrate);
          if (err != 0) {
            OBMC_WARN("Unable to check baudrate for PSU at addr %02x", addr);
            continue;
          }
        }

        if (rackmond_config.multi_port) {
          err = read_holding_reg(&rackmond_config.rs485[rack],
                                rackmond_config.modbus_timeout, addr,
                                psu_status_register, 1, &output, baudrate);
        } else {
          err = read_holding_reg(&rackmond_config.rs485[0],
                                rackmond_config.modbus_timeout, addr,
                                psu_status_register, 1, &output, baudrate);
        }
        if (err != 0) {
          continue;
        }
        if (offset >= ARRAY_SIZE(rackmond_config.active_addrs)) {
          OBMC_WARN("Too many PSUs detected: addr %#02x ignored.", addr);
          continue;
        }

        OBMC_INFO("detected PSU at addr %#02x", addr);
        rackmond_config.active_addrs[offset++] = addr;
      }
    }
  }
  num_psus = rackmond_config.num_active_addrs = offset;

  //its the only stdlib sort
  qsort(rackmond_config.active_addrs, rackmond_config.num_active_addrs,
        sizeof(uint8_t), sub_uint8s);

  scanning = 0;
  global_unlock();

  return num_psus;
}

/*
 * "psu_datastore_t" points to the memory area which stores all the
 * monitored registers of a specific PSU (identified by address), and
 * logically the memory area can be divided into 3 sub-areas:
 *
 * M = "config->num_intervals", and N = "iv->keep" in below picture:
 *
 * |--------------------------|
 * |                          |
 * | struct psu_datastore_t   |
 * |                          |
 * |--------------------------|
 * | reg_range_data_t[0]      |
 * | reg_range_data_t[1]      |
 * | ......                   |
 * | reg_range_data_t[M-1]    |
 * |--------------------------|
 * | timestamp(4-byte)        |
 * | reg_interval_0,keep#0    |
 * | timestamp(4-byte)        |
 * | reg_interval_0,keep#1    |
 * |......                    |
 * | timestamp(4-byte)        |
 * | reg_interval_0,keep#N-1] |
 * | timestamp(4-byte)        |
 * | reg_interval_1,keep#0    |
 * | timestamp(4-byte)        |
 * | reg_interval_1,keep#1    |
 * |......                    |
 * | timestamp(4-byte)        |
 * | reg_interval_M-1,keep#0  |
 * |......                    |
 * | timestamp(4-byte)        |
 * | reg_interval_M-1,keep#N-1|
 * |--------------------------|
 */
static psu_datastore_t* alloc_monitoring_data(uint8_t addr)
{
  void *mem;
  size_t size;
  psu_datastore_t *d;
  monitor_interval *iv;
  int i, pitch, data_size;

  size = sizeof(psu_datastore_t) +
         sizeof(reg_range_data_t) * rackmond_config.config->num_intervals;

  for (i = 0; i < rackmond_config.config->num_intervals; i++) {
    iv = &rackmond_config.config->intervals[i];
    pitch = REG_INT_DATA_SIZE(iv);
    data_size = pitch * iv->keep;
    size += data_size;
  }

  d = calloc(1, size);
  if (d == NULL) {
    OBMC_WARN("Failed to allocate memory for sensor data.\n");
    return NULL;
  }

  d->addr = addr;
  d->crc_errors = 0;
  d->timeout_errors = 0;
  d->baudrate = DEFAULT_BAUDRATE;
  d->supports_baudrate = false;
  d->consecutive_failures = 0;
  d->timeout_mode = false;
  TIME_UPDATE(d->last_comms);
  mem = d;
  mem += (sizeof(psu_datastore_t) +
          sizeof(reg_range_data_t) * rackmond_config.config->num_intervals);

  for (i = 0; i < rackmond_config.config->num_intervals; i++) {
    iv = &rackmond_config.config->intervals[i];
    pitch = REG_INT_DATA_SIZE(iv);
    data_size = pitch * iv->keep;
    d->range_data[i].i = iv;
    d->range_data[i].mem_begin = mem;
    d->range_data[i].mem_pos = 0;
    mem += data_size;
  }

  return d;
}

static int sub_storeptrs(const void* va, const void *vb)
{
  //more *s than i like :/
  psu_datastore_t* a = *(psu_datastore_t**)va;
  psu_datastore_t* b = *(psu_datastore_t**)vb;

  //nulls to the end
  if (b == NULL && a == NULL) {
    return 0;
  }
  if (b == NULL) {
    return -1;
  }
  if (a == NULL) {
    return 1;
  }
  return (int)(a->addr - b->addr);
}

static int alloc_monitoring_datas(void)
{
  int i;
  int error = 0;

  if (global_lock() != 0) {
    return -1;
  }

  if (rackmond_config.config == NULL) {
    global_unlock();
    return 0;
  }

  qsort(rackmond_config.stored_data, MAX_ACTIVE_ADDRS,
        sizeof(psu_datastore_t*), sub_storeptrs);

  for (i = 0; i < rackmond_config.num_active_addrs; i++) {
    int slot;
    uint8_t addr = rackmond_config.active_addrs[i];

    slot = lookup_data_slot(addr);
    if (slot < 0) {
      OBMC_WARN("no data space left for psu addr %#02x", addr);
      error = -1;
      break;
    }

    if (rackmond_config.stored_data[slot] == NULL) {
      // this will only be logged once per address
      OBMC_INFO("Detected PSU at address 0x%02x", addr);

      rackmond_config.stored_data[slot] = alloc_monitoring_data(addr);
      if (rackmond_config.stored_data[slot] == NULL) {
        OBMC_WARN("failed to allocate datastore for psu addr %#02x", addr);
        error = -1;
        break;
      }
    }
  } /* for */

  global_unlock();
  return error;
}

static int uart_rs485_open(struct rackmon_io_handler *handler)
{
  int fd, ret;
  struct serial_rs485 rs485conf;

  fd = open(handler->dev_path, O_RDWR | O_NOCTTY);
  if (fd < 0) {
    OBMC_ERROR(errno, "failed to open %s", handler->dev_path);
    return -1;
  }

  /*
   * NOTE: "SER_RS485_RTS_AFTER_SEND" and "SER_RS485_RX_DURING_TX" flags
   * are not handled in kernel 4.1, but they are required in the latest
   * kernel.
   */
  memset(&rs485conf, 0, sizeof(rs485conf));
  rs485conf.flags = SER_RS485_ENABLED;
  rs485conf.flags |= (SER_RS485_RTS_AFTER_SEND | SER_RS485_RX_DURING_TX);

  ret = ioctl(fd, TIOCSRS485, &rs485conf);
  if (ret < 0) {
    int saved_errno = errno;

    OBMC_ERROR(errno, "failed to turn on RS485 mode");
    close(fd);  /* ignore errors */
    errno = saved_errno;

    return -1;
  }

  return fd;
}

/*
 * I/O operation handler for UART-RS485 transactions, used by wedge40 and
 * wedge100 as of now.
 */
static struct rackmon_io_handler uart_rs485_io = {
  .dev_path = DEFAULT_TTY,
  .open = uart_rs485_open,
};

/*
 * Set "rackmon_io" to "uart_rs485_io" by default, mainly because we want
 * to minimize changes to the existing wedge40 and wedge100 platforms.
 */
struct rackmon_io_handler *rackmon_io = &uart_rs485_io;

static void rs485_device_cleanup(rs485_dev_t *dev)
{
  if (dev->tty_fd >= 0) {
    pthread_mutex_destroy(&dev->lock);
    close(dev->tty_fd);
  }
}

static int rs485_device_init(const char* tty_dev, rs485_dev_t *dev) {
  int ret = 0;

  OBMC_INFO("Opening %s\n", tty_dev);
  strcpy(rackmon_io->dev_path,tty_dev);
  dev->tty_fd = rackmon_io->open(rackmon_io);
  if (dev->tty_fd < 0)
    return -1;

  ret = pthread_mutex_init(&dev->lock, NULL);
  if (ret != 0) {
    OBMC_ERROR(ret, "failed to initialize rs485 dev mutex");
    close(dev->tty_fd);  /* ignore errors */
    dev->tty_fd = -1;
    errno = ret;
    return -1;
  }

  return 0;
}

// This flag is triggered by SIGTERM and SIGINT signal handlers
// indicating that the program should finish up what it's doing and exit
// gracefully
volatile sig_atomic_t should_exit;

static void trigger_graceful_exit(int sig)
{
  should_exit = 1;
}

static int reset_psu_baudrate(void)
{
  int pos, ret = 0, err;
  uint16_t value;
  psu_datastore_t *mdata;

  int (*get_psu_location)(int, int*, int*, int*);
  int psu_baudrate_register;

  if (rackmond_config.rack_version==2) {
    get_psu_location=psu_location;
    psu_baudrate_register=REGISTER_PSU_BAUDRATE;
  } else if (rackmond_config.rack_version==3) {
    get_psu_location=psu_location_V3;
    psu_baudrate_register=REGISTER_PSU_BAUDRATE_V3;
  } else {
    OBMC_INFO("rackmon version error: current:%d, set by default version 2", rackmond_config.rack_version);
    get_psu_location=psu_location;
    psu_baudrate_register=REGISTER_PSU_BAUDRATE;
  }

  global_lock();
  rackmond_config.paused = 1;
  rackmond_config.min_delay = BAUDRATE_CMD_TIMEOUT;
  global_unlock();

  // set all connected PSUs back to the default baud rate
  OBMC_INFO("restoring PSUs to the default baud rate");
  for (pos = 0; pos < ARRAY_SIZE(rackmond_config.stored_data); pos++) {
    int rack=0,shelf=0,psu=0;
    mdata = rackmond_config.stored_data[pos];
    if (mdata == NULL) {
      continue;
    }
    if (mdata->baudrate <= 0) {
      continue;
    }
    if (get_psu_location(mdata->addr,&rack,&shelf,&psu)) {
      continue;
    }
    if (mdata->baudrate != DEFAULT_BAUDRATE) {
      value = baudrate_to_value(DEFAULT_BAUDRATE);
      
      if (rackmond_config.multi_port) {
        err = write_holding_reg(&rackmond_config.rs485[rack],
                                BAUDRATE_CMD_TIMEOUT, mdata->addr,
                                psu_baudrate_register, value,
                                NULL, mdata->baudrate);
      } else {
        err = write_holding_reg(&rackmond_config.rs485[0],
                                BAUDRATE_CMD_TIMEOUT, mdata->addr,
                                psu_baudrate_register, value,
                                NULL, mdata->baudrate);
      }
      if (err != 0) {
        OBMC_WARN("Unable to reset PSU %02x to the original baudrate",
                  mdata->addr);
        ret = 1;
      }
    }
  }

  return ret;
}

static void record_data(reg_range_data_t* rd, uint32_t time,
                        uint16_t* regs) 
{
  int n_regs = (rd->i->len);
  int pitch = REG_INT_DATA_SIZE(rd->i);
  int mem_size = pitch * rd->i->keep;

  memcpy(rd->mem_begin + rd->mem_pos, &time, sizeof(time));
  rd->mem_pos += sizeof(time);
  memcpy(rd->mem_begin + rd->mem_pos, regs, n_regs * sizeof(uint16_t));
  rd->mem_pos += n_regs * sizeof(uint16_t);
  rd->mem_pos = rd->mem_pos % mem_size;
}

static int reload_psu_registers(psu_datastore_t *mdata)
{
  int r, error;
  speed_t baudrate;
  uint8_t addr = mdata->addr;
  int rack=0,shelf=0,psu=0;
  
  int (*get_psu_location)(int, int*, int*, int*);
  int psu_baudrate_register;

  if (rackmond_config.rack_version==2) {
    get_psu_location=psu_location;
    psu_baudrate_register=REGISTER_PSU_BAUDRATE;
  } else if (rackmond_config.rack_version==3){
    get_psu_location=psu_location_V3;
    psu_baudrate_register=REGISTER_PSU_BAUDRATE_V3;
  } else {
    OBMC_INFO("rackmon version error: current:%d, set by default version 2", rackmond_config.rack_version);
    get_psu_location=psu_location;
    psu_baudrate_register=REGISTER_PSU_BAUDRATE;
  }

  if (get_psu_location(addr,&rack,&shelf,&psu)) {
    return -1;
  }

  if (global_lock() != 0) {
    return -1;
  }
  error = check_psu_baudrate(mdata, &baudrate);
  global_unlock();
  if (error != 0) {
    OBMC_WARN("Unable to check baudrate for PSU at addr %02x", addr);
    return -1;
  }

  CPU_USAGE_START();
  for (r = 0; r < rackmond_config.config->num_intervals; r++) {
    int err;
    uint32_t timestamp;
    reg_range_data_t* rd = &mdata->range_data[r];
    monitor_interval* iv = rd->i;
    uint16_t regs[iv->len];

    if (rackmond_config.multi_port) {
      err = read_holding_reg(&rackmond_config.rs485[rack],
                            rackmond_config.modbus_timeout, addr,
                            iv->begin, iv->len, regs, baudrate);
    } else {
      err = read_holding_reg(&rackmond_config.rs485[0],
                            rackmond_config.modbus_timeout, addr,
                            iv->begin, iv->len, regs, baudrate);
    }
    sched_yield();
    if (err != 0) {
      if (err != READ_ERROR_RESPONSE && err != PSU_TIMEOUT_RESPONSE) {
        log("Error %d reading %02x registers at %02x from %02x\n",
            err, iv->len, iv->begin, addr);
      }

      continue;
    }

    TIME_UPDATE(timestamp);
    if (iv->flags & MONITOR_FLAG_ONLY_CHANGES) {
      int pitch = REG_INT_DATA_SIZE(iv);
      int lastpos = rd->mem_pos - pitch;
      if (lastpos < 0) {
        lastpos = (pitch * iv->keep) - pitch;
      }
      if (!memcmp(rd->mem_begin + lastpos + sizeof(timestamp),
                  regs, sizeof(uint16_t) * iv->len) &&
           memcmp(rd->mem_begin, "\x00\x00\x00\x00", 4)) {
        continue;
      }

      if (rackmond_config.status_log) {
        time_t rawt;
        struct tm* ti;
        char timestr[80];

        time(&rawt);
        ti = localtime(&rawt);
        strftime(timestr, sizeof(timestr), "%b %e %T", ti);
        fprintf(rackmond_config.status_log,
                "%s: Change to status register %02x on address %02x. "
                "New value: %02x\n",
                timestr, iv->begin, addr, regs[0]);
        fflush(rackmond_config.status_log);
      }
    }

    global_lock();
    if (iv->begin == psu_baudrate_register) {
      uint16_t baudrate_value = regs[0] >> 8;
      mdata->supports_baudrate = (baudrate_value != 0);
      mdata->baudrate = BAUDRATE_VALUES[baudrate_value];
    }
    record_data(rd, timestamp, regs);
    global_unlock();
  } /* for each monitor_interval */
  CPU_USAGE_END("reloading psu %#x registers", addr);

  return 0;
}

static int fetch_monitored_data(void)
{
  int pos;
  psu_datastore_t *mdata;

  if (global_lock() != 0) {
    return -1;
  }
  if (rackmond_config.paused == 1 || rackmond_config.config == NULL) {
    global_unlock();
    return -1;
  }
  global_unlock();

  for (pos = 0; pos < ARRAY_SIZE(rackmond_config.stored_data); pos++) {
    if (should_exit)
      break;

    mdata = rackmond_config.stored_data[pos];
    if (mdata != NULL) {
      reload_psu_registers(mdata);

      if (!should_exit)
        sleep(PSU_ACCESS_NICE_DELAY);
    }
  } /* for each psu */

  return 0;
}

static int update_all_psu_timestamp(void)
{
  int pos;
  psu_datastore_t *mdata;
  int rc = 0;

  if (global_lock() != 0) {
    return -1;
  }
  if (rackmond_config.paused == 1 ||
      rackmond_config.config == NULL) {
    global_unlock();
    return -1;
  }
  global_unlock();

  for (pos = 0; pos < ARRAY_SIZE(rackmond_config.stored_data); pos++) {
    if (should_exit)
      break;
    mdata = rackmond_config.stored_data[pos];
    if (mdata != NULL) {
      rc |= set_psu_timestamp(mdata, 0);
    }
  }
  return rc;
}

static time_t search_at = 0;

void* monitoring_loop(void* arg)
{
  int ret;
  sigset_t sig_mask;
  time_t timestamp_at = 0;
  int last_num_psus = 0, num_psus = 0;

  /*
   * Block SIGINT and SIGTERM so these signals can be delivered to the
   * main thread for processing.
   */
  sigemptyset(&sig_mask);
  sigaddset(&sig_mask, SIGINT);
  sigaddset(&sig_mask, SIGTERM);
  ret = pthread_sigmask(SIG_BLOCK, &sig_mask, NULL);
  if (ret != 0) {
    OBMC_ERROR(ret, "failed to set signal mask in monitoring thread");
    return NULL;
  }

  rackmond_config.status_log = fopen(RACKMON_STAT_STORE, "a+");
  if (rackmond_config.status_log == NULL) {
    OBMC_ERROR(errno, "failed to open %s", RACKMON_STAT_STORE);
    /* XXX shall we exit the thread??? */
  }

  while (!should_exit) {
    long delay;
    struct timespec now;

    clock_gettime(CLOCK_REALTIME, &now);
    if (search_at <= now.tv_sec) {
      num_psus = check_active_psus();

      clock_gettime(CLOCK_REALTIME, &now);
      search_at = now.tv_sec + PSU_SCAN_INTERVAL;

      if (num_psus > 0) {
        alloc_monitoring_datas();

        sleep(PSU_ACCESS_NICE_DELAY);
        fetch_monitored_data();
      }
    }

    if (last_num_psus != num_psus || timestamp_at <= now.tv_sec) {
      OBMC_INFO("Updating timestamp\n");
      update_all_psu_timestamp();
      timestamp_at = now.tv_sec + PSU_TIMESTAMP_UPDATE_INTERVAL;
    }

    /*
     * TODO: ideally we should allow the main thread to wake up this
     * monitoring loop (for example, in case "force_rescan" command is
     * received).
     */
    delay = search_at - now.tv_sec;
    last_num_psus = num_psus;
    if (delay > PSU_ACCESS_NICE_DELAY)
      delay = PSU_ACCESS_NICE_DELAY;
    sleep(delay);
  } /* while (!should_exit) */

  OBMC_INFO("exiting rackmon monitoring thread");
  return NULL;
}

static int run_cmd_raw_modbus(rackmond_command* cmd, write_buf_t *wb)
{
  char *resp_buf;
  int timeout, resp_len, err, slot;
  uint16_t exp_resp_len, resp_len_wire, error_code;
  speed_t baudrate;
  psu_datastore_t *psu = NULL;

  if (cmd->raw_modbus.custom_timeout) {
    timeout = cmd->raw_modbus.custom_timeout * 1000; // ms to us
  } else {
    timeout = rackmond_config.modbus_timeout;
  }

  if (cmd->raw_modbus.length < 1) {
    OBMC_WARN("Raw modbus command too short!");
    return -1;
  }

  if (global_lock() != 0) {
    return -1;
  }

  slot = lookup_data_slot((uint8_t) cmd->raw_modbus.data[0]);
  if (slot >= 0) {
    psu = rackmond_config.stored_data[slot];
  }

  err = check_psu_baudrate(psu, &baudrate);

  global_unlock();

  if (err != 0) {
    error_code = -err;
    resp_len_wire = 0;
    buf_write(wb, &resp_len_wire, sizeof(uint16_t));
    buf_write(wb, &error_code, sizeof(uint16_t));
    return 0;
  }

  exp_resp_len = cmd->raw_modbus.expected_response_length;
  if (exp_resp_len == 0) {
    exp_resp_len = 1024;
  }

  resp_buf = malloc(exp_resp_len);
  if (resp_buf == NULL) {
    OBMC_WARN("failed to allocate resp_buf for raw_modbus command");
    return -1;
  }

  CHECK_LATENCY_START();
  if (rackmond_config.multi_port){
    resp_len = modbus_command(&rackmond_config.rs485[cmd->rack], timeout,
                              cmd->raw_modbus.data, cmd->raw_modbus.length,
                              resp_buf, exp_resp_len, exp_resp_len, baudrate, "raw_modbus");
  } else {
    resp_len = modbus_command(&rackmond_config.rs485[0], timeout,
                              cmd->raw_modbus.data, cmd->raw_modbus.length,
                              resp_buf, exp_resp_len, exp_resp_len, baudrate, "raw_modbus");
  }
  CHECK_LATENCY_END("rackmond::raw_modbus::modbus_command cmd_len=%lu, resp_len=%lu status=%d",
      (unsigned long)MODBUS_FUN3_REQ_HDR_SIZE, (unsigned long)exp_resp_len, resp_len);
  if(resp_len < 0) {
    error_code = -resp_len;
    resp_len_wire = 0;
    buf_write(wb, &resp_len_wire, sizeof(uint16_t));
    buf_write(wb, &error_code, sizeof(uint16_t));
  } else {
    resp_len_wire = resp_len;
    buf_write(wb, &resp_len_wire, sizeof(uint16_t));
    buf_write(wb, resp_buf, resp_len);
  }

  free(resp_buf);
  return 0;
}

static int run_cmd_set_config(rackmond_command* cmd, write_buf_t *wb)
{
  int error = 0;
  size_t config_size;

  if (global_lock() != 0) {
    return -1;
  }

  if (rackmond_config.config != NULL) {
    BAIL("rackmond already configured\n");
  }

  config_size = sizeof(monitoring_config) +
          (sizeof(monitor_interval) * cmd->set_config.config.num_intervals);
  rackmond_config.config = calloc(1, config_size);
  if (rackmond_config.config == NULL) {
    BAIL("failed to allocate config structure");
  }

  memcpy(rackmond_config.config, &cmd->set_config.config, config_size);
  OBMC_INFO("got configuration");
  TIME_UPDATE(search_at);

cleanup:
  global_unlock();
  return error;
}

static int run_cmd_dump_status(rackmond_command* cmd, write_buf_t *wb)
{
  if (global_lock() != 0) {
    return -1;
  }

  if (rackmond_config.config == NULL) {
    buf_printf(wb, "Unconfigured\n");
  } else {
    int i;
    time_t now;

    TIME_UPDATE(now);
    buf_printf(wb, "Monitored PSUs:\n");
    for (i = 0; i < ARRAY_SIZE(rackmond_config.stored_data); i++) {
      if (rackmond_config.stored_data[i] == NULL) {
        continue;
      }

      speed_t baudrate = rackmond_config.stored_data[i]->baudrate;
      buf_printf(wb, "PSU addr %02x - crc errors: %d, timeouts: %d, baud rate: %s",
                 rackmond_config.stored_data[i]->addr,
                 rackmond_config.stored_data[i]->crc_errors,
                 rackmond_config.stored_data[i]->timeout_errors,
                 baud_to_str(baudrate));
      if (rackmond_config.stored_data[i]->timeout_mode) {
        time_t until = rackmond_config.stored_data[i]->last_comms + NON_COMMUNICATION_TIMEOUT;
        buf_printf(wb, " (in timeout mode for the next %d seconds)", until - now);
      }
      buf_printf(wb, "\n");
    }

    buf_printf(wb, "Active on last scan: ");
    for (i = 0; i < rackmond_config.num_active_addrs; i++) {
      buf_printf(wb, "%02x ", rackmond_config.active_addrs[i]);
    }
    buf_printf(wb, "\n");
    buf_printf(wb, "Next scan in %d seconds.\n", search_at - now);
  }

  global_unlock();
  return 0;
}

static int run_cmd_force_scan(rackmond_command* cmd, write_buf_t *wb)
{
  if (global_lock() != 0) {
    return -1;
  }

  if (rackmond_config.config == NULL) {
    buf_printf(wb, "Unconfigured\n");
  } else {
    TIME_UPDATE(search_at);
    buf_printf(wb, "Triggering PSU scan...\n");
  }

  global_unlock();
  return 0;
}

static int run_cmd_dump_json(rackmond_command* cmd, write_buf_t *wb)
{
  if (global_lock() != 0) {
    return -1;
  }

  if (rackmond_config.config == NULL) {
    buf_write(wb, "[]", 2);
  } else {
    int i, j, c;
    uint32_t now;
    int data_pos = 0;

    TIME_UPDATE(now);
    buf_write(wb, "[", 1);
    while (rackmond_config.stored_data[data_pos] != NULL &&
           data_pos < MAX_ACTIVE_ADDRS) {
      psu_datastore_t *pdata = rackmond_config.stored_data[data_pos];

      buf_printf(wb, "{\"addr\":%d,\"crc_fails\":%d,\"timeouts\":%d,"
                 "\"now\":%d,\"ranges\":[",
                 pdata->addr, pdata->crc_errors, pdata->timeout_errors, now);

      for (i = 0; i < rackmond_config.config->num_intervals; i++) {
        uint32_t time;
        reg_range_data_t *rd = &pdata->range_data[i];
        char* mem_pos = rd->mem_begin;

        buf_printf(wb,"{\"begin\":%d,\"readings\":[", rd->i->begin);
        // want to cut the list off early just before
        // the first entry with time == 0
        memcpy(&time, mem_pos, sizeof(time));
        for(j = 0; j < rd->i->keep && time != 0; j++) {
          mem_pos += sizeof(time);
          buf_printf(wb, "{\"time\":%d,\"data\":\"", time);
          for(c = 0; c < rd->i->len * 2; c++) {
            buf_printf(wb, "%02x", *mem_pos);
            mem_pos++;
          }
          buf_write(wb, "\"}", 2);
          memcpy(&time, mem_pos, sizeof(time));
          if (time == 0) {
            break;
          }
          if ((j+1) < rd->i->keep) {
            buf_write(wb, ",", 1);
          }
        }
        buf_write(wb, "]}", 2);
        if ((i+1) < rackmond_config.config->num_intervals) {
          buf_write(wb, ",", 1);
        }
      }

      data_pos++;
      if (data_pos < MAX_ACTIVE_ADDRS &&
          rackmond_config.stored_data[data_pos] != NULL) {
        buf_write(wb, "]},", 3);
      } else {
        buf_write(wb, "]}", 2);
      }
    }
    buf_write(wb, "]", 1);
  }

  global_unlock();
  return 0;
}

static int run_cmd_dump_info(rackmond_command* cmd, write_buf_t *wb)
{
  int data_pos = 0;
  if (global_lock() != 0) {
    return -1;
  }

  if (rackmond_config.config == NULL) {
    buf_write(wb, "Unconfigured", 12);
  } else {
    while ((data_pos < MAX_ACTIVE_ADDRS) &&
           (rackmond_config.stored_data[data_pos] != NULL)) {
        rackmon_print_info(wb,
          rackmond_config.stored_data[data_pos],
          rackmond_config.config->num_intervals);
        data_pos++;
      }
  }
  global_unlock();
  return 0;
}

static int run_cmd_pause_monitoring(rackmond_command* cmd, write_buf_t *wb)
{
  uint8_t was_paused;

  if (global_lock() != 0) {
    return -1;
  }

  was_paused = rackmond_config.paused;
  rackmond_config.paused = 1;
  buf_write(wb, &was_paused, sizeof(was_paused));

  global_unlock();
  return 0;
}

static int run_cmd_start_monitoring(rackmond_command* cmd, write_buf_t *wb)
{
  uint8_t was_started;

  if (global_lock() != 0) {
    return -1;
  }

  was_started = !rackmond_config.paused;
  rackmond_config.paused = 0;
  buf_write(wb, &was_started, sizeof(was_started));

  global_unlock();
  return 0;
}

static struct {
  const char *name;
  int (*handler)(rackmond_command* cmd, write_buf_t *wb);
} rackmond_cmds[COMMAND_TYPE_MAX] = {
  [COMMAND_TYPE_RAW_MODBUS] = {
    .name = "raw_modbus",
    .handler = run_cmd_raw_modbus,
  },
  [COMMAND_TYPE_SET_CONFIG] = {
    .name = "set_config",
    .handler = run_cmd_set_config,
  },
  [COMMAND_TYPE_DUMP_DATA_JSON] = {
    .name = "dump_data_json",
    .handler = run_cmd_dump_json,
  },
  [COMMAND_TYPE_DUMP_DATA_INFO] = {
    .name = "dump_data_info",
    .handler = run_cmd_dump_info,
  },
  [COMMAND_TYPE_PAUSE_MONITORING] = {
    .name = "pause_monitoring",
    .handler = run_cmd_pause_monitoring,
  },
  [COMMAND_TYPE_START_MONITORING] = {
    .name = "start_monitoring",
    .handler = run_cmd_start_monitoring,
  },
  [COMMAND_TYPE_DUMP_STATUS] = {
    .name = "dump_status",
    .handler = run_cmd_dump_status,
  },
  [COMMAND_TYPE_FORCE_SCAN] = {
    .name = "force_scan",
    .handler = run_cmd_force_scan,
  },
};

static int do_command(int sock, rackmond_command* cmd)
{
  int error = 0;
  write_buf_t wb;
  uint16_t type = cmd->type;

  //128k write buffer
  if (buf_open(&wb, sock, 128*1000) != 0) {
    return -1;
  }

  if (type <= COMMAND_TYPE_NONE || type >= COMMAND_TYPE_MAX ||
      rackmond_cmds[type].name == NULL) {
    BAIL("Unknown command type %u\n", type);
  }
  OBMC_INFO("processing command %s\n", rackmond_cmds[type].name);

  assert(rackmond_cmds[type].handler != NULL);
  CHECK_LATENCY_START();
  error = rackmond_cmds[type].handler(cmd, &wb);
  CHECK_LATENCY_END("rackmond::do_command::%s status=%d", rackmond_cmds[type].name, error);
  if (error != 0) {
    OBMC_WARN("error while processing command %s", rackmond_cmds[type].name);
  } else {
    OBMC_INFO("command %s was handled properly", rackmond_cmds[type].name);
  }

cleanup:
  buf_close(&wb);
  return error;
}

// receive the command as a length prefixed block
// (uint16_t, followed by data)
// this is all over a local socket, won't be doing
// endian flipping, clients should only be local procs
// compiled for the same arch

static int recv_sock_message(int sock, void *buf, size_t size)
{
  int ret = 0;
  struct pollfd pfd = {
    .fd = sock,
    .events = POLLIN | POLLERR | POLLHUP,
  };

  do {
    ret = poll(&pfd, 1, 1000);
    if (ret < 0) {
      return -1;
    } else if (ret == 0) {
      OBMC_INFO("poll socket (%d) timed out", sock);
      continue;
    } else if (pfd.revents & (POLLERR | POLLHUP)) {
      errno = EIO;
      return -1;
    }

    ret = recv(sock, buf, size, MSG_DONTWAIT);
  } while (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK));

  return ret;
}

static int handle_connection(int sock)
{
  int ret;
  int error = 0;
  char body_buf[1024];
  uint16_t body_len = 0;

  OBMC_INFO("new connection established, socket=%d\n", sock);

  ret = recv_sock_message(sock, &body_len, sizeof(body_len));
  if (ret < 0) {
    OBMC_ERROR(errno, "failed to recv message header from socket %d", sock);
    return -1;
  } else if (ret < sizeof(body_len)) {
    OBMC_WARN("message header short read, expect %u, actual %u",
              sizeof(body_len), ret);
    return -1;
  } else if (body_len > sizeof(body_buf)) {
    OBMC_WARN("message body exceeds max size (%u > %u)",
              body_len, sizeof(body_buf));
    return -1;
  }

  ret = recv_sock_message(sock, body_buf, body_len);
  if (ret < 0) {
    OBMC_ERROR(errno, "failed to recv message body from socket %d", sock);
    return -1;
  }

  error = do_command(sock, (rackmond_command*)body_buf);

  return error;
}

static int signal_handler_init(void)
{
  struct sigaction ign_action, exit_action;

  ign_action.sa_flags = 0;
  sigemptyset(&ign_action.sa_mask);
  ign_action.sa_handler = SIG_IGN;
  if (sigaction(SIGPIPE, &ign_action, NULL) != 0) {
    OBMC_ERROR(errno, "failed to set SIGPIPE handler");
    return -1;
  }

  exit_action.sa_flags = 0;
  sigemptyset(&exit_action.sa_mask);
  exit_action.sa_handler = trigger_graceful_exit;
  if (sigaction(SIGTERM, &exit_action, NULL) != 0) {
    OBMC_ERROR(errno, "failed to set SIGTERM handler");
    return -1;
  }
  if (sigaction(SIGINT, &exit_action, NULL) != 0) {
    OBMC_ERROR(errno, "failed to set SIGTERM handler");
    return -1;
  }

  return 0;
}

static int user_socket_init(const char *sock_path)
{
  int sock_len;
  int sock = -1;
  struct sockaddr_un local;

  sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0) {
    OBMC_ERROR(errno, "failed to create user-request socket");
    return -1;
  }

  unlink(sock_path); /* ignore failures */
  local.sun_family = AF_UNIX;
  strcpy(local.sun_path, sock_path);
  sock_len = sizeof(local.sun_family) + strlen(local.sun_path);
  if (bind(sock, (struct sockaddr *)&local, sock_len) != 0) {
    OBMC_ERROR(errno, "failed to bind socket to %s", sock_path);
    close(sock);
    return -1;
  }

  if (listen(sock, 20) != 0) {
    OBMC_ERROR(errno, "failed to set passive socket");
    close(sock);
    return -1;
  }

  return sock;
}

int main(int argc, char** argv)
{
  int error = 0;
  int sock = -1;
  pthread_t monitoring_tid;
  sigset_t new_mask, old_mask;
  struct sockaddr_un client;

  if (single_instance_lock(DAEMON_NAME) < 0) {
    fprintf(stderr, "Another %s instance is running. Exiting!\n",
            DAEMON_NAME);
    return -1;
  }

  obmc_log_init(DAEMON_NAME, LOG_INFO, 0);
  obmc_log_set_syslog(LOG_CONS, LOG_DAEMON);
  if (getenv("RACKMOND_FOREGROUND") == NULL) {
    obmc_log_unset_std_stream();
    if (daemon(0, 0) < 0)
      OBMC_ERROR(errno, "daemon error");
  }

#ifdef RACKMON_PROFILING
  OBMC_INFO("ticks_per_sec: %ld", sysconf(_SC_CLK_TCK));
#endif /* RACKMON_PROFILING */

  if (signal_handler_init() != 0)
    return -1;

  if (getenv("RACKMOND_RACK_VERSION") !=NULL) {
    rackmond_config.rack_version = atoll(getenv("RACKMOND_RACK_VERSION"));
    OBMC_INFO("set RACKMON_RACK_VERSION to %d",
              rackmond_config.rack_version);
  } else {
    rackmond_config.rack_version = 2;
    OBMC_WARN("rackmon version error: no version, set to default:%d", rackmond_config.rack_version);
  }

  if (getenv("RACKMOND_TIMEOUT") != NULL) {
    rackmond_config.modbus_timeout = atoll(getenv("RACKMOND_TIMEOUT"));
    OBMC_INFO("set timeout to RACKMOND_TIMEOUT (%dms)",
              rackmond_config.modbus_timeout / 1000);
  }
  if (getenv("RACKMOND_MIN_DELAY") != NULL) {
    rackmond_config.min_delay = atoll(getenv("RACKMOND_MIN_DELAY"));
    OBMC_INFO("set mindelay to RACKMOND_MIN_DELAY(%dus)",
              rackmond_config.min_delay);
  }
  verbose = getenv("RACKMOND_VERBOSE") != NULL ? 1 : 0;

  if (getenv("RACKMOND_MULTI_PORT") != NULL) {
    rackmond_config.multi_port = atoll(getenv("RACKMOND_MULTI_PORT"));
    OBMC_INFO("set RACKMOND_MULTI_PORT to %d, for FT4232",
                rackmond_config.multi_port);
  } else {
    rackmond_config.multi_port = 0;
  }

  if (getenv("RACKMOND_SWAP_ADDR") != NULL) {
    rackmond_config.swap_addr = atoll(getenv("RACKMOND_SWAP_ADDR"));
    OBMC_INFO("set RACKMOND_SWAP_ADDR to %d, for W400 MP Respin",
                rackmond_config.swap_addr);
  } else {
    rackmond_config.swap_addr = 0;
  }

  if (getenv("RACKMOND_DESIRED_BAUDRATE") != NULL) {
    int parsed_baudrate_int = atoi(getenv("RACKMOND_DESIRED_BAUDRATE"));
    rackmond_config.desired_baudrate = int_to_baudrate(parsed_baudrate_int);
    OBMC_INFO("set desired baudrate value to RACKMOND_DESIRED_BAUDRATE (%d -> %s)",
              parsed_baudrate_int,
              baud_to_str(rackmond_config.desired_baudrate));
  }
  if (getenv("RACKMOND_IGNORE_PSUS") != NULL) {
    char *ignore_list = strdup(getenv("RACKMOND_IGNORE_PSUS"));
    if (ignore_list) {
      char *psu_str;
      for (psu_str = strtok(ignore_list, ",");
          psu_str != NULL;
          psu_str = strtok(NULL, ",")) {
        unsigned int psu_addr = strtoul(psu_str, 0, 16);
        int rack, shelf, psu;
        int (*get_psu_location)(int, int*, int*, int*);
        if (rackmond_config.rack_version==2) {
          get_psu_location=psu_location;
        } else if (rackmond_config.rack_version==3) {
          get_psu_location=psu_location_V3;
        } else {
          OBMC_INFO("rackmon version error: current:%d, set by default version 2", rackmond_config.rack_version);
          get_psu_location=psu_location;
        }
        if (!get_psu_location(psu_addr, &rack, &shelf, &psu)) {
          rackmond_config.ignored_psus[rack][shelf][psu] = true;
          OBMC_INFO("Ignoring PSU: 0x%x\n", psu_addr);
        } else {
          OBMC_ERROR(EINVAL, "Unsupported PSU: 0x%x provided to RACKMOND_IGNORE_PSUS\n", psu_addr);
        }
      }
      free(ignore_list);
    }
  }

  OBMC_INFO("rackmon/modbus service starting");
  if (rackmon_plat_init() != 0) {
    return -1;
  }

  if (rackmond_config.multi_port) {
    if (rs485_device_init(DEFAULT_TTY1, &rackmond_config.rs485[0]) != 0) {
      error = -1;
      goto exit_rs485;
    }
    if (rs485_device_init(DEFAULT_TTY2, &rackmond_config.rs485[1]) != 0) {
      error = -1;
      goto exit_rs485;
    }
    if (rs485_device_init(DEFAULT_TTY3, &rackmond_config.rs485[2]) != 0) {
      error = -1;
      goto exit_rs485;
    }
  } else {
    if (rs485_device_init(DEFAULT_TTY, &rackmond_config.rs485[0]) != 0) {
      error = -1;
      goto exit_rs485;
    }
  }

  error = pthread_create(&monitoring_tid, NULL, monitoring_loop, NULL);
  if (error != 0) {
    OBMC_ERROR(error, "failed to create monitor loop thread");
    error = -1;
    goto exit_thread;
  }

  sock = user_socket_init(RACKMON_IPC_SOCKET);
  if (sock < 0) {
    goto exit_sock;
  }
  OBMC_INFO("rackmon is listening to user connections");

  /*
   * SIGINT and SIGTERM are blocked before testing "should_exit", and
   * they will be unblocked in pselect(): this is to prevent the race
   * when the signals were delivered right after testing "should_exit"
   * but before calling pselect().
   */
  sigemptyset(&new_mask);
  sigaddset(&new_mask, SIGINT);
  sigaddset(&new_mask, SIGTERM);
  sigprocmask(SIG_BLOCK, &new_mask, &old_mask);

  while (!should_exit) {
    int ret;
    fd_set rfds;
    socklen_t clisocklen = sizeof(struct sockaddr_un);

    FD_ZERO(&rfds);
    FD_SET(sock, &rfds);
    ret = pselect(sock + 1, &rfds, NULL, NULL, NULL, &old_mask);
    if (ret > 0 && FD_ISSET(sock, &rfds)) {
        int clisock = accept(sock, (struct sockaddr*)&client, &clisocklen);
        if (clisock < 0) {
          OBMC_ERROR(errno, "failed to accept new connection");
          continue;
        }

        handle_connection(clisock);
        close(clisock);
    }
  }

  close(sock); /* ignore errors */
  if (reset_psu_baudrate() != 0)
    error = -1;
exit_sock:
  pthread_cancel(monitoring_tid);     /* ignore errors */
  pthread_join(monitoring_tid, NULL); /* ignore errors */
exit_thread:
  if (rackmond_config.multi_port) {
    rs485_device_cleanup(&rackmond_config.rs485[0]);
    rs485_device_cleanup(&rackmond_config.rs485[1]);
    rs485_device_cleanup(&rackmond_config.rs485[2]);
  } else {
    rs485_device_cleanup(&rackmond_config.rs485[0]);
  }
exit_rs485:
  rackmon_plat_cleanup();
  OBMC_INFO("rackmon is terminated, exit code: %d", error);
  return error;
}
