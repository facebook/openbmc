/*
 *
 * Copyright 2017-present Facebook. All Rights Reserved.
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

#include <assert.h>
#include <jansson.h>
#include <string.h>
#include "aggregate-sensor-internal.h"
#include <syslog.h>

/* Search through an array of strings for a particular string
 * and return the index if successful. Negative error code on
 * failure */
static int get_expression_idx(const char *value, 
    char name2idx_map[MAX_CONDITIONALS][MAX_STRING_SIZE], size_t num, size_t *idx)
{
  size_t j;
  for(j = 0; j < num; j++) {
    if (!strncmp(value, name2idx_map[j], MAX_STRING_SIZE)) {
      *idx = j;
      return 0;
    }
  }
  return -1;
}

static void cleanup_vars(variable_type *vars, size_t count)
{
  size_t i;

  for(i = 0; i < count; i++) {
    if (vars[i].state) {
      free(vars[i].state);
    }
  }
  free(vars);
}

static int logical_expression_parse(void *state, float *value)
{
  expression_type *exp = (expression_type *)state;
  return expression_evaluate(exp, value);
}

/* Load SENSOR[X]::sources[Y] a specific source variable */
static int load_variable(const char *name, json_t *obj, variable_type *var)
{
  json_t *fru_o, *id_o, *exp_o;
  struct sensor_src *s;

  if (!obj) {
    return -1;
  }
  strncpy(var->name, name, sizeof(var->name) - 1);

  exp_o = json_object_get(obj, "expression");
  fru_o = json_object_get(obj, "fru");
  id_o  = json_object_get(obj, "sensor_id");
  if (exp_o && json_is_string(exp_o)) {
    /* defer parsing the expression till we have loaded
     * all the variables. So, just set the function to call
     * and copy over the expression char string to be
     * parsed later */
    const char *str = json_string_value(exp_o);
    var->state = malloc(strlen(str) + 1);
    if (!var->state) {
      return -1;
    }
    strcpy((char *)var->state, str);
    var->value = logical_expression_parse;
  } else if (fru_o && id_o && json_is_number(fru_o) &&
      json_is_number(id_o)) {
    /* Copy the function pointer which will be called
     * when the value of this variable is required */
    var->value = get_sensor_value;

    /* Allocate the state which will be passed to
     * get_sensor_value (fru, id) */
    s = calloc(1, sizeof(struct sensor_src));
    if (!s) {
      return -1;
    }

    s->fru = json_integer_value(fru_o);
    s->id  = json_integer_value(id_o);

    var->state = s;
  } else {
    return -1;
  }

  return 0;
}

size_t sort_variables(variable_type *vars, int num_vars)
{
  int i, j;

  /* sort so all expression variables are towards the end */
  for (i = 0, j = num_vars-1; i < j; i++) {
    while (vars[j].value == logical_expression_parse && j >= 0)
      j--;
    /* vars[j] points to the first non-expression variable when
     * scanned from the last */
    if (i < j && vars[i].value == logical_expression_parse) {
      variable_type tmp = vars[j];
      vars[j] = vars[i];
      vars[i] = tmp;
      j--;
    }
  }

  return (size_t)(j + 1);
}

/* Parse SENSORS[X]::sources the JSON detailing the source sensors and
 * information on how to read them from PAL APIs (fru + id). */
static variable_type *load_variables(json_t *obj, size_t *count)
{
  size_t num_vars;
  void *iter;
  size_t i, exp_start;
  variable_type *vars;
  size_t redo_count = 0;

  if (!obj) {
    DEBUG("Getting sources failed!\n");
    return NULL;
  }

  *count = num_vars = json_object_size(obj);
  if (!num_vars) {
    DEBUG("0 variables in sources!\n");
    return NULL;
  }
  vars = calloc(num_vars, sizeof(variable_type));
  if (!vars) {
    return NULL;
  }
  for(i = 0, iter = json_object_iter(obj);
      i < num_vars && iter;
      i++, iter = json_object_iter_next(obj, iter)) {
    const char *key = json_object_iter_key(iter);
    json_t *val = json_object_iter_value(iter);

    if (load_variable(key, val, &vars[i])) {
      goto bail;
    }
  }
  
  exp_start = sort_variables(vars, num_vars);

  for (i = exp_start; i < num_vars;) {
    char *str_expression = (char *)vars[i].state;
    /* Use only the expressions which have been pased up till
     * now. This makes sure we are detect recursive expressions
     * in our sources and fail early. */
    vars[i].state = expression_parse(str_expression, vars, i);
    if (!vars[i].state) {
      size_t j;
      variable_type tmp;
      /* Parsing failed. We probably depend on another expression
       * which we are yet to parse. Just put this variable at the last
       * and shift everything left and continue. */

      vars[i].state = str_expression;
      if ((num_vars - i - 1) == redo_count) {
        /* The dependency cannot be met. we have a circular/recursive
         * dependency in the expressions or use of an unknown variable */
        goto bail;
      }
      redo_count++;
      tmp = vars[i];
      for (j = i + 1; j < num_vars; j++) {
        vars[j - 1] = vars[j];
      }
      vars[num_vars - 1] = tmp;
      continue;
    }
    /* Successful parse. free the string, reset the redo count and
     * begin parsing of the next expression variable */
    free(str_expression);
    redo_count = 0;
    i++;
  }
  return vars;
bail:
  cleanup_vars(vars, num_vars);
  return NULL;
}

/* Parse SENSORS[X]::composition if it is of type
 * "linear_expression". */
static int load_linear_eq(aggregate_sensor_t *snr, json_t *obj)
{
  variable_type *vars = 0;
  size_t num_vars = 0;
  json_t *tmp;

  snr->conditional = false;

  vars = load_variables(json_object_get(obj, "sources"), &num_vars);
  if (!vars) {
    return -1;
  }
  snr->num_expressions = 1;
  snr->expressions = calloc(1, sizeof(expression_type *));
  if (!snr->expressions) {
    DEBUG("Allocation failure");
    goto bail_linear_exp;
  }

  tmp = json_object_get(obj, "linear_expression");
  if (!tmp || !json_is_string(tmp)) {
    DEBUG("Getting key: linear_expression failed!\n");
    goto bail_linear_exp;
  }

  snr->expressions[0] = expression_parse(json_string_value(tmp), vars, num_vars);
  if (!snr->expressions[0]) {
    DEBUG("Expression parsing failed!\n");
    goto bail_linear_exp;
  }

  /* We don't need vars anymore */
  free(vars);
  return 0;
bail_linear_exp:
  cleanup_vars(vars, num_vars);
  return -1;
}

/* Parse SENSORS[X]::composition if it is of type
 * "conditional_linear_expression". */
static int load_linear_cond_eq(aggregate_sensor_t *snr, json_t *obj)
{
  json_t *tmp;
  json_t *tmp2;
  char name2idx_map[MAX_CONDITIONALS][MAX_STRING_SIZE];
  void *iter;
  size_t i;
  variable_type *vars = 0;
  size_t num_vars = 0;

  vars = load_variables(json_object_get(obj, "sources"), &num_vars);
  if (!vars) {
    return -1;
  }

  snr->conditional = true;

  tmp = json_object_get(obj, "linear_expressions");
  if (!tmp) {
    DEBUG("Getting key: linear_expressions failed!\n");
    goto bail_linear_exp;
  }

  snr->num_expressions = json_object_size(tmp);
  /* array of pointers to expressions */
  snr->expressions = calloc(snr->num_expressions, sizeof(expression_type *));
  if (!snr->expressions) {
    DEBUG("Allocation failure");
    goto bail_linear_exp;
  }

  for (i = 0, iter = json_object_iter(tmp);
      i < snr->num_expressions && iter; 
      iter = json_object_iter_next(tmp, iter), i++) {
    const char *key = json_object_iter_key(iter);
    json_t *val = json_object_iter_value(iter);
    if (!val || !json_is_string(val)) {
      DEBUG("Expression is not a string!\n");
      goto bail_exp_parse;
    }

    strncpy(&name2idx_map[i][0], key, MAX_STRING_SIZE - 1);

    DEBUG("Loading expression: %zu\n", i);
    snr->expressions[i] = expression_parse(json_string_value(val), vars, num_vars);
    if (!snr->expressions[i]) {
      DEBUG("Expression[%zu] parsing failed!\n", i);
      goto bail_exp_parse;
    }
  }
  /* This should never happen */
  assert(!iter);
  
  tmp = json_object_get(obj, "condition");
  if (!tmp) {
    DEBUG("Getting key condition failed\n");
    goto bail_exp_parse;
  }

  tmp2 = json_object_get(tmp, "key_type");
  if (!tmp2 || !json_is_string(tmp2)) {
    snr->cond_type = KEY_REGULAR;
  } else {
    if (!strcmp(json_string_value(tmp2), "regular")) {
      snr->cond_type = KEY_REGULAR;
    } else if (!strcmp(json_string_value(tmp2), "persistent")) {
      snr->cond_type = KEY_PERSISTENT;
    } else if (!strcmp(json_string_value(tmp2), "path")) {
      snr->cond_type = KEY_PATH;
    } else {
      goto bail_exp_parse;
    }
  }

  tmp2 = json_object_get(tmp, "key");
  if (!tmp2 || !json_is_string(tmp2)) {
    DEBUG("Getting key key failed\n");
    goto bail_exp_parse;
  }
  strncpy(snr->cond_key, json_string_value(tmp2), MAX_KEY_LEN - 1);
  tmp2 = json_object_get(tmp, "value_map");
  if (!tmp2) {
    DEBUG("Getting key value_map failed!\n");
    goto bail_exp_parse;
  }

  for (i = 0, iter = json_object_iter(tmp2);
      i < MAX_CONDITIONALS && iter; 
      iter = json_object_iter_next(tmp2, iter), i++) {
    const char *key = json_object_iter_key(iter);
    json_t *val = json_object_iter_value(iter);
    if (!val || !json_is_string(val)) {
      DEBUG("value_map[%zu] value get failed!\n", i);
      goto bail_exp_parse;
    }
    strncpy(snr->value_map[i].condition_value, key, MAX_VALUE_LEN);
    if (get_expression_idx(json_string_value(val), name2idx_map, 
        snr->num_expressions, &snr->value_map[i].formula_index)) {
      goto bail_exp_parse;
    }
  }
  snr->value_map_size = i;
  snr->default_expression_idx = -1;
  if ((tmp = json_object_get(tmp, "default_expression")) != NULL &&
      json_is_string(tmp)) {
    size_t idx;
    if (get_expression_idx(json_string_value(tmp), name2idx_map, 
          snr->num_expressions, &idx) == 0) {
      snr->default_expression_idx = (int)idx;
    }
  }
  /* We don't need vars anymore */
  free(vars);
  return 0;
bail_exp_parse:
  for (i = 0; i < snr->num_expressions; i++) {
    if (snr->expressions[i]) {
      expression_destroy(snr->expressions[i]);
    }
  }
  free(snr->expressions);
bail_linear_exp:
  cleanup_vars(vars, num_vars);
  return -1;
}

/* Parse SENSORS[X]::composition. Currently there exists
 * support for just conditional linear expression. This
 * can be expanded in the future for others */
static int load_composition(aggregate_sensor_t *snr, json_t *obj)
{
  json_t *tmp;
  int ret;

  if (!obj) {
    DEBUG("Getting key: composition failed!\n");
    return -1;
  }
  tmp = json_object_get(obj, "type");
  if (!tmp || !json_is_string(tmp)) {
    DEBUG("Getting type of composition failed!\n");
    return -1;
  }
  if (!strncmp(json_string_value(tmp), "conditional_linear_expression", 
        MAX_STRING_SIZE)) {
    ret = load_linear_cond_eq(snr, obj);
  } else if (!strncmp(json_string_value(tmp), "linear_expression", MAX_STRING_SIZE)) {
    ret = load_linear_eq(snr, obj);
  }
  else {
    DEBUG("Unsupported composition: %s\n", json_string_value(tmp));
    /* Others can go here as else if */
    return -1;
  }
  return ret;
}

/* Load SENSORS[X]::thresholds. */
static int load_thresholds(thresh_sensor_t *th, json_t *obj)
{
  json_t *tmp;

  th->flag = SETBIT(0, SENSOR_VALID);

  /* This is optional. So if the JSON does not contain
   * any threshold description, just consider all as 
   * NA */
  if (!obj) {
    return 0;
  }

  if ((tmp = json_object_get(obj, "ucr")) != NULL && json_is_real(tmp)) {
    th->flag = SETBIT(th->flag, UCR_THRESH);
    th->ucr_thresh = json_real_value(tmp);
  }
  if ((tmp = json_object_get(obj, "unc")) != NULL && json_is_real(tmp)) {
    th->flag = SETBIT(th->flag, UNC_THRESH);
    th->unc_thresh = json_real_value(tmp);
  }
  if ((tmp = json_object_get(obj, "unr")) != NULL && json_is_real(tmp)) {
    th->flag = SETBIT(th->flag, UNR_THRESH);
    th->unr_thresh = json_real_value(tmp);
  }
  if ((tmp = json_object_get(obj, "lcr")) != NULL && json_is_real(tmp)) {
    th->flag = SETBIT(th->flag, LCR_THRESH);
    th->lcr_thresh = json_real_value(tmp);
  }
  if ((tmp = json_object_get(obj, "lnc")) != NULL && json_is_real(tmp)) {
    th->flag = SETBIT(th->flag, LNC_THRESH);
    th->lnc_thresh = json_real_value(tmp);
  }
  if ((tmp = json_object_get(obj, "lnr")) != NULL && json_is_real(tmp)) {
    th->flag = SETBIT(th->flag, LNR_THRESH);
    th->lnr_thresh = json_real_value(tmp);
  }
  return 0;
}

