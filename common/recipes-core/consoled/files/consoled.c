/*
 * consoled
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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
#include <unistd.h>
#include <syslog.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <signal.h>
#include <sys/stat.h>
#include <openbmc/pal.h>

#define BAUDRATE      B57600
#define CTRL_X        0x18
#define ASCII_ENTER   0x0D
#define MAX_LOGFILE_LINES 1200 // Maximum lines based on carriage returns or new line
#define MAX_LOGFILE_SIZE 102400 // 100KB size => 1200 lines of 80 characters each = ~108000B
static sig_atomic_t sigexit = 0;

static void
write_data(int file, char *buf, int len, char *fname) {
  int wlen = 0;
  char *tbuf = buf;
  while(len > 0) {
    errno = 0;
    wlen = write(file, tbuf, len);
    // In case wlen < 0, retry write()
    if (wlen >= 0) {
      len -= wlen;
      tbuf = tbuf + wlen;
    } else {
      syslog(LOG_WARNING, "write_data: write() failed to file %s | errno: %d",
          fname, errno);
      return;
    }
  }
}

static void
exit_session(int sig)
{
  sigexit = sig;
}

static void
print_usage() {
  printf("Usage: consoled [ %s ]\n", pal_server_list);
}

static void
run_console(char* fru_name, int term) {

  int i;
  int tty;    // serial port
  int buf_fd;  // Buffer File
  int blen;   // len for
  int nfd = 0;      // For number of fd
  int nevents;      // For number of events in fd
  int nline = 0;
  //int pid_fd;
  int flags;
  pid_t pid;        // For pid of the daemon
  uint8_t fru;
  char in;          // For stdin character
  char data[32];
  char pid_file[64];
  char devtty[32];  // For tty dev path
  char bfname[32];  // For buffer file path
  char old_bfname[32];  // For old buffer file path
  char buf[256];    // For buffer data
  struct termios ottytio, nttytio;  // For the tty dev

  int stdi;    // STDIN_FILENO
  int stdo; // STDOUT_FILENO
  struct termios ostditio, nstditio;  // For STDIN_FILENO
  struct termios ostdotio, nstdotio;  // For STDOUT_FILENO

  struct stat buf_stat;

  struct pollfd pfd[2];

  /* Start Daemon for the console buffering */
  if (!term) {
    daemon(0,1);
    openlog("consoled", LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "consoled: daemon started");
  }

  if (pal_get_fru_id(fru_name, &fru)) {
    exit(-1);
  }

  if (pal_get_fru_devtty(fru, devtty)) {
    exit(-1);
  }

  /* Handling the few Signals differently */
  signal(SIGHUP, exit_session);
  signal(SIGINT, exit_session);
  signal(SIGTERM, exit_session);
  signal(SIGPIPE, exit_session);
  signal(SIGQUIT, exit_session);

  /* Different flag value for tty dev */
  flags = term == 1 ? (O_RDWR | O_NOCTTY | O_NONBLOCK) :
    (O_RDONLY | O_NOCTTY | O_NONBLOCK);

  if ((tty = open(devtty, flags)) < 0) {
    syslog(LOG_WARNING, "Cannot open the file %s", devtty);
    exit(-1);
  }
  fcntl(tty, F_SETFL, O_RDWR);

  /* Changing the attributes of the tty dev */
  tcgetattr(tty, &ottytio);
  memcpy(&nttytio, &ottytio, sizeof(struct termios));
  cfmakeraw(&nttytio);
  cfsetspeed(&nttytio, BAUDRATE);
  tcflush(tty, TCIFLUSH);
  tcsetattr(tty, TCSANOW, &nttytio);
  pfd[0].fd = tty;
  pfd[0].events = POLLIN;
  nfd++;

  /* Buffering the console data into a file */
  sprintf(old_bfname, "/tmp/consoled_%s_log-old", fru_name);
  sprintf(bfname, "/tmp/consoled_%s_log", fru_name);
  if ((buf_fd = open(bfname, O_RDWR | O_APPEND | O_CREAT, 0666)) < 0) {
    syslog(LOG_WARNING, "Cannot open the file %s", bfname);
    exit(-1);
  }

  if (term) {
    /* Changing the attributes of STDIN_FILENO */
    stdi = STDIN_FILENO;
    tcgetattr(stdi, &ostditio);
    memcpy(&nstditio, &ostditio, sizeof(struct termios));
    cfmakeraw(&nstditio);
    tcflush(stdi, TCIFLUSH);
    tcsetattr(stdi, TCSANOW, &nstditio);

    /* Changing the attributes of STDOUT_FILENO */
    stdo = STDOUT_FILENO;
    tcgetattr(stdo, &ostdotio);
    memcpy(&nstdotio, &ostdotio, sizeof(struct termios));
    cfmakeraw(&nstdotio);
    tcflush(stdo, TCIFLUSH);
    tcsetattr(stdo, TCSANOW, &nstdotio);

    /* Adding STDIN_FILENO to the poll fd set */
    pfd[1].fd = stdi;
    pfd[1].events = POLLIN;
    nfd++;
  }

  /* Handling the input event from the  terminal and tty dev */
  while (!sigexit && (nevents = poll(pfd, nfd, -1 /* Timeout */))) {

    /* Input to the terminal from the user */
    if (term && nevents && nfd > 1 && pfd[1].revents > 0) {
      blen = read(stdi, &in, sizeof(in));
      if (blen < 1) {
        nfd--;
      }

      if (in == CTRL_X) {
        break;
      }

      write_data(tty, &in, sizeof(in), "tty");

      nevents--;
    }

    /* Input from the tty dev */
    if (nevents && pfd[0].revents > 0) {
      blen = read(tty, buf, sizeof(buf));
      if (blen > 0) {
        for (i = 0; i < blen; i++) {
          if (buf[i] == 0xD || buf[i] == 0xA)
            nline++;
        }
        write_data(buf_fd, buf, blen, bfname);
        fsync(buf_fd);
        if (term) {
          write_data(stdo, buf, blen, "STDOUT_FILENO");
        }
      } else if (blen < 0) {
        raise(SIGHUP);
      }

      // Get File stat information
      memset(&buf_stat, 0, sizeof(struct stat));
      fstat(buf_fd, &buf_stat);

      /* Log Rotation based on max number of lines or max file size */
      if (nline >= MAX_LOGFILE_LINES || buf_stat.st_size >= MAX_LOGFILE_SIZE) {
        close(buf_fd);
        remove(old_bfname);
        rename(bfname, old_bfname);
        if ((buf_fd = open(bfname, O_RDWR | O_APPEND | O_CREAT, 0666)) < 0) {
          syslog(LOG_WARNING, "Cannot open the file %s", bfname);
          exit(-1);
        }
        nline = 0;
      }
      nevents--;
    }
  }

  /* Close the console buffer file */
  close(buf_fd);

  /* Revert the tty dev to old attributes */
  tcflush(tty, TCIFLUSH);
  tcsetattr(tty, TCSANOW, &ottytio);

  /* Revert STDIN to old attributes */
  if (term) {
    tcflush(stdo, TCIFLUSH);
    tcsetattr(stdo, TCSANOW, &ostdotio);
    tcflush(stdi, TCIFLUSH);
    tcsetattr(stdi, TCSANOW, &ostditio);
  }

  /* Delete the pid file */
  if (!term)
    remove(pid_file);
}

int
main(int argc, void **argv) {
  int dev, rc, lock_file;
  char file[64];
  int term;
  char *fru_name;

  if (argc != 3) {
    print_usage();
    exit(1);
  }

  // A lock file for one instance of consoled for each fru
  sprintf(file, "/var/lock/consoled_%s", argv[1]);
  lock_file = open(file, O_CREAT | O_RDWR, 0666);
  rc = flock(lock_file, LOCK_EX | LOCK_NB);
  if(rc) {
    if(errno == EWOULDBLOCK) {
      printf("Another consoled %s instance is running...\n", argv[1]);
      exit(-1);
    }
  } else {

    fru_name = argv[1];

    if (!strcmp(argv[2], "--buffer")) {
        term = 0;
    } else if (!strcmp(argv[2], "--term")) {
        term = 1;
    } else {
      print_usage();
      exit(-1);
    }

    run_console(fru_name, term);
  }

  return sigexit;
}

