# The CentOS 10 build hosts use GCC 14, which miscompiles the ancient (2017)
# Python 3.5.3 sources at -O2. The native interpreter builds successfully but
# segfaults at runtime -- the reproducible crash is in the _ctypes module
# (PyCFuncPtr_new, creating the first libffi callback), which is hit as soon as
# anything imports ctypes. python3-setuptools-native's do_compile/do_install
# (`setup.py build` / `setup.py install`) import setuptools -> ctypes and so
# segfault. This was verified locally: a Python 3.5.3 native interpreter built
# with GCC 14 at -O2 crashes on `import ctypes` 20/20 times and on setuptools
# build/install; rebuilding at -O0 (or -O1) makes all of them pass.
#
# Native python is only a build-time tool, so its performance is irrelevant.
# Drop optimisation to side-step the GCC 14 miscompilation (the trailing -O0
# wins over the recipe's -O2 and CPython configure's OPT=-O3).
CFLAGS_append_class-native = " -O0"