/* Load SENSORS[X] */
static int load_sensor_conf(aggregate_sensor_t *snr, json_t *obj)
{
  json_t *tmp;

  if (!obj) {
    DEBUG("Index invalid\n");
    return -1;
  }

  tmp = json_object_get(obj, "name");
  if (!tmp || !json_is_string(tmp)) {
    DEBUG("Getting string key: name failed\n");
    return -1;
  }
  strncpy(snr->sensor.name, json_string_value(tmp), sizeof(snr->sensor.name) - 1);
  tmp = json_object_get(obj, "units");
  if (!tmp || !json_is_string(tmp)) {
    DEBUG("Getting string key: units failed\n");
    return -1;
  }
  strncpy(snr->sensor.units, json_string_value(tmp), sizeof(snr->sensor.units) - 1);

  if (load_thresholds(&snr->sensor, json_object_get(obj, "thresholds"))) {
    DEBUG("Loading of thresholds failed!\n");
    return -1;
  }
  tmp = json_object_get(obj, "poll_interval");
  snr->sensor.poll_interval = tmp ? json_integer_value(tmp) : 2;

  return load_composition(snr, json_object_get(obj, "composition"));
}

/* Load information of aggregate sensors given their information
 * from the json file path */
int load_aggregate_conf(const char *file)
{
  json_error_t error;
  json_t *conf;
  json_t *tmp;
  size_t i;
  int ret = -1;
  
  conf = json_load_file(file, 0, &error);
  if (!conf) {
    DEBUG("Loading %s failed!\n", file);
    return -1;
  }
  tmp = json_object_get(conf, "version");
  if (!tmp || !json_is_string(tmp)) {
    DEBUG("Getting CONF version failed!\n");
    goto bail;
  }
  DEBUG("Loading configuration version: %s\n", json_string_value(tmp));

  tmp = json_object_get(conf, "sensors");
  if (!tmp || !json_is_array(tmp)) {
    DEBUG("Loading sensors failed!\n");
    goto bail;
  }

  size_t offset = g_sensors_count;
  size_t sensors_count = json_array_size(tmp);
  size_t new_sensors_count = g_sensors_count + sensors_count;
  if (!sensors_count) {
    DEBUG("No sensors available!\n");
    ret = 0;
    goto bail;
  }
  // We can do shallow copy of the structs as long as they
  // are a single copy.
  aggregate_sensor_t *new_sensors = 
    realloc(g_sensors, new_sensors_count * sizeof(aggregate_sensor_t));
  if (!new_sensors) {
    DEBUG("Memory allocation failure!\n");
    if (g_sensors) {
      free(g_sensors);
    }
    g_sensors_count = 0;
    g_sensors = NULL;
    goto bail;
  }
  g_sensors = new_sensors;
  aggregate_sensor_t *sensors = g_sensors + offset;
  memset(sensors, 0, sizeof(aggregate_sensor_t) * sensors_count);

  DEBUG("Loading %zu sensors\n", sensors_count);
  for (i = 0; i < sensors_count; i++) {
    sensors[i].idx = g_sensors_count + i;
    DEBUG("Loading sensor: %zu as %zu\n", i, sensors[i].idx);
    ret = load_sensor_conf(&sensors[i], json_array_get(tmp, i));
    if (ret) {
      DEBUG("Loading configuration for sensor %zu failed!\n", i);
      free(g_sensors);
      g_sensors = NULL;
      g_sensors_count = 0;
      goto bail;
    }
  }
  g_sensors_count = new_sensors_count;
  ret = 0;
bail:
  json_decref(conf);
  return ret;
}


