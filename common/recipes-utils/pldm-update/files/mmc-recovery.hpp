#pragma once

#include <fcntl.h>
#include <termios.h>
#include <sys/file.h>
#include <unistd.h> 
#include <string>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <filesystem>

constexpr uint32_t MAGIC_HEADER = 0x74547053;
constexpr uint8_t UART_HEADER_MAX = 0x40;
constexpr uint16_t UART_SEND_DATA_MAX = 0x400;
constexpr uint16_t UART_RECEIVE_DATA_MAX = 0x400;
constexpr uint16_t UART_RECEIVE_BUFFER_SIZE = UART_HEADER_MAX + UART_RECEIVE_DATA_MAX;

constexpr uint32_t UART_LOADER_ADDRESS = 0x10088000;
constexpr uint32_t UART_LOADER_ENTRY_OFFSET = 0x21C;

constexpr uint8_t PACKAGE_CMD_LENGTH = 5;  // cmd + addr
constexpr uint8_t DATA_CMD_LENGTH = 2;     // data length
constexpr uint8_t CRC8_LENGTH = 1;
constexpr uint8_t CRC8_START_OFFSET = 6;   // start from cmd column
constexpr uint8_t MIN_PACKAGE_LENGTH = CRC8_START_OFFSET + PACKAGE_CMD_LENGTH + CRC8_LENGTH;

constexpr uint8_t PACKAGE_RESP_LENGTH = 2;  // cmd + status
constexpr uint8_t DATA_RESP_LENGTH = 2;     // data length
constexpr uint8_t MIN_RESP_PACKAGE_LENGTH = CRC8_START_OFFSET + PACKAGE_RESP_LENGTH + CRC8_LENGTH;
constexpr const char* DEFAULT_SERIAL_PORT = "/dev/ttyUSB6";

extern std::string board_num;
extern std::string board;

enum npcm_uart_cmd
{
	UART_CMD_START = 0x0,
	UART_CMD_WRITE_MEMORY = 0x1,
	UART_CMD_READ_MEMORY = 0x2,
	UART_CMD_GET_DIGEST = 0x3,
	UART_CMD_EXE_ADR = 0x4,

	UART_CMD_FLASH_SELECT = 0x20,
	UART_CMD_FLASH_READ_CAPACITY = 0x21,
	UART_CMD_FLASH_WRITE = 0x22,
	UART_CMD_FLASH_WRITE_FINISH = 0x23,
	UART_CMD_FLASH_READ = 0x24,
	UART_CMD_FLASH_ERASE = 0x25,

	UART_CMD_LOADER_REBOOT = 0x70,
	UART_CMD_LOADER_ALIVE = 0x7E,
	UART_CMD_MAX_SUPPORT = 0x7F,
};

enum npcm_uart_resp_status
{
	UART_RESP_STATUS_NO_ERR,
	UART_RESP_STATUS_HDR_ERR,
	UART_RESP_STATUS_INVALID_CMD,
	UART_RESP_STATUS_INVALID_ADR,
	UART_RESP_STATUS_INVALID_LEN,
	UART_RESP_STATUS_CRC_FAIL,
	UART_RESP_STATUS_REJECT,
	UART_RESP_STATUS_TIMEOUT,
	UART_RESP_STATUS_READ_ERR,
	UART_RESP_STATUS_VIRIFY_ERR,
};

struct SerialSel {
    std::string uart_path;
    int bus = -1;
};

class RecoveryOps {
public:
    virtual ~RecoveryOps();
    virtual SerialSel resolve_serial(const std::string& board_num,
                                     const std::string& board) = 0;

    virtual bool set_recovery_mode(bool state, int bus);
};
std::unique_ptr<RecoveryOps> create_recovery_ops();

class SerialDevice
{
  public:
    explicit SerialDevice(const std::string& serialPath): 
      serialPath(serialPath) {}

    void open_port()
    {
        struct termios options;

        serialFd = open(serialPath.c_str(), O_RDWR | O_NONBLOCK);
        if (serialFd < 0)
        {
            std::cerr << "Failed to open serial port: "
                      << strerror(errno) << std::endl;
            return;
        }

        if (tcgetattr(serialFd, &options) != 0)
        {
            std::cerr << "Failed to get serial attributes: "
                      << strerror(errno) << std::endl;
            close(serialFd);
            serialFd = -1;
            return;
        }

        /* Setting the baudrate */
        cfsetispeed(&options, baudRate);
        cfsetospeed(&options, baudRate);

        /* enable the receiver and set local mode */
        options.c_cflag |= (CLOCAL|CREAD);

        /* setting the character size (8-bits) */
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS8;

        /* Setting Parity Checking: NONE (NO Parity) */
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSTOPB;

        /* Disable Hardware flow control */
        options.c_cflag &= ~CRTSCTS;

        /* Disable Input Parity Checking */
        options.c_iflag &= ~INPCK;

        /* Disable software flow control */
        options.c_iflag &= ~(IXON|IXOFF|IXANY);
        options.c_iflag &= ~(IGNPAR|ICRNL);

        /* output raw data */
        options.c_oflag &= ~OPOST;

        /* disablestd input */
        options.c_lflag &= ~(ICANON|ECHO|ECHOE|ISIG);

        /* clean the current setting */
        tcflush(serialFd, TCIFLUSH);

        /* Enable the new setting right now */
        tcsetattr(serialFd, TCSANOW, &options);
        if (tcsetattr(serialFd, TCSANOW, &options) != 0)
        {
			std::cerr << "Failed to set serial attributes: " 
					  << strerror(errno) << std::endl;
			close(serialFd);
			serialFd = -1;
        }
    }

    void close_port()
    {
        if (serialFd != -1)
        {
            close(serialFd);
            serialFd = -1;
        }
    }

    ~SerialDevice()
    {
        close_port();
    }

    ssize_t write_data(const char *data, size_t size)
    {
        return write(serialFd, data, size);
    }

    ssize_t read_data(char* rbuf, size_t rlen)
    {
        return read(serialFd, rbuf, rlen);
    }

    int get_fd() const
    {
      	return serialFd;
    }

    std::string get_path() const
    {
        return serialPath;
    }

  private:
    int serialFd = -1;
    const std::string serialPath;
    const speed_t baudRate = B115200;
};

struct RemovablePath
{
    std::filesystem::path path;

    explicit RemovablePath(const std::filesystem::path& path) : path(path) {}
    ~RemovablePath()
    {
        if (!path.empty())
        {
            std::error_code ec;
            std::filesystem::remove_all(path, ec);
        }
    }

    RemovablePath(const RemovablePath& other) = delete;
    RemovablePath& operator=(const RemovablePath& other) = delete;
    RemovablePath(RemovablePath&&) = delete;
    RemovablePath& operator=(RemovablePath&&) = delete;
};

void recover_mmc(RecoveryOps *ops, const std::string& tarFile, 
        const std::string& board_num, std::string board);