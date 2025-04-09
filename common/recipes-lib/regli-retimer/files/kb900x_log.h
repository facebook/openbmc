#ifndef __KB_LOG_H
#define __KB_LOG_H

#include <stdio.h>
#include <time.h>

#define LOG_VERSION "0.1.0"

typedef enum {
  LOG_DEBUG = 0,
  LOG_INFO = 1,
  LOG_WARN = 2,
  LOG_ERR = 3,
  LOG_FATAL = 4,
  LOG_LEVEL_LENGTH = 5
} LOG_LEVEL;

void kandou_log_msg(LOG_LEVEL level, const char *file, int line,
                    const char *fmt, ...);
void kandou_log_set_level(int level);
void kandou_log_set_output_file(FILE *fp);
void kandou_log_set_quiet(int enable);

#define KANDOU_DEBUG(...)                                                      \
  kandou_log_msg(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define KANDOU_INFO(...)                                                       \
  kandou_log_msg(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define KANDOU_WARN(...)                                                       \
  kandou_log_msg(LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define KANDOU_ERR(...) kandou_log_msg(LOG_ERR, __FILE__, __LINE__, __VA_ARGS__)
#define KANDOU_FATAL(...)                                                      \
  kandou_log_msg(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

#endif // __KB_LOG_H
