/*
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
#ifndef _CMOCK_H_
#define _CMOCK_H_
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define _GET_NTH_ARG(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, N, ...) N
#define COUNT_VARARGS(...) _GET_NTH_ARG("ignored", ##__VA_ARGS__, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define NAMES0() 
#define NAMES1() p1
#define NAMES2() p1, p2
#define NAMES3() p1, p2, p3
#define NAMES4() p1, p2, p3, p4
#define NAMES5() p1, p2, p3, p4, p5
#define NAMES6() p1, p2, p3, p4, p5, p6
#define NAMES7() p1, p2, p3, p4, p5, p6, p7
#define NAMES8() p1, p2, p3, p4, p5, p6, p7, p8
#define NAMES9() p1, p2, p3, p4, p5, p6, p7, p8, p9

#define PTYPES0() void
#define PTYPES1(t1) t1
#define PTYPES2(t1, t2) t1, t2
#define PTYPES3(t1, t2, t3) t1, t2, t3
#define PTYPES4(t1, t2, t3, t4) t1, t2, t3, t4
#define PTYPES5(t1, t2, t3, t4, t5) t1, t2, t3, t4, t5
#define PTYPES6(t1, t2, t3, t4, t5, t6) t1, t2, t3, t4, t5, t6
#define PTYPES7(t1, t2, t3, t4, t5, t6, t7) t1, t2, t3, t4, t5, t6, t7
#define PTYPES8(t1, t2, t3, t4, t5, t6, t7, t8) t1, t2, t3, t4, t5, t6, t7, t8
#define PTYPES9(t1, t2, t3, t4, t5, t6, t7, t8, t9) t1, t2, t3, t4, t5, t6, t7, t8, t9

#define PARAMS0() void
#define PARAMS1(t1) t1 p1
#define PARAMS2(t1, t2) t1 p1, t2 p2
#define PARAMS3(t1, t2, t3) t1 p1, t2 p2, t3 p3
#define PARAMS4(t1, t2, t3, t4) t1 p1, t2 p2, t3 p3, t4 p4
#define PARAMS5(t1, t2, t3, t4, t5) t1 p1, t2 p2, t3 p3, t4 p4, t5 p5
#define PARAMS6(t1, t2, t3, t4, t5, t6) t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6
#define PARAMS7(t1, t2, t3, t4, t5, t6, t7) t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7
#define PARAMS8(t1, t2, t3, t4, t5, t6, t7, t8) t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8
#define PARAMS9(t1, t2, t3, t4, t5, t6, t7, t8, t9) t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8, t9 p9

#define __FULL_PARAMS(n, ...) PARAMS##n(__VA_ARGS__)
#define _FULL_PARAMS(n, ...) __FULL_PARAMS(n, __VA_ARGS__)
#define FULL_PARAMS(...) _FULL_PARAMS(COUNT_VARARGS(__VA_ARGS__), __VA_ARGS__)

#define __FULL_PTYPES(n, ...) PTYPES##n(__VA_ARGS__)
#define _FULL_PTYPES(n, ...) __FULL_PTYPES(n, __VA_ARGS__)
#define FULL_PTYPES(...) _FULL_PTYPES(COUNT_VARARGS(__VA_ARGS__), __VA_ARGS__)

#define __NAMES(n) NAMES##n()
#define _NAMES(n) __NAMES(n)
#define NAMES(...) _NAMES(COUNT_VARARGS(__VA_ARGS__))

/* Declare a function we are going to mock in the tests to follow.
 * ret - Return type of the function.
 * name - Name of the function.
 * ... - Types of the function parameters.
 * 
 * All functions being mocked in the test must declare the 
 * functions once at the top. For example
 * int foo(int x, float y) -> DECLARE_MOCK_FUNC(int, foo, int, float)
 */
