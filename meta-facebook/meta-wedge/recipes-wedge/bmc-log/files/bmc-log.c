/*
 * Copyright 2014-present Facebook. All Rights Reserved.
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
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <linux/serial.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/param.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <pty.h>
#include <errno.h>
#include <signal.h>
#include <netinet/in.h>
#include <netdb.h>
#include "bmc-log.h"

FILE *error_file = NULL;

bool kill_received = false;	// To check if a killing interrupt is received

speed_t baud_rate = B57600;	// Default baud rate - change if user inputs a different one

int fd_tty = -1, fd_soc = -1;

/* Hostname and port of the server */
char *hostname;
int port;

struct termios orig_tty_state;

char *get_time()
{
	static char mytime[TIME_FORMAT_SIZE];
	memset(mytime, 0, sizeof(mytime));
	time_t this_time;
	struct tm *this_tm;
	this_time = time(NULL);
	this_tm = localtime(&this_time);

	snprintf(mytime, sizeof(mytime), "%04d-%02d-%02d %02d:%02d:%02d",
	                                 1900 + this_tm->tm_year, this_tm->tm_mon + 1,
	                                 this_tm->tm_mday, this_tm->tm_hour,
	                                 this_tm->tm_min, this_tm->tm_sec);
	return mytime;
}

void errlog(char *frmt, ...)
{
	va_list args;
	va_start(args, frmt);
	struct stat st;

	char *time_now = get_time();

	fprintf(stderr, "[%s] ", time_now);
	vfprintf(stderr, frmt, args);

	if (error_file) {
		stat(error_log_file, &st);
		if (st.st_size >= MAX_LOG_FILE_SIZE) {
			truncate(error_log_file, 0);
		}
		fprintf(error_file, "[%s] ", time_now);
		vfprintf(error_file, frmt, args);
		fflush(error_file);
	}
}

/* Get the address info of netcons server */
struct addrinfo *get_addr_info(int ip_version)
{
	int ip_family = (ip_version == IPV4) ? AF_INET : AF_INET6;
	struct addrinfo hints;

	struct addrinfo *result;
	result = malloc(sizeof(*result));
	if (!result) {
		errlog("Error: Unable to allocate memory - %m\n");
		return NULL;
	}

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = ip_family;	/* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_DGRAM;	/* Datagram socket */
	hints.ai_flags = AI_PASSIVE;	/* For wildcard IP address */
	hints.ai_protocol = 0;	/* Any protocol */
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	if (getaddrinfo(hostname, NULL, &hints, &result)) {
		errlog("Error: getaddrinfo failed - %m\n");
		return NULL;
	}

	return result;
}

/* Prepare Ipv4 Socket */
bool prepare_sock(struct sockaddr_in * tgt_addr)
{
	struct addrinfo *addr_info;
	memset(tgt_addr, 0, sizeof(*tgt_addr));

	if ((addr_info = get_addr_info(IPV4)) == NULL) {
		errlog("Error: Unable to get address info\n");
		return false;
	}

	tgt_addr->sin_addr = ((struct sockaddr_in *)addr_info->ai_addr)->sin_addr;
	tgt_addr->sin_port = htons(port);
	tgt_addr->sin_family = AF_INET;

	return true;
}

/* Prepare Ipv6 Socket */
bool prepare_sock6(struct sockaddr_in6 * tgt_addr6)
{
	struct addrinfo *addr_info;
	memset(tgt_addr6, 0, sizeof(*tgt_addr6));

	if ((addr_info = get_addr_info(IPV6)) == NULL) {
		errlog("Erorr: Unable to get address info\n");
		return false;
	}

	tgt_addr6->sin6_addr = ((struct sockaddr_in6 *)addr_info->ai_addr)->sin6_addr;
	tgt_addr6->sin6_port = htons(port);
	tgt_addr6->sin6_family = AF_INET6;

	return true;
}

/* Set TTY to raw mode */
bool set_tty(int fd)
{
	struct termios tty_state;

	if (tcgetattr(fd, &tty_state) < 0) {
		return false;
	}

	if (tcgetattr(fd, &orig_tty_state) < 0)	// Save original settings
	{
		return false;
	}

	if (cfsetspeed(&tty_state, baud_rate) == -1) {
		errlog("Error: Baud Rate not set - %m\n");
		return false;
	}

	tty_state.c_lflag &= ~(ICANON | IEXTEN | ISIG | ECHO);
	tty_state.c_iflag &= ~(IGNCR | ICRNL | INPCK | ISTRIP | IXON | BRKINT);
	tty_state.c_iflag |= (IGNCR | ICRNL);
	tty_state.c_oflag &= ~OPOST;
	tty_state.c_cflag |= CS8;

	tty_state.c_cc[VMIN] = 1;
	tty_state.c_cc[VTIME] = 0;

	if (tcsetattr(fd, TCSAFLUSH, &tty_state) < 0) {
		return false;
	}

	return true;
}

/* Create a pseudo terminal for other process to use (as this program is using up the actual TTY) */
int create_pseudo_tty()
{
	int amaster, aslave;
	int flags;

	if (openpty(&amaster, &aslave, NULL, NULL, NULL) == -1) {
		errlog("Error: Openpty failed - %m\n");
		return -1;
	}

	/* Set to non blocking mode */
	flags = fcntl(amaster, F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(amaster, F_SETFL, flags);

	FILE *pseudo_save_file = fopen(pseudo_tty_save_file, "w+");
	if (!pseudo_save_file) {
		errlog("Error: Unable to open the pseudo info file - %m\n");
		return -1;
	}
	/* Save the name of the created pseudo tty in a text file for other processes to use */
	if (fprintf(pseudo_save_file, "%s\n", ttyname(aslave)) == -1) {
		errlog("Error writing to the pseudo info file\n");
		fclose(pseudo_save_file);
		return -1;
	}
	fclose(pseudo_save_file);

	if (set_tty(aslave) == -1) {
		errlog("Error: Slave TTY not set properly\n");
		return -1;
	}

	return amaster;
}

/* Prepare logs from the read_buf and send them to the server */
bool prepare_log_send(char *read_buf, int max_read, int fd_socket)
{
	size_t buff_index = 0;	// Index for the read_buf string

	static char line[LINE_LEN] = { 0 };
	static size_t line_index = 0;	// Index for the line string

	char msg[MSG_LEN] = { 0 };	// Message to be sent to the server

	/* Kernel Version */
	static char kernel_version[KERNEL_VERSION_LEN] = "dummy_kernel";
	static int kernel_search_pos = 0;

	while (buff_index < max_read && read_buf[buff_index] != '\0') {
		if (read_buf[buff_index] == 'L')	// Check if there is a possibility of new kernel version
		{
			kernel_search_pos = line_index;
		}

		/* Send the log when a line is read */
		if (read_buf[buff_index] == '\n') {
			if (kernel_search_pos > 0) {
				if (strncmp(line + kernel_search_pos, "Linux version ", kernel_search_len) == 0) {
					sscanf(line + kernel_search_pos + kernel_search_len, "%s", kernel_version);
				}
				kernel_search_pos = 0;
			}

			/* Prepare the message */
			memset(msg, 0, sizeof(msg));
			if (snprintf(msg, sizeof(msg), "%s %s %s %s", "kernel:", kernel_version, "- msg", line) < 0) {
				errlog("Error copying the message - %m\n");
				return false;
			}

			/* Send message to the server */
			if (write(fd_socket, msg, strlen(msg)) < 0) {
				errlog("Error: Write to socket failed - %m\n");
				return false;
			}

			/* Reset the line buffer */
			line_index = 0;
			memset(line, 0, sizeof(line));

			buff_index++;
			continue;
		}

		/* If line is too big, send only the first few bytes and discard others. */
		if (line_index >= sizeof(line)) {
			line[line_index - 1] = 0;
			buff_index++;
			continue;
		}

		line[line_index++] = read_buf[buff_index++];
	}

	return true;
}

/* Read text from the TTY and send to send as logs */
bool read_send(int fd_tty, int fd_socket)
{
	char read_buf[LINE_LEN] = { 0 };	// Buffer to be read into.
	int read_size = 0;
	fd_set readset;
	int sel;
	int fdmax;

	int pseudo_tty = create_pseudo_tty();

	if (pseudo_tty == -1) {
		errlog("Error: Cannot create a psuedo terminal\n");
		return false;
	}

	fdmax = MAX(fd_tty, pseudo_tty);

	while (!kill_received) {
		do {
			FD_ZERO(&readset);
			FD_SET(fd_tty, &readset);
			FD_SET(pseudo_tty, &readset);

			sel = select(fdmax + 1, &readset, NULL, NULL, NULL);
		}
		while (sel == -1 && errno == EINTR && !kill_received);

		memset(read_buf, 0, sizeof(read_buf));
		if (FD_ISSET(fd_tty, &readset)) {
			read_size = read(fd_tty, read_buf, sizeof(read_buf) - 1);

			if (read_size == 0) {
				continue;
			}
			if (read_size < 0) {
				if (errno == EAGAIN) {
					continue;
				}
				errlog("Error: Read from tty failed - %m\n");
				return false;
			}

			/* Send the read data to the pseudo terminal */
			if (write(pseudo_tty, read_buf, read_size) < 0) {
				if (errno == EAGAIN)	// Output buffer full - flush it.
				{
					tcflush(pseudo_tty, TCIOFLUSH);
					continue;
				}

				errlog("Error: Write to pseudo tty failed - %m\n");
				return false;
			}

			/* Prepare log message and send to the server */
			if (!prepare_log_send(read_buf, sizeof(read_buf), fd_socket)) {
				errlog("Error: Sending log failed - %m\n");
				return false;
			}
		}
		/*if (FD_ISSET(fd_tty, &readset)) */
		if (kill_received) {
			break;
		}

		/* Check if there is an data in the pseudo terminal's buffer */
		if (FD_ISSET(pseudo_tty, &readset)) {
			read_size = read(pseudo_tty, read_buf, sizeof(read_buf) - 1);

			if (read_size == 0) {
				continue;
			}
			if (read_size < 0) {
				if (errno == EAGAIN) {
					continue;
				}
				errlog("Error: Read from pseudo tty failed - %m\n");
				return false;
			}

			if (write(fd_tty, read_buf, read_size) < 0) {
				if (errno == EAGAIN)	// Output buffer full - flush it.
				{
					tcflush(fd_tty, TCIOFLUSH);
					continue;
				}

				errlog("Error: Write to tty failed - %m\n");
				return false;
			}
		}		/*if (FD_ISSET(pseudo_tty,&readset)) */
	}			/*while (!kill_received) */

	return true;
}

void cleanup()
{
	remove(pseudo_tty_save_file);
	tcsetattr(fd_tty, TCSAFLUSH, &orig_tty_state);	//Restore original settings
	close(fd_tty);
	close(fd_soc);
	fclose(error_file);
}

void sig_kill(int signum)
{
	kill_received = true;
}

void register_kill()
{
	struct sigaction sigact;
	sigset_t sigset;

	sigemptyset(&sigset);
	memset(&sigact, 0, sizeof sigact);
	sigact.sa_handler = sig_kill;
	sigact.sa_mask = sigset;

	sigaction(SIGHUP, &sigact, NULL);
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGQUIT, &sigact, NULL);
	sigaction(SIGPIPE, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGKILL, &sigact, NULL);
	sigaction(SIGABRT, &sigact, NULL);
}

void usage(char *prog_name)
{
	printf("Usage:\n");
	printf("\t%s TTY ip_version(4 or 6) hostname port [baud rate (like 57600)]\n", prog_name);
	printf("\t%s -h : For this help\n", prog_name);
	printf("Example:\n\t./bmc-log /dev/ttyS1 4 netcons.any.facebook.com 1514\n");
	printf("\tOR\n\t./bmc-log /dev/ttyS1 6 netcons6.any.facebook.com 1514 57600\n");
}

bool parse_user_input(int nargs, char **args, char *read_tty, int read_tty_size, int *ip_version)
{
	if (nargs < 5) {
		if ((nargs > 1) && ((strcmp(args[1], "-h") == 0) || (strcmp(args[1], "--help") == 0))) {
			usage(args[0]);
			return false;	// Not an error but returning -1 for the main function to return
		}
		fprintf(stderr, "Error: Invalid number of arguments\n");
		usage(args[0]);
		return false;
	}

	if (strlen(args[1]) > read_tty_size) {
		fprintf(stderr, "Error: TTY too long\n");
		usage(args[0]);
		return false;
	}

	/* TTT to read the logs from */
	strncpy(read_tty, args[1], read_tty_size);

	/* IP Version, IP Address and Port of the netcons server */
	*ip_version = atoi(args[2]);
	if (*ip_version != IPV4 && *ip_version != IPV6) {
		fprintf(stderr, "Error: Invalid IP Version input\n");
		usage(args[0]);
		return false;
	}

	hostname = args[3];
	port = atoi(args[4]);

	baud_rate = B57600;
	if (nargs == 6)
		baud_rate = atoi(args[5]);

	return true;
}

int main(int argc, char **argv)
{
	char read_tty[TTY_LEN] = { 0 };
	int ip_version;
	int socket_domain = AF_UNSPEC;
	char cmd[COMMAND_LEN] = { 0 };

	/* Open the error log file */
	error_file = fopen(error_log_file, "a+");
	if (!error_file) {
		printf("Error: Unable to open log file - %m\n");
		return 1;
	}

	/* Register actions upon interrupts */
	register_kill();

	/* Parse the user input */
	if (!parse_user_input(argc, argv, read_tty, sizeof(read_tty), &ip_version)) {
		return 2;
	}

	snprintf(cmd, sizeof(cmd), "%s %s", uS_console, "connect");
	if (system(cmd) == -1) {
		errlog("Error: Unable to connect to the micro-server\n");
		return 3;
	}

	/* Create a socket to communicate with the netcons server */
	socket_domain = (ip_version == IPV4) ? AF_INET : AF_INET6;
	fd_soc = socket(socket_domain, SOCK_DGRAM, 0);
	if (fd_soc == -1) {
		errlog("Error: Socket creation failed -  %m\n");
		return 4;
	}

	if (ip_version == IPV4) {	/* IPv4 */
		struct sockaddr_in tgt_addr;
		if (!prepare_sock(&tgt_addr)) {
			close(fd_soc);
			errlog("Error: Socket not valid\n");
			return 5;
		}

		if (connect(fd_soc, (struct sockaddr *)&tgt_addr, sizeof(tgt_addr)) == -1) {
			close(fd_soc);
			errlog("Error: Socket connection failed - %m\n");
			return 6;
		}

	} else {		/* IPv6 */

		struct sockaddr_in6 tgt_addr6;
		if (!prepare_sock6(&tgt_addr6)) {
			close(fd_soc);
			errlog("Error: Socket not valid\n");
			return 5;
		}

		if (connect(fd_soc, (struct sockaddr *)&tgt_addr6, sizeof(tgt_addr6)) == -1) {
			close(fd_soc);
			errlog("Error: Socket connection failed - %m\n");
			return 6;
		}
	}

	/* TTY Operations */
	if ((fd_tty = open(read_tty, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK)) == -1) {
		close(fd_soc);
		errlog("Error: Serial Port %s open failed - %m\n", read_tty);
		return 7;
	}

	if (!set_tty(fd_tty)) {
		errlog("Error: tty not set properly\n");
		cleanup();
		return 8;
	}

	/* Read, prepare and send the logs */
	if (!read_send(fd_tty, fd_soc)) {
		errlog("Error: Sending logs failed\n");
		cleanup();
		return 9;
	}

	cleanup();
	return 0;
}
