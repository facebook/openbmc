import sys
import types


# Workaround because pal and sdr are unavailable in some unit test envs
#
# Unfortuntaly, instead of a dependency injection, we currently just hardcode
# dependencies. Thus to unit-test we use monkey patching. And at the same
# we have a bunch of platform-specific modules, which in some environments
# even do not exists. So to make unit-tests work in those environments we
# softly try to import the required module, and if it is non-importable then
# we just set a dummy placeholder instead (which will be monkey-patched in
# an unit-test).
#
# We cannot just always use dummy placeholders for unit-tests instead, because
# in some cases it creates a clash between a real module and a dummy one (and
# unit-tests fails with obscure reasons).
#
# TODO: remove this
def force_import(module_path: str):
    import importlib

    try:
        sys.modules[module_path] = importlib.import_module(module_path)
    except Exception:
        sys.modules[module_path] = types.ModuleType(module_path)


force_import("pal")
force_import("sdr")
force_import("sensors")
force_import("aggregate_sensor")
