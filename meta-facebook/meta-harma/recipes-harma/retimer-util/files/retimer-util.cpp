#include <CLI/CLI.hpp>
#include <openbmc/aries_common.h>
#include <syslog.h>
#include <iostream>
#include <openbmc/misc-utils.h>
#include <cstdlib>

static int polling_stop(void) {
  return std::system("systemctl stop xyz.openbmc_project.hwmontempsensor.service");
}

static int polling_start(void) {
  return std::system("systemctl start xyz.openbmc_project.hwmontempsensor.service");
}

static void do_margin(std::string fru, int id, std::string& type, std::string& file) {

  polling_stop();
  AriesMargin(fru.c_str(), id, type.c_str(), file.c_str());
  polling_start();

}

static void do_print_link (std::string fru, int id) {
  polling_stop();
  AriesPrintState(fru.c_str(), id);
  polling_start();
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
  app.add_option("FRU", fru, "FRU name: mb")
    ->check(CLI::IsMember({"mb"}))->required();

  auto margin = app.add_subcommand("margin", "Perform margin test");
  margin->add_option("ID", rt_id, "Retimer ID 0-1")
    ->check(CLI::Range(0, 1))->required();
  margin->add_option("TYPE", margin_type, "Margin test for UpStream or DownStream")
    ->check(CLI::IsMember({"up", "down"}))->required();
  margin->add_option("FILE", margin_file, "The file name of margin test result")->required();
  margin->callback([&]() { do_margin(fru, rt_id, margin_type, margin_file); });

  auto link = app.add_subcommand("link", "Dump link status");
  link->add_option("ID", rt_id, "Retimer ID 0-1")->check(CLI::Range(0, 1))->required();
  link->callback([&]() { do_print_link(fru, rt_id); });

  app.require_subcommand(/* min */ 1, /* max */ 1);

  CLI11_PARSE(app, argc, argv);

  return 0;
}
