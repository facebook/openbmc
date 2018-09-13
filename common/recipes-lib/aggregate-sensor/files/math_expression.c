/* Math expression parsing and evaluation.
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
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include "math_expression.h"

typedef enum {
  TERM_VARIABLE,
  TERM_CONSTANT
} term_type;

typedef struct {
  term_type type;
  union {
    variable_type var;
    float         constant;
  } term;
} expression_term_type;

typedef enum {
  OP_INVALID = 0, /* No R value */
  OP_ADD, /* L + R */
  OP_SUBTRACT, /* L - R */
  OP_MULTIPLY, /* L * R */
  OP_DIVIDE /* L / R */
} operator_type;

struct expression_type_s {
  operator_type type;
  expression_term_type *left_exp_term;
  expression_type      *left_exp_group;
  expression_term_type *right_exp_term;
  expression_type      *right_exp_group;
  expression_type      *parent;
};

static operator_type get_operator(char *str)
{
  operator_type op;

  if (str[1] != '\0') {
    return OP_INVALID;
  }

  switch(str[0]) {
    case '+':
      op = OP_ADD;
      break;
    case '-':
      op = OP_SUBTRACT;
      break;
    case '*':
      op = OP_MULTIPLY;
      break;
    case '/':
      op = OP_DIVIDE;
      break;
    default:
      op = OP_INVALID;
      break;
  }
  return op;
}

static bool is_constant(char *str)
{
  bool period_done = false;
  if (str[0] == '-')
    str++;
  while(*str) {
    if (period_done == false && *str == '.') {
      period_done = true;
    } else if (!isdigit(*str)) {
      return false;
    }
    str++;
  }
  return true;
}

static expression_type *alloc_expression(expression_type *parent)
{
  expression_type *o = calloc(1, sizeof(expression_type));
  if (!o) {
    return NULL;
  }
  o->parent = parent;
  return o;
}

static expression_term_type *alloc_expression_term(char *name, variable_type *va, int num)
{
  int i;
  expression_term_type *term = calloc(1, sizeof(expression_term_type));
  if (!term) {
    return NULL;
  }
  term->type = is_constant(name) ? TERM_CONSTANT : TERM_VARIABLE;
  if (term->type == TERM_CONSTANT) {
    term->term.constant = atof(name);
  } else {
    /* Look for the variable name in the list and copy it over */
    for (i = 0; i < num; i++) {
      if (!strncmp(name, va[i].name, sizeof(va[i].name))) {
        term->term.var = va[i];
        break;
      }
    }
    if (i == num) {
      /* Could not find the variable */
      free(term);
      return NULL;
    }
  }
  return term;
}

static int expression_term_evaluate(expression_term_type *term, float *value)
{
  if (term->type == TERM_CONSTANT) {
    *value = term->term.constant;
    return 0;
  }
  return term->term.var.value(term->term.var.state, value);
}

expression_type *expression_parse(const char *user_str, variable_type *vars, size_t num)
{
  char *token, *saveptr;
  expression_type *tmp, *current = NULL, *ret = NULL;
  char *str;

  str = calloc(strlen(user_str) + 1, 1);
  if (!str) {
    return NULL;
  }
  strcpy(str, user_str);

  ret = current = alloc_expression(NULL);
  if (!current) {
    free(str);
    return NULL;
  }

  for(token = strtok_r(str, " \n", &saveptr);
      token != NULL;
      token = strtok_r(NULL, " \n", &saveptr)) {

    assert(current);

    if (!strncmp(token, "(", 2)) {
      /* Start a new group and set it as current */
      current = alloc_expression(current);
      if (!current) {
        goto free_bail;
      }
    } else if (!strncmp(token, ")", 2)) {
      expression_type *parent = current->parent;
      /* Close the current group, set it as the parent's
       * left (if empty) or right term and finally set
       * the parent as the current group. */
      if (parent->left_exp_term == NULL && parent->left_exp_group == NULL) {
        parent->left_exp_group = current;
      } else if (parent->right_exp_term == NULL && parent->right_exp_group == NULL) {
        parent->right_exp_group = current;
      } 
      current = parent;
    } else if(get_operator(token) != OP_INVALID) {
      if (current->type == OP_INVALID) {
        /* Simple case, we have parsed the left term
         * and just parsing the operator. Just set it */
        current->type = get_operator(token);
      } else {
        /* If the current expression is already a valid
         * operation, then this is the second + in a + b + c.
         * Thus, artificially close the current group, create
         * a parent group with the left expression as the current
         * group and then set the new one as the current with
         * its operator set to the freshly parsed operator */
        tmp = alloc_expression(current->parent);
        if (!tmp) {
          goto free_bail;
        }
        tmp->left_exp_group = current;
        current->parent = tmp;
        tmp->type = get_operator(token);
        current = tmp;
      }
    } else {
      expression_term_type *term = alloc_expression_term(token, vars, num);
      if (!term) {
        goto free_bail;
      }
      /* This is either a variable or a constant. Allocate
       * a expression term and set it as the left (if available) or
       * right of the current expression group */
      if (current->left_exp_term == NULL && current->left_exp_group == NULL) {
        current->left_exp_term = term;
      } else if (current->right_exp_term == NULL && current->right_exp_term == NULL) {
        current->right_exp_term = term;
      } else {
        assert(0);
      }
    }
  }
  /* We are done parsing, ensure that we are the top group
   * and the user is not missing any unclosed parenthesis */
  assert(current->parent == NULL);
  free(str);
  return current;
free_bail:
  /* Free the partially created tree if it exists */
  expression_destroy(ret);
  return NULL;
}

