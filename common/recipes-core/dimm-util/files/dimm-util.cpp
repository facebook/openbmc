/*
 *
 * Copyright 2014-present Facebook. All Rights Reserved.
 *
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
#include <getopt.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <openbmc/dimm.h>

#ifdef DEBUG_DIMM_UTIL
  #define DBG_PRINT(...) printf(__VA_ARGS__)
#else
  #define DBG_PRINT(...)
#endif


static int parse_dimm_label(char *label, uint8_t *dimmNum)
{
  // dimm label to dimm number look up
  for (int cpu = 0; cpu < num_cpus; ++cpu) {
    for (int dimm = 0; dimm < num_dimms_per_cpu; ++dimm) {
      if (strcasecmp(get_dimm_label(cpu, dimm), label) == 0) {
        *dimmNum = cpu * num_dimms_per_cpu + dimm;
        return 0;
      }
    }
  }
  *dimmNum = 0;
  return -1;
}

static int parse_cmdline_args(int argc, char **argv,
			      uint8_t *dimm, bool *json, bool *force, uint8_t *options)
{
  int ret, opt_index, idx;
  char *endptr = NULL;
  static const char *optstring = "d:jflic";
  struct option long_opts[] = {
    {"dimm"    , required_argument, NULL, 'd'},
    {"json"    , no_argument      , NULL, 'j'},
    {"force"   , no_argument      , NULL, 'f'},
    {"err_list", no_argument      , NULL, 'l'},
    {"err_inj" , required_argument, NULL, 'i'},
    {"err_clear" , no_argument    , NULL, 'c'},
    {NULL, 0,	NULL,	0},
  };

  *dimm = total_dimms;
  *json = false;
  while (1) {
    opt_index = 0;
    ret = getopt_long(argc, argv, optstring,
          long_opts, &opt_index);
    if (ret == -1)
      break;	/* end of arguments */

    switch (ret) {
    case 'd':
      *dimm = strtol(optarg, &endptr, 10);
      if ((*endptr != '\0' && *endptr != '%')) {
        if (parse_dimm_label(optarg, dimm) == -1) {
          printf("invalid argument \"--dimm %s\"\n", optarg);
          return ERR_INVALID_SYNTAX;
        }
      } else if ((*dimm > total_dimms)) {
        printf("Error: invalid \"--dimm %d\"\n", *dimm);
        return ERR_INVALID_SYNTAX;
      }
      break;
    case 'j':
      *json = true;
      break;
    case 'f':
      *force = true;
      break;
    case 'l':
      options[0] = OPTION_LIST_ERR;
      break;
    case 'c':
      options[0] = OPTION_CLEAR_ERR;
      break;
    case 'i':
      idx = pmic_err_index(optarg);
      if (idx < 0) {
        printf("Invalid error type: %s\n", optarg);
        return ERR_INVALID_SYNTAX;
      }
      options[0] = (uint8_t)idx;
      break;
    default:
      return ERR_INVALID_SYNTAX;
    }
  }
  if (optind < argc) {
    return ERR_INVALID_SYNTAX;
  }

  return 0;
}

static int
parse_arg_and_exec(int argc, char **argv, uint8_t fru_id,
                  int (*pF)(uint8_t, uint8_t, bool, uint8_t*)) {
  uint8_t dimm = total_dimms;
  uint8_t i, fru_start, fru_end = 0;
  bool    json = false;
  bool    force = false;
  int ret = 0;
  uint8_t options[MAX_EXTEND_OPTION] = {OPTION_LIST_ERR,}; // for extended parameters

  if (argc > 3) {
    // parse option fields,
    // skipping first 3 arguments, which will be "dimm-util fru cmd"
    if (parse_cmdline_args(argc - 2, &(argv[2]), &dimm, &json, &force, options) != 0)
      return ERR_INVALID_SYNTAX;
  }

  if (fru_id == fru_id_all) {
    fru_start = fru_id_min;
    fru_end   = fru_id_max + 1;
  } else {
    fru_start = fru_id;
    fru_end   = fru_id + 1;
  }

  for (i = fru_start; i < fru_end; ++i) {
    if (force == false) {
      ret = util_check_me_status(i);
      if (ret == 0) {
        ret = pF(i, dimm, json, options);
      } else {
        printf("Error: ME status check failed for fru:%s\n", fru_name[i - 1]);
        continue;
      }
    } else {
      ret = pF(i, dimm, json, options);
    }
  }

  return ret;
}

