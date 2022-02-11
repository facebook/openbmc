
# at-scale-debug

  Intel(R) At-Scale Debug (ASD) solution provides debug access through the BMC
  to the CPU/PCH JTAG chain(s) and target pins in order to facilitate debug.

## Development Pre-commit Checklist

  To keep the code tidy, or moving in that direction,
  please follow these steps for each commit.

* **Write unit tests for all new code**

  Please ensure dependencies for the unit of code under test are mocked out
  to reduce the unit test complexity and maintain a fast set of tests. Also
  limit the scope of the unit test as much as possible.

* **Ensure all unit tests pass**

  Each c file has it's own set of unit tests. You can run them individually
  if you like, or run ctest to run them all at once.

  An example to run them all at once:

  `(cd [build-path]/tmp/work/[platform]/at-scale-debug/1.0-r0/build/tests/; ctest)`

* **Code coverage tools can be used to verify all code is tested**

  To build and generate a code coverage report, run:
  `make && make test_coverage`
  Then open the index.html in the test_coverage directory.

* **Run clang-format to fix code style**

  Note the '.clang-format' config file is used when providing the '-style=file'
  argument.

  `clang-format -style=file -i <filename>`

* **Run valgrind**

  Since all code is unit tested, its helpful run Valgrind on the unit tests to
  screen for potential issues.

## Running Jtag Test on BMC

  asd & jtag_test can be found in /usr/bin on the BMC.
  From the BMC console, type `jtag_test --help` or 'asd --help'
  for instructions.
