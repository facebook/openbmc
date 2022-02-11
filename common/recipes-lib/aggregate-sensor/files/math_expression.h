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
#ifndef _MATH_EXPRESSION_H_
#define _MATH_EXPRESSION_H_
/* Rules:
 * 1. Expression is always parsed left to right with no regard to operator
 *    precedence. When in doubt, use parenthesis. Also use
 *    expression_print() which should clearly show the order of evaluation.
 *    Example: Input: "4 * a + 5 * b - 6" ->
 *    ( ( ( ( 4.00000 * a  ) + 5.00000  ) * b  ) - 6.00000  ) which is most
 *    probably not what you want. Hence use "( 4 * a ) + ( 5 * b ) - 6" ->
 *    ( ( ( 4.00000 * a  ) + ( 5.00000 * b  )  ) - 6.00000  )
 *
 * 2. Expressions are parsed using spaces. So, 'a*b+c' is invalid while
 *    'a * b + c' is valid.
 */

/* The function which is fed into expression_parse which stores this 
 * with the variable, thus keeping all the string parsing and comparision
 * at parse time rather than at evaluation time */
typedef int (*get_value_func)(void *state, float *value);

/* Full description of the variable */
typedef struct {
  /* Name of the variable used in the expression */
  char  name[32];
  /* Function to be called to get the current value 
   * of the variable */
  get_value_func value;
  /* Any state information which will be useful to determine
   * the variable the call is interested in. */
  void  *state;
} variable_type;

/* Opaque object with full description of the expression */
struct expression_type_s;
typedef struct expression_type_s expression_type;

/* Parse the expression provided in 'str' and return the evaluatable 
 * expression object. The list of variables (see above) is also taken.
 * A copy of the variable (name, value() and state) is made so the
 * caller may free it after this function returns, but must maintain
 * the scope of 'value' & 'state' if they are dynamic objects */
expression_type *expression_parse(const char *str, variable_type *vars, size_t num);

/* Evaluate the expression. Note, multiple calls to the functions
 * provided in 'vars' in expression_parse might be made */
int expression_evaluate(expression_type *op, float *value);

/* Destroy the object created in expression_parse */
void expression_destroy(expression_type *exp);

/* Prints the expression with information on the order of evaluation */
void expression_print(expression_type *exp);

#endif