static void
print_usage_help(void) {
  int i, j;

  printf("Usage: dimm-util <");
  for (i = 0; i < num_frus; ++i) {
    printf("%s", fru_name[i]);
    if (i < num_frus - 1)
      printf("|");
  }
  printf("> <CMD> [OPT1] [OPT2] ...\n");
  printf("\nCMD list:\n");
  printf("   --serial   - get DIMM module serial number\n");
  printf("   --part     - get DIMM module part number\n");
  printf("   --raw      - dump raw SPD data\n");
  printf("   --cache    - read from SMBIOS cache\n");
  printf("   --config   - print DIMM configuration info\n");
  printf("   --pmic     - inject or show error of PMIC\n");
  printf("\nOPT list:\n");
  printf("   --dimm [N/Label] - DIMM number (");
  for (i = 0; i < total_dimms - 1; ++i)
    printf("%02d, ", i);
  printf("%02d) [default %d = ALL]\n", total_dimms - 1, total_dimms);
  printf("                        or\n");
  printf("                      DIMM Label  (");
  for (i = 0; i < num_cpus; ++i)
    for (j = 0; j < num_dimms_per_cpu; ++ j)
      if (i == num_cpus - 1 && j == num_dimms_per_cpu - 1)
        printf("%2s)\n", get_dimm_label(i, j));
      else
        printf("%2s, ", get_dimm_label(i, j));
  printf("   --json     - output in JSON format\n");
  printf("   --force    - skips ME status check\n");
  printf("   --err_list - show PMIC error\n");
  printf("   --err_inj  - the error to inject, option list:\n");
  printf("                <SWAout_OV|SWBout_OV|SWCout_OV|SWDout_OV|VinB_OV|VinM_OV|\n");
  printf("                 SWAout_UV|SWBout_UV|SWCout_UV|SWDout_UV|VinB_UV|\n");
  printf("                 Vin_switchover|\n");
  printf("                 high_temp|\n");
  printf("                 Vout_1v8_PG|\n");
  printf("                 high_current|\n");
  printf("                 current_limit|\n");
  printf("                 critical_temp_shutdown>\n");
  printf("   --err_clear - clear PMIC error\n");
}

static int
parse_fru(char* fru_str, uint8_t *fru) {
  uint8_t i, found = 0;

  for (i = 0; i < num_frus; ++i) {
    if (!strcmp(fru_str, fru_name[i])) {
      // fru_name string is 0 based but fru id is 1 based
      *fru = i + 1;
      found = 1;
      break;
    }
  }

  if (!found)
    return -1;
  else
    return 0;
}

int
main(int argc, char **argv) {
  uint8_t fru;
  int ret = 0;

  // init platform-specific variables, e.g. max dimms, max cpu, dimm labels,
  ret = plat_init();
  DBG_PRINT("num_frus(%d), num_dimms_per_cpu(%d), num_cpus(%d), total_dimms(%d)\n",
          num_frus, num_dimms_per_cpu, num_cpus, total_dimms);
  if (ret != 0) {
    printf("init failed (%d)\n", ret);
    return ret;
  }

  if (argc < 3)
    goto err_exit;

  if (parse_fru(argv[1], &fru) == -1) {
    goto err_exit;
  }

  if (!strcmp(argv[2], "--serial")) {
    ret = parse_arg_and_exec(argc, argv, fru, &util_get_serial);
  } else if (!strcmp(argv[2], "--part")) {
    ret = parse_arg_and_exec(argc, argv, fru, &util_get_part);
  } else if (!strcmp(argv[2], "--raw")) {
    ret = parse_arg_and_exec(argc, argv, fru, &util_get_raw_dump);
  } else if (!strcmp(argv[2], "--cache")) {
    ret = parse_arg_and_exec(argc, argv, fru, &util_get_cache);
  } else if (!strcmp(argv[2], "--config")) {
    ret = parse_arg_and_exec(argc, argv, fru, &util_get_config);
  } else if (!strcmp(argv[2], "--pmic")) {
    if (is_pmic_supported() == true) {
      ret = parse_arg_and_exec(argc, argv, fru, &util_pmic_err);
    } else {
      printf("Platform does not support PMIC function\n");
      goto err_exit;
    }
  } else {
    goto err_exit;
  }

  if (ret == ERR_INVALID_SYNTAX)
    goto err_exit;

  return ret;

err_exit:
  print_usage_help();
  return ERR_INVALID_SYNTAX;
}
