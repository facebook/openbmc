# CMock - A Simple Unit Testing Framework for C

This simple header-only framework allows you to create simple
unit tests for C programs where you can mock external C functions
provided by external libraries.

Mocking internal functions would need some refactoring (See later)
Mocking static functions are not possible.

## Defining a Mock

Assume you have two external functions foo and bar:
```
int foo(int x);
void bar(float y, const char *z);
```
The first thing we need to do in one of our test sources is to define
the mock functions and declare their use in the other sources.

So right after your includes (which actually declare foo and bar),
you would use `DEFINE_MOCK_FUNC` and `DEFINE_MOCK_VOIDFUNC`.
test-x.c:
```
#include "foo.h"
#include "bar.h"

DEFINE_MOCK_FUNC(int, foo, int);
DEFINE_MOCK_VOIDFUNC(bar, float, const char*);
```

NOTE: `DEFINE_MOCK_FUNC` and `DEFINE_MOCK_VOIDFUNC`
defines the mocked functions `foo` and `bar`. This implicitly assumes
that you are not linking in the library which defines these methods.

Note that if another test source `test-y.c` needs to use these mocks,
those sources need to declare them
test-y.c:
```
#include "foo.h"
#include "bar.h"

DECLARE_MOCK_FUNC(int, foo, int);
DECLARE_MOCK_VOIDFUNC(bar, float, const char*);
```

## Defining a Test

`DEFINE_TEST` is what you would use to define the test. It just creates
a function for you but does some set up which you can call in later.

This is a simple hello-world Test which always fails.
```
DEFINE_TEST(hello_world) {
  ASSERT(false, "Hello World");
}
```

Now to call that tests from the test executable's `main`:
```
int main(void)
{
  CALL_TESTS();
  return 0;
}

```
Executing and running will have this:
```
FAIL: Hello World
a.out: test.c:4: hello_world_impl: Assertion `0' failed.
Aborted (core dumped)
```
For the sake of brevity, the definition of main() with the calls to
`CALL_TEST` will be excluded in future sections. But note that it is
needed. If you are wondering why your tests somehow magically pass when
it should not, this is probably it.

## Assert Helpers

There are some simple asserts helper macros provided for use:
| Prototype | Description |
|-----------|-------------|
| `ASSERT(cond, txt)` | If condition is false, it will print `FAIL: txt` and exit |
| `ASSERT_EQ(a, b, txt)` | Asserts `a == b,` else prints fail text as above |
| `ASSERT_NEQ(a, b, txt)` | Asserts `a != b`, else prints fail text |
| `ASSERT_EQ_FLT(a, b, txt)` | `ASSERT_EQ` for floating point numbers |
| `ASSERT_NEQ_FLT(a, b, txt)` | `ASSERT_NEQ` for floating point numbers |
| `ASSERT_EQ_STR(a, b, txt)` | `ASSERT_EQ` for C strings |
| `ASSERT_NEQ_STR(a, b, txt)` | `ASSERT_NEQ` for C strings |
| `ASSERT_CALL_COUNT(name, min, max, txt)` | Asserts that mocked function `name` is called at least `min` times and at most `max` times (See mocking) |

## Simple mocks of functions

Say, we want to test foo2() indeed doubles the return value of foo().
```
int foo2(int v)
{
  return foo(v) * 2;
}
```
This is the scenario where we would want to mock `foo()`. Start with the usual
declarations:
```
#include <foo2.h> // int foo2(int)

DECLARE_MOCK_FUNC(int, foo, int);
```

Now for the test:
```
DEFINE_TEST(foo2_test) {
  MOCK_RETURN(foo, 4);
  ASSERT_EQ(foo2(4), 8, "foo2 not doubling 4");
  ASSERT_CALL_COUNT(foo, 1, 1, "foo called exactly once");
}
```
There are a few things happening here.
1. First we call `MOCK_RETURN(foo, 4)` which creates a mock for the function `foo` which just returns `4`.
2. With the mock in place, we check if `foo2` does the right thing. We use `ASSERT_EQ` to check the equality of the return value of `foo2(4)` with `8`. `foo2` would have called our mocked function `foo` which should have returned 4 and our test theoretically should have passed.
3. Ensure that `foo` was indeed called using `ASSERT_CALL_COUNT`. In this case we expect the call to be exactly once, but we can specify a inclusive-range (low and high range numbers are included in the range).

## Mocking a more complex function

`MOCK_RETURN` might be very simple, but sometimes our functions are more complicated. You either want to assert on each passed parameter, or you want to return by reference or you want the mock to create some side-effect. In these cases, you should use the more generic `MOCK` which take in a function pointer.

Lets look at testing `bar2` which calls bar only on even numbers:
```
void bar2(int x)
{
  if (x % 2 == 0)
    bar((float)x, "testing");
}
```

Now the test for this:
```
DEFINE_TEST(bar2) {
  // Yes, C allows you to define a function within a function :-)
  void bar_mock(float x, const char *v) {
    ASSERT_EQ_STR(v, "testing", "v is not testing");
    ASSERT((int)x % 2 == 0, "Passed val is not /2");
  }
  MOCK(bar, bar_mock);
  bar2(1);
  bar2(2);
  bar2(3);
  bar2(4);
  ASSERT_CALL_COUNT(bar, 2, 2, "Called for odd numbers");
}
```
1. Since `bar` does not return a value, we can create a full mock of it. This is defining the mock function `bar_mock` within the test code for `bar2`. Obviously, this should match the actual prototype of `bar`.
  a. Since we expect `bar2` to always pass in the const string "testing", check for it.
  b. Since we expect `bar2` to pass only even numbers to `bar`, check if thats the case.
2. Now that the mock is defined, tell the framework that it is a mock for `bar`. This is `MOCK(bar, bar_mock)`.
3. This is when we make 4 calls.
4. Finally we do our asserts on how many times we expect `bar` to be called.

# Mocking internal functions

Even though this is not formally supported, there are somethings we can do to organize the code to allow this.
1. No one said we want to create 1 test executable! We can always build multiple executables and have one shell-script which calls all of them.
2. Refactor/Reorganize the code to ensure that the functions we want to test (and all its dependencies) are in different source files than the function we want to mock. This allows us to pick-and-choose sources to build with the test source to form our test executables.

That's a lot of words. Lets start with two examples we considered above. Only change is the functions `foo` and `bar` are internal functions.

Mocked functions:
```
int foo(int);
void bar(float, const char*);
```

Functions under test:
```
int foo2(int);
void bar2(int);
```

In order for us to pull this off, we need to reorg into different source files that way the mocked functions and under-test functions are not in the same source files.
|foobar.c | Defines foo() and bar() |
|foobar2.c | Defines foo2() and bar2 |
|test_foobar2.c | Defines our tests as above which has the main() |

Ensure that we build `test_foobar2.c` and `foobar2.c` together into `test_foobar2`. Since we sometimes mock some functions, and test others, it is completely possible that not all tests are workable and hence we might need different source files.
