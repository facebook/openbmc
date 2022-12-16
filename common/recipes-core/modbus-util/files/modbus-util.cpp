// Copyright (c) Meta Platforms, Inc. and affiliates.

#include <CLI/CLI.hpp>
#include <openbmc/misc-utils.h>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <cerrno>
#include <cassert>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <unordered_map>

#define GREEN "\u001b[0;32m"
#define YELLOW "\u001b[0;33m"
#define RESET "\u001b[0;m"

template <typename T>
struct Hex {
  T value;
};

template <typename T>
static std::istringstream &operator>>(std::istringstream& in, Hex<T>& hex) {
  std::string s;
  in >> s;

  char* end = nullptr;
  auto v = strtoul(s.data(), &end, 16);
  if (v > std::numeric_limits<T>::max() || end != s.data() + s.size()) {
    throw CLI::ConversionError("Invalid byte literal: " + s);
  }

  hex.value = v;
  return in;
}

enum class Parity {
  Even,
  Odd,
  None,
};

static const std::unordered_map<std::string, Parity> PARITY_STRING_MAP = {
  {"even", Parity::Even},
  {"odd", Parity::Odd},
  {"none", Parity::None},
};

struct SerialOptions {
  uint32_t baudrate;
  Parity parity;
  float rx_timeout_secs;
  bool rx_parity_check;
};

static int configure_serial_device(int fd, const SerialOptions &options) {
  struct termios term = {};

  cfsetspeed(&term, options.baudrate);

  switch (options.parity) {
  case Parity::Even:
    term.c_cflag |= PARENB;
    break;
  case Parity::Odd:
    term.c_cflag |= PARENB;
    term.c_cflag |= PARODD;
    break;
  case Parity::None:
    break;
  }
  if (options.rx_parity_check) {
    term.c_iflag |= INPCK;
  }
  term.c_cflag |= CLOCAL;
  term.c_cflag |= CS8;
  term.c_cflag |= CREAD;
  term.c_cc[VMIN] = 0;
  term.c_cc[VTIME] = options.rx_timeout_secs * 10.0f;

  if (tcsetattr(fd, TCSAFLUSH, &term)) {
    perror("tcsetattr");
    return -1;
  }
  return 0;
}

static int modbus_send(int fd, const uint8_t *buf, ssize_t len) {
  printf(YELLOW "%s" RESET "...", __func__);
  fflush(stdout);

  for (ssize_t i = 0, n = 0; n < len; i += n) {
    n = write(fd, &buf[i], len - i);
    if (n < 0) {
      printf("write: %s\n", strerror(errno));
      return -1;
    }
    if (n == 0) {
      printf(YELLOW "write blocked...");
      fflush(stdout);
      continue;
    }
    printf(GREEN);
    for (ssize_t j = 0; j < n; j++) {
      if (j) {
        printf(" ");
      }
      printf("%02x", buf[i + j]);
    }
    printf(RESET "...");
    fflush(stdout);
  }

  tcdrain(fd);
  printf("done\n");

  return 0;
}

static ssize_t modbus_recv(int fd, uint8_t *buf, ssize_t capacity) {
  printf(YELLOW "%s" RESET "...", __func__);
  fflush(stdout);

  ssize_t len = 0;
  while (len < capacity) {
    ssize_t n = read(fd, &buf[len], capacity - len);
    if (n < 0) {
      printf("read: %s\n", strerror(errno));
      return -1;
    }
    if (n == 0) {
      break;
    }
    printf(GREEN);
    for (ssize_t i = 0; i < n; i++) {
      if (i) {
        printf(" ");
      }
      printf("%02x", buf[len + i]);
    }
    printf(RESET "...");
    fflush(stdout);
    len += n;
  }

  if (len == 0) {
    printf("timeout\n");
    return -1;
  }
  printf("done\n");
  return 0;
}

int main(int argc, char** argv) {
  std::string tty;
  std::vector<Hex<uint8_t>> hex_bytes;
  SerialOptions tty_options = {};
  tty_options.baudrate = 19200;
  tty_options.parity = Parity::Even;
  tty_options.rx_timeout_secs = 0.5f;
  tty_options.rx_parity_check = true;

  auto app = CLI::App(GREEN "modbus-util" RESET);
  app.add_option(
      "TTY", tty, "Serial device to transmit on, e.g. /dev/ttyUSB1")
      ->required();
  app.add_option(
          "BYTES",
          hex_bytes,
          "Bytes to send, as 2-digit hex literals, e.g. 0a 1b 2c 3d")
      ->required();
  app.add_option("--baudrate", tty_options.baudrate, "TTY baudrate", true);
  app.add_option("--parity", tty_options.parity, "TTY parity config", true)
    ->transform(CLI::CheckedTransformer(PARITY_STRING_MAP, CLI::ignore_case));
  app.add_option("--rx-timeout-secs", tty_options.rx_timeout_secs, "TTY rx timeout (seconds)", true);
  app.add_option("--rx-parity-check", tty_options.rx_parity_check, "TTY rx parity check", true);

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    return app.exit(e);
  }

  int fd = open(tty.c_str(), O_RDWR | O_NOCTTY | O_CLOEXEC);
  if (fd == -1) {
    printf("Unable to open '%s': %s\n", tty.c_str(), strerror(errno));
    return 1;
  }

  if (configure_serial_device(fd, tty_options)) {
    printf("Error configuring serial device '%s'\n", tty.c_str());
    return 1;
  }

  auto start = &hex_bytes[0].value;
  auto end = start + hex_bytes.size();
  auto bytes = std::vector<uint8_t>(start, end);
  auto crc = crc16(bytes.data(), bytes.size());
  bytes.push_back(crc >> 8);
  bytes.push_back(crc & 0xFF);

  if (modbus_send(fd, bytes.data(), bytes.size())) {
    printf("Error sending command\n");
    return 1;
  }

  uint8_t buf[32];
  if (modbus_recv(fd, buf, sizeof(buf))) {
    printf("Error receiving response\n");
    return 1;
  }

  return 0;
}
