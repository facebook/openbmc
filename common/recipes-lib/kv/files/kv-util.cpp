#include <iostream>
#include <string>
#include "kv.hpp"

void usage(const char* exe_name) {
  std::cout << exe_name << " <op> [args]+\n";
  std::cout
      << "    get:\n"
         "        get <key> <type|>*\n"
         "    set:\n"
         "        set <key> <value> <type|>*\n"
         "    del:\n"
         "        del <key> <type|>*\n"
         "\n"
         "    valid types:\n"
         "        persistent - use the persistent kv store.\n"
         "        create - create the key if it does not already exist.\n";
}

/* Argument positions */
static constexpr auto pos_exe = 0;
static constexpr auto pos_cmd = 1;
static constexpr auto pos_key = 2;
static constexpr auto pos_get_flag = 3;
static constexpr auto pos_set_value = 3;
static constexpr auto pos_set_flag = 4;

/** Parse a flag parameter to determine the region. */
kv::region region(const char* flag) {
  if (std::string::npos != std::string(flag).find("persistent")) {
    return kv::region::persist;
  }

  return kv::region::temp;
}

/** Parse a flag parameter to determine if create is required. */
bool require_create(const char* flag) {
  if (std::string::npos != std::string(flag).find("create")) {
    return true;
  }

  return false;
}

/** Handle 'get' subcommand. */
int cmd_get(int argc, const char** argv) {
  if (argc <= pos_key) {
    // Not enough args.
    usage(argv[pos_exe]);
    return 1;
  }

  // Parse flags
  auto r = region(argc <= pos_get_flag ? "" : argv[pos_get_flag]);

  // Read the kv and display it.
  try {
    std::cout << kv::get(argv[pos_key], r);
  } catch (std::exception&) {
    // Eat any exception and simply return a bad rc.
    return 1;
  }
  return 0;
}

int cmd_del(int argc, const char** argv) {
  if (argc <= pos_key) {
    // Not enough args.
    usage(argv[pos_exe]);
    return 1;
  }

  // Parse flags
  auto r = region(argc <= pos_get_flag ? "" : argv[pos_get_flag]);

  // delete kv
  try {
    kv::del(argv[pos_key], r);
  } catch (kv::key_does_not_exist&) {
    std::cerr << argv[pos_key] << " does not exist.\n";
    return 1;
  } catch (std::exception& e) {
    std::cerr << argv[pos_key] << " Error: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}

/** Handle 'set' subcommand. */
int cmd_set(int argc, const char** argv) {
  if (argc < pos_set_value) {
    // Not enough argc.
    usage(argv[pos_exe]);
    return 1;
  }

  // Parse flags.
  auto r = region(argc <= pos_set_flag ? "" : argv[pos_set_flag]);
  auto c = require_create(argc <= pos_set_flag ? "" : argv[pos_set_flag]);

  // Set the kv.
  kv::set(argv[pos_key], argv[pos_set_value], r, c);
  return 0;
}

int main(int argc, const char** argv) {
  do {
    // Got to at least have a sub-command.
    if (argc <= pos_cmd) {
      break;
    }

    // Parse sub-command and call handler.
    if (std::string("get") == argv[pos_cmd]) {
      return cmd_get(argc, argv);
    } else if (std::string("del") == argv[pos_cmd]) {
      return cmd_del(argc, argv);
    } else if (std::string("set") == argv[pos_cmd]) {
      return cmd_set(argc, argv);
    } else if (std::string("help") == argv[pos_cmd]) {
      usage(argv[pos_exe]);
      return 0;
    }

  } while (false);

  // Something didn't go right in sub-command parsing, display usage and fail.
  usage(argv[pos_exe]);
  return 1;
}