int expression_evaluate(expression_type *exp, float *value)
{
  float l_val, r_val;
  int ret;
  assert(exp->left_exp_term || exp->left_exp_group);
  ret = exp->left_exp_term ? expression_term_evaluate(exp->left_exp_term, &l_val) :
    expression_evaluate(exp->left_exp_group, &l_val);
  if (ret) {
    return ret;
  }

  if (!exp->right_exp_term && !exp->right_exp_group) {
    /* Example use case would be the expression ( a + b ). This would be parsed
     * as ( ( a + b ) ) thus, the outer redundant group would have ( a + b )
     * as the left expression group with nothing on the right. Hense just
     * return success with the evaluation of the left expression. */
    *value = l_val;
    return 0;
  }
  ret = exp->right_exp_term ? expression_term_evaluate(exp->right_exp_term, &r_val) :
    expression_evaluate(exp->right_exp_group, &r_val);
  if (ret) {
    return ret;
  }

  switch(exp->type) {
    case OP_ADD:
      *value = l_val + r_val;
      break;
    case OP_SUBTRACT:
      *value = l_val - r_val;
      break;
    case OP_MULTIPLY:
      *value = l_val * r_val;
      break;
    case OP_DIVIDE:
      *value = l_val / r_val;
      break;
    default:
      assert(0);
  }
  return 0;
}

void expression_destroy(expression_type *exp)
{
  if (!exp) {
    return;
  }
  if (exp->left_exp_term)
    free(exp->left_exp_term);
  else if (exp->left_exp_group)
    expression_destroy(exp->left_exp_group);
  if (exp->right_exp_term)
    free(exp->right_exp_term);
  else if (exp->right_exp_group)
    expression_destroy(exp->right_exp_group);
  free(exp);
}

void expression_print(expression_type *exp)
{
  printf("( ");
  if (exp->left_exp_term) {
    if (exp->left_exp_term->type == TERM_CONSTANT) {
      printf("%2.5f ", exp->left_exp_term->term.constant);
    } else {
      printf("%s ", exp->left_exp_term->term.var.name);
    }
  } else {
      expression_print(exp->left_exp_group);
  }

  switch(exp->type) {
    case OP_ADD:
      printf("+ ");
      break;
    case OP_SUBTRACT:
      printf("- ");
      break;
    case OP_MULTIPLY:
      printf("* ");
      break;
    case OP_DIVIDE:
      printf("/ ");
      break;
    default:
      /* This case is possible only if there is no right term */
      assert(exp->right_exp_term == NULL && exp->right_exp_group == NULL);
      break;
  }

  if (exp->right_exp_term) {
    if (exp->right_exp_term->type == TERM_CONSTANT) {
      printf("%2.5f ", exp->right_exp_term->term.constant);
    } else {
      printf("%s ", exp->right_exp_term->term.var.name);
    }
  } else if (exp->right_exp_group) {
    expression_print(exp->right_exp_group);
  }
  printf(") ");
}

#ifdef __EXPRESSION_TEST__
int test_get_value(void *state, float *value)
{
  *value = *((float *)state);
  return 0;
}

int main(int argc, char *argv[])
{
  expression_type *op;
  variable_type *input;
  int i, num, rc;
  float ret;

  if (argc < 3) {
    return 0;
  }

  num = argc - 2;
  input = calloc(num, sizeof(*input));
  assert(input);
  for(i = 2; i < argc; i++) {
    char *tmp;
    variable_type *vi = &input[i-2];
    tmp = strtok(argv[i], "=");
    assert(tmp);
    strncpy(vi->name, tmp, sizeof(vi->name));
    tmp = strtok(NULL, "=");
    assert(tmp);
    vi->value = test_get_value;
    vi->state = malloc(sizeof(float));
    *((float *)vi->state) = atof(tmp);
  }
  op = expression_parse(argv[1], input, num);
  printf("Input:\n");
  for(i = 0; i < num; i++) {
    int rc;
    float val = 0;
    rc = input[i].value(input[i].state, &val);
    printf("%s : (rc =  %d) %4.3f\n", input[i].name, rc, val);
  }
  printf("Evaluating expression: ");
  expression_print(op);
  printf("\n");
  rc = expression_evaluate(op, &ret);
  printf("= (ret=%d) %4.3f\n", rc, ret);
  expression_destroy(op);
  return 0;
}
#endif
