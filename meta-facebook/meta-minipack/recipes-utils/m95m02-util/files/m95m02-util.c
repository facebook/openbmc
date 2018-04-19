/*
 * m95m02-util.c
 *
 * Copyright 2018-present Facebook. All Rights Reserved.
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
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#define MAX_BUF_SIZE 1024*256

unsigned char g_buf[MAX_BUF_SIZE];
int check_file_parameter(long offset, long length);
long get_file_size(char * file);
int read_file(char* file, long offset, long length);
int write_file(char* file, long offset, long length);
int erase_eeprom(char* file);
int read_eeprom (char* eerpom, char* file);
int write_eeprom(char* eerpom, char* file);
void format_file(char* src_file, char* new_file);
void usage();

long get_file_size(char * file)
{
  FILE* fp;
  long file_size;

  fp = fopen(file, "rb");
  if (fp == NULL) {
    fprintf(stderr, "Error in open file[%s]\n", file);
    return -1;
  } else {
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fclose(fp);
  }
  return file_size;
}

int check_file_parameter(long offset, long length)
{
  if (offset < 0) {
    fprintf(stderr, "Offset should be a positive value. \n");
    return -1;
  }
  if (length <= 0) {
    fprintf(stderr, "Length should be greater than 0. \n");
    return -1;
  }
  return 0;
}

int read_file(char* file, long offset, long length)
{
  FILE* fp;
  size_t read_size;

  if (check_file_parameter(offset, length) != 0)
    return -1;

  fp = fopen(file, "rb");

  if (fp == NULL) {
    fprintf(stderr, "Error in open file[%s]\n", file);
    return -1;
  } else {
    fseek(fp, offset, SEEK_SET);
    read_size = fread(g_buf, sizeof(unsigned char), length, fp);

    if (read_size != length) {
      fprintf(stderr, "Error, reading file [%s] expect(%ld) read(%ld)\n",
          file, length, read_size);
      return -1;
    }

    fclose(fp);
  }
  return 0;
}

int write_file(char* file, long offset, long length)
{
  FILE* fp;
  long write_size;

  if (check_file_parameter(offset, length) != 0)
    return -1;

  fp = fopen(file, "wb");
  if (fp == NULL) {
    fprintf(stderr, "Error in open file[%s]\n", file);
    return -1;
  } else {
    fseek(fp, offset, SEEK_SET);
    write_size = fwrite(g_buf, sizeof(unsigned char), length, fp);
    if (write_size <= 0 || write_size > MAX_BUF_SIZE)
        printf("Error write too much or write nothing. count[%ld]\n", write_size);
    else
        printf("Write bytes[%ld]\n", write_size);
    fclose(fp);
  }
  return 0;
}

int erase_eeprom(char* file)
{
  FILE* fp;
  long write_size;
  unsigned char erase_data[MAX_BUF_SIZE];
  memset(erase_data, 0xff, sizeof(erase_data));

  fp = fopen(file, "wb");

  if (fp == NULL) {
    fprintf(stderr, "Error in open file[%s]\n", file);
    return -1;
  } else {
    fseek(fp, 0, SEEK_SET);
    write_size = fwrite(erase_data, sizeof(unsigned char), sizeof(erase_data), fp);
    printf("Erase bytes[%ld]\n", write_size);
    fclose(fp);
  }
  return 0;
}

int read_eeprom(char* eeprom, char* file)
{
  long offset = 0;
  long length = get_file_size(eeprom);
  //memset(g_buf, 0xff, sizeof(g_buf));
  if (read_file(eeprom, offset, length) != -1) {
    write_file(file, offset, length);
    printf("Read eeprom done.\n");
  } else {
    printf("Read failed.\n");
  }

  return 0;
}

int write_eeprom(char* eeprom, char* target_file)
{
  long offset = 0;
  long read_bytes = get_file_size(target_file);

  /* It needs to read file first store the data in g_buf. */
  if (read_file(target_file, 0, read_bytes) != -1)
  {
      erase_eeprom(eeprom);
      write_file(eeprom, offset, read_bytes);
  }
  return 0;
}

/* Format the source file size to eeprom size */
void format_file(char* src_file, char* new_file)
{
  long file_size = get_file_size(src_file);
  memset(g_buf, 0xff, sizeof(g_buf));
  read_file(src_file, 0, file_size);
  write_file(new_file, 0, sizeof(g_buf));
}

void usage()
{
  printf("\
    Usage: \n\
    m95m02-util write [EEPROM] [FILE] \n\
    m95m02-util read [EEPROM] [FILE]\n\
    m95m02-util format_file [FILE] [NEW FILE NAME]\n\
    m95m02-util erase [EEPROM]\n");
}

int main(int argc, char *argv[])
{
  if (argc >= 3 || argc <= 4) {
    char * cmd = argv[1];
    char * eeprom_node = argv[2];

    if (argc == 3) {
      if (!strcmp(cmd, "erase"))
          erase_eeprom(eeprom_node);
      else {
          usage();
          exit(-1);
      }
    } else if (argc == 4) {
      if (!strcmp(cmd, "write")) {
          char * file_for_write = argv[3];
          write_eeprom(eeprom_node, file_for_write);
      } else if (!strcmp(cmd, "read")) {
          char * read_to_file = argv[3];
          read_eeprom(eeprom_node, read_to_file);
      } else if (!strcmp(cmd, "format_file")) {
          format_file(argv[2], argv[3]);
      } else {
          usage();
          exit(-1);
      }
    } else {
      usage();
      exit(-1);
    }
  }
  else {
    usage();
    exit(-1);
  }

  return 0;
}