#define DECLARE_MOCK_FUNC(ret, name, ...) 	\
	static ret __##name##_mock_return;		\
	static int __##name##_mock_call_cnt;		\
	static bool __##name##_mock_return_valid = false;\
	static ret __##name##_default_mock(FULL_PARAMS(__VA_ARGS__)) {	\
		assert(__##name##_mock_return_valid == true);	\
		return __##name##_mock_return;			\
	}							\
	static ret (*__##name##_mock)(FULL_PTYPES(__VA_ARGS__)) = &__##name##_default_mock;	\
	ret name(FULL_PARAMS(__VA_ARGS__)) {	\
	  __##name##_mock_call_cnt++;		\
		return __##name##_mock(NAMES(__VA_ARGS__)); \
	}

/* Declare a function returning void
 * name - Name of the function.
 * ... - Parameter type list.
 *
 * Same as DECLARE_MOCK_FUNC. Example:
 * void bar(int x) -> DECLARE_MOCK_VOIDFUNC(bar, int)
 */
#define DECLARE_MOCK_VOIDFUNC(name, ...) 	\
	static int __##name##_mock_call_cnt;		\
	static bool __##name##_mock_return_valid = false;\
	static void __##name##_default_mock(FULL_PARAMS(__VA_ARGS__)) {	\
	}							\
	static void (*__##name##_mock)(FULL_PTYPES(__VA_ARGS__)) = &__##name##_default_mock;	\
	void name(FULL_PARAMS(__VA_ARGS__)) {	\
	  __##name##_mock_call_cnt++;		\
    if (__##name##_mock != NULL) \
		  __##name##_mock(NAMES(__VA_ARGS__)); \
	}

/* Mock the function with a custom implementation.
 * name - Named of the mocked function.
 * func - function pointer of the custom implementation.
 */
#define MOCK(name, func)	\
	__##name##_mock = func

/* Mock the return value of the function.
 * name - Name of the mocked function.
 * ret_val - The value we want the mocked function to return.
 */
#define MOCK_RETURN(name, ret_val) 	\
	do {				\
		__##name##_mock_return_valid = true;	\
		__##name##_mock_return = ret_val;	\
	} while(0)

/* Clean up any state changed by mocking done in previous tests.
 * name - Name of the mocked function.
 */
#define MOCK_END(name)	\
		__##name##_mock_return_valid = false;	\
		__##name##_mock_call_cnt = 0;	\
		__##name##_mock = __##name##_default_mock

#define CALL_COUNT(name) __##name##_mock_call_cnt

/* Define a test case.
 * name - Name of the test case 
 */
#define DEFINE_TEST(name) 	\
	void name##_impl(void);	\
	void name(void) {	\
		name##_impl();	\
		printf("%s: PASSED\n", #name);	\
	}	\
	void name##_impl(void)

#define CALL_TEST(name) \
  name()

#define ASSERT(cond, txt) do { \
  if (!(cond)) { \
    printf("FAIL: %s\n", txt); \
  } \
  assert(cond); \
} while(0)

#define ASSERT_EQ(v1, v2, txt) \
  ASSERT((v1) == (v2), txt)
#define ASSERT_EQ_FLT(v1, v2, txt) \
  ASSERT((v1) < ((v2) + 0.001) && (v1) > ((v2) - 0.001), txt)
#define ASSERT_EQ_STR(v1, v2, txt) \
  ASSERT(strcmp(v1, v2) == 0, txt)

#define ASSERT_NEQ(v1, v2, txt) \
  ASSERT((v1) != (v2), txt)
#define ASSERT_NEQ_FLT(v1, v2, txt) \
  ASSERT((v1) > ((v2) + 0.001) || (v1) < ((v2) - 0.001), txt)
#define ASSERT_NEQ_STR(v1, v2, txt) \
  ASSERT(strcmp(v1, v2) != 0, txt)

#define ASSERT_CALL_COUNT(name, min, max, txt) \
  ASSERT(CALL_COUNT(name) >= min && CALL_COUNT(name) <= max, txt)

#endif
