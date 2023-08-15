#include <CLI/CLI.hpp>
#include <openbmc/aries_common.h>
#include <openbmc/pal.h>
#include <syslog.h>
#include <iostream>

#define HEARTBEAT 0x1
#define CODE_LOAD 0x2

static void do_margin(std::string fru, int id, std::string& type, std::string& file) {
  int lock = -1;

  if(fru == "hgx") {
    AriesMargin(fru.c_str(), id, type.c_str(), file.c_str());
    return;
  }

  if ((lock = mb_retimer_lock()) < 0) {
    std::cout<< "Cannot get retimer lock" << std::endl;
    return;
  }
  AriesMargin(fru.c_str(), id, type.c_str(), file.c_str());
  mb_retimer_unlock(lock);
}

static void do_print_link (std::string fru, int id) {
  int lock = -1;

  if(fru == "hgx") {
    AriesPrintState(fru.c_str(), id);
    return;
  }

  if ((lock = mb_retimer_lock()) < 0) {
    std::cout<< "Cannot get retimer lock" << std::endl;
    return;
  }
  AriesPrintState(fru.c_str(), id);
  mb_retimer_unlock(lock);
}

static void do_print_health (std::string fru, int id) {
  int lock = -1;
  uint8_t health = 0;

  if(fru == "hgx") {
    AriesGetHealth(fru.c_str(), id, &health);
  }
  else {
    if ((lock = mb_retimer_lock()) < 0) {
      std::cout<< "Cannot get retimer lock" << std::endl;
      return;
    }
    AriesGetHealth(fru.c_str(), id, &health);
    mb_retimer_unlock(lock);
  }

  std::cout<< "heartbeat: "
           << ((health & HEARTBEAT)?"good":"bad") << std::endl;

  std::cout<< "code load status: "
           << ((health & CODE_LOAD)?"good":"bad") << std::endl;
}

int main(int argc, char* argv[]) {
  int rt_id;
  std::string fru{};
  std::string margin_type{};
  std::string margin_file{};
  std::string link_file{};

  CLI::App app("Retimer Helper Utility");
  app.failure_message(CLI::FailureMessage::help);

  // Allow flags/options to fallthrough from subcommands.
  app.fallthrough();
  app.add_option("FRU", fru, "FRU name: mb or hgx")
    ->check(CLI::IsMember({"mb", "hgx"}))->required();

  auto margin = app.add_subcommand("margin", "Perform margin test");
  margin->add_option("ID", rt_id, "Retimer ID 0-7")
    ->check(CLI::Range(0, 7))->required();
  margin->add_option("TYPE", margin_type, "Margin test for UpStream or DownStream")
    ->check(CLI::IsMember({"up", "down"}))->required();
  margin->add_option("FILE", margin_file, "The file name of margin test result")->required();
  margin->callback([&]() { do_margin(fru, rt_id, margin_type, margin_file); });

  auto link = app.add_subcommand("link", "Dump link status");
  link->add_option("ID", rt_id, "Retimer ID 0-7")->check(CLI::Range(0, 7))->required();
  link->callback([&]() { do_print_link(fru, rt_id); });

  auto health = app.add_subcommand("health", "Get retimer healthy");
  health->add_option("health", rt_id, "Retimer ID 0-7")->check(CLI::Range(0, 7))->required();
  health->callback([&]() { do_print_health(fru, rt_id); });

  app.require_subcommand(/* min */ 1, /* max */ 1);

  CLI11_PARSE(app, argc, argv);

  return 0;
}
