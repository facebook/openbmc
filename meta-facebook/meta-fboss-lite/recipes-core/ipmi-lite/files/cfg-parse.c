/* Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This file defines all common helper functions for IPMIv2 and the
 * configuration parsing.
 * for those platform-specific helper functions, to be defined in the
 * plat_support_api.c file.
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
#include "cfg-parse.h"
#include <cjson/cJSON.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "common-handles.h"

#define PLAT_CFG_FILE "/usr/local/fbpackages/ipmid/config.json"

#define INSERT_CFG_ENT(_ptr)              \
  do {                                    \
    if (!g_cfg->cfg_ent_list) {           \
      g_cfg->cfg_ent_list = (_ptr);       \
      (_ptr)->next = NULL;                \
    } else {                              \
      (_ptr)->next = g_cfg->cfg_ent_list; \
      g_cfg->cfg_ent_list = (_ptr);       \
    }                                     \
    g_cfg->cnt++;                         \
  } while (0)

static cfg_item_t* get_cfg_item(const char* name) {
  cfg_item_t* ptr = g_cfg->cfg_ent_list;
  while (ptr) {
    if (!strcmp(name, ptr->name))
      return ptr;
    ptr = ptr->next;
  }
  return NULL;
}

static int add_cfg_item_num(const char* name, int val) {
  cfg_item_t* ptr;

  ptr = calloc(1, sizeof(cfg_item_t));
  if (!ptr) {
    syslog(LOG_ERR, "failed to allocate memory for cfg_item object.\n");
    return -1;
  }
  strncpy(ptr->name, name, MAX_NAME_SIZE - 1);
  ptr->type = NUMBER;
  ptr->u.ival = val;
  // no race condition here as all entities are inserted before reading.
  INSERT_CFG_ENT(ptr);
  return 0;
}

static int add_cfg_item_bool(const char* name, bool val) {
  cfg_item_t* ptr;

  ptr = calloc(1, sizeof(cfg_item_t));
  if (!ptr) {
    syslog(LOG_ERR, "failed to allocate memory for cfg_item object.\n");
    return -1;
  }
  strncpy(ptr->name, name, MAX_NAME_SIZE - 1);
  ptr->type = BOOLEAN;
  ptr->u.bval = val;
  INSERT_CFG_ENT(ptr);
  return 0;
}

static int add_cfg_item_str(const char* name, char* str_val) {
  cfg_item_t* ptr;

  ptr = calloc(1, sizeof(cfg_item_t));
  if (!ptr) {
    syslog(LOG_ERR, "failed to allocate memory for cfg_item object.\n");
    return -1;
  }
  strncpy(ptr->name, name, MAX_NAME_SIZE - 1);
  ptr->type = STRING;
  strncpy(ptr->u.str, str_val, MAX_VAL_STR_SIZE - 1);
  INSERT_CFG_ENT(ptr);
  return 0;
}

static int add_cfg_item_kv(const char* name) {
  cfg_item_t* ptr;

  ptr = calloc(1, sizeof(cfg_item_t));
  if (!ptr) {
    syslog(LOG_ERR, "failed to allocate memory for cfg_item object.\n");
    return -1;
  }
  strncpy(ptr->name, name, MAX_NAME_SIZE - 1);
  ptr->type = KV_ACCESS;
  INSERT_CFG_ENT(ptr);
  return 0;
}

int get_cfg_item_str(const char* name, char* val, uint8_t len) {
  if (!name || !val) {
    syslog(LOG_ERR, "invalid parameters");
    return -1;
  }
  cfg_item_t* ci = get_cfg_item(name);
  if (ci == NULL) {
    syslog(LOG_ERR, "\"%s\" is not configured", name);
    return -1;
  }
  if (ci->type == STRING) {
    strncpy(val, ci->u.str, len);
    return 0;
  } else {
    val[0] = '\0';
    return -1;
  }
}

int get_cfg_item_num(const char* name, int* val) {
  if (!name || !val) {
    syslog(LOG_ERR, "invalid parameters");
    return -1;
  }
  cfg_item_t* ci = get_cfg_item(name);
  if (ci == NULL) {
    syslog(LOG_ERR, "\"%s\" is not configured", name);
    return -1;
  }
  if (ci->type == NUMBER) {
    *val = ci->u.ival;
    return 0;
  }
  return -1;
}

int get_cfg_item_bool(const char* name, bool* val) {
  cfg_item_t* ci;

  if (!name || !val) {
    syslog(LOG_ERR, "invalid parameters");
    return -1;
  }
  if (NULL == (ci = get_cfg_item(name))) {
    syslog(LOG_ERR, "\"%s\" is not configured", name);
    return -1;
  }
  if (ci->type == BOOLEAN) {
    *val = ci->u.bval;
    return 0;
  }
  return -1;
}

int get_cfg_item_kv(const char* name) {
  if (!name) {
    syslog(LOG_ERR, "invalid parameters");
    return -1;
  }
  cfg_item_t* ci = get_cfg_item(name);
  if (ci == NULL) {
    syslog(LOG_ERR, "\"%s\" is not configured", name);
    return -1;
  }
  return ci->type == KV_ACCESS ? 0 : -1;
}

int get_cfg_item_fru(const char* name, fru_t* fru) {
  return 0;
}

void free_cfg_items() {
  cfg_item_t* ptr = g_cfg->cfg_ent_list;
  cfg_item_t* cur;

  while (ptr) {
    cur = ptr;
    ptr = ptr->next;
    free(cur);
  }
}

int plat_config_parse() {
  FILE* fd;
  int seek_end_result;
  int seek_set_result;
  long cfg_file_size;
  char* input_json_buffer = NULL;
  size_t read_result;
  int status = 0;
  cJSON* root = NULL;
  cJSON* item;
  cJSON *cobjs, *cobj;
  cJSON *name, *type, *val;

  fd = fopen(PLAT_CFG_FILE, "rb");
  if (fd == NULL) {
    syslog(LOG_ERR, "error in open file %s\n", PLAT_CFG_FILE);
    return -1;
  }
  seek_end_result = fseek(fd, 0, SEEK_END);
  if (seek_end_result != 0) {
    syslog(LOG_ERR, "fseek end error, %i\n", seek_end_result);
    fclose(fd);
    return -1;
  }

  cfg_file_size = ftell(fd);
  if (cfg_file_size < 0) {
    syslog(LOG_ERR, "ftell error, %li\n", cfg_file_size);
    fclose(fd);
    return -1;
  }

  seek_set_result = fseek(fd, 0, SEEK_SET);
  if (seek_set_result != 0) {
    syslog(LOG_ERR, "fseek set error, %i\n", seek_set_result);
    fclose(fd);
    return -1;
  }

  if (cfg_file_size == 0)
    input_json_buffer = (char*)calloc(1, 1);
  else
    input_json_buffer = (char*)malloc(cfg_file_size);

  if (input_json_buffer == NULL) {
    syslog(LOG_ERR, "memory allocation failed, %li bytes\n", cfg_file_size);
    fclose(fd);
    status = -1;
    goto end;
  }
  read_result = fread(input_json_buffer, 1, cfg_file_size, fd);
  fclose(fd);

  if (read_result < (size_t)cfg_file_size) {
    syslog(LOG_ERR, "fread error, %lu\n", (unsigned long)read_result);
    status = -1;
    goto end;
  }

  root = cJSON_Parse(input_json_buffer);
  if (root == NULL) {
    const char* error_ptr = cJSON_GetErrorPtr();
    if (error_ptr != NULL) {
      syslog(LOG_ERR, "Error before: %s\n", error_ptr);
    }
    status = -1;
    goto end;
  }

  item = cJSON_GetObjectItemCaseSensitive(root, "platform");
  if (cJSON_IsNull(item) || !cJSON_IsString(item)) {
    syslog(LOG_ERR, "failed to read \"platform\" item\n");
    status = -1;
    goto end;
  }
  strncpy(g_cfg->plat_name, cJSON_GetStringValue(item), MAX_NAME_SIZE - 1);

  // read in all cfg values.
  cobjs = cJSON_GetObjectItemCaseSensitive(root, "configure");
  if (!cJSON_IsArray(cobjs)) {
    syslog(LOG_ERR, "failed to read \"configure\" array\n");
    status = -1;
    goto end;
  }

  cJSON_ArrayForEach(cobj, cobjs) {
    name = cJSON_GetObjectItemCaseSensitive(cobj, "name");
    type = cJSON_GetObjectItemCaseSensitive(cobj, "type");

    if (!cJSON_IsString(name) || !cJSON_IsString(type)) {
      syslog(LOG_ERR, "invalid value of \"name\" or \"type\"\n");
      status = -1;
      goto end;
    }

    if (!strcmp("number", cJSON_GetStringValue(type))) {
      val = cJSON_GetObjectItemCaseSensitive(cobj, "val");
      add_cfg_item_num(
          cJSON_GetStringValue(name), (int)cJSON_GetNumberValue(val));

    } else if (!strcmp("string", cJSON_GetStringValue(type))) {
      val = cJSON_GetObjectItemCaseSensitive(cobj, "val");
      add_cfg_item_str(cJSON_GetStringValue(name), cJSON_GetStringValue(val));

    } else if (!strcmp("boolean", cJSON_GetStringValue(type))) {
      val = cJSON_GetObjectItemCaseSensitive(cobj, "val");
      add_cfg_item_bool(cJSON_GetStringValue(name), cJSON_IsTrue(val));
    } else if (!strcmp("kv-access", cJSON_GetStringValue(type))) {
      add_cfg_item_kv(cJSON_GetStringValue(name));
    } else if (!strcmp("fru", cJSON_GetStringValue(type))) {
      syslog(LOG_ERR, "fru unsupported yet \"%s\"\n", type->valuestring);
    } else {
      syslog(LOG_ERR, "unsupported type \"%s\"\n", type->valuestring);
      status = -1;
      goto end;
    }
  } // end of for-loop

end:
  if (input_json_buffer)
    free(input_json_buffer);
  if (root)
    cJSON_Delete(root);
  if (status)
    free_cfg_items();

  return status;
}
