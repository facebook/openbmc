#include <CLI/CLI.hpp>
#include <openbmc/aries_api.h>
#include <openbmc/pal.h>
#include <syslog.h>
#include <iostream>

#define RT_LOCK "/tmp/pal_rt_lock"
#define MAX_RETRY 10

static void do_margin(int id, std::string& type, std::string& file) {
  int fd_lock = -1, retry = 0;

  while (fd_lock < 0 && retry < MAX_RETRY) 
  {
    if (retry == MAX_RETRY) {
      std::cout<< "Other process are using" << std::endl;
      return;
    } 
    
    fd_lock = pal_lock(RT_LOCK);
    retry++;
    sleep(1);
  }

  AriesMargin(id, type.c_str(), file.c_str());

  pal_unlock(fd_lock);
}

/*static void do_update(const std::string& comp, const std::string& path, bool async, bool json_fmt) {
}*/

int main(int argc, char* argv[]) {
  CLI::App app("Retimer Helper Utility");
  app.failure_message(CLI::FailureMessage::help);

  // Allow flags/options to fallthrough from subcommands.
  app.fallthrough();

  int rt_id;
  std::string margin_type{};
  std::string margin_file{};
  auto margin = app.add_subcommand("margin", "Perform margin test");
  margin->add_option("ID", rt_id, "Retimer ID 0-7")
    ->check(CLI::Range(0, 7))->required();
  margin->add_option("TYPE", margin_type, "Margin test for UpStream or DownStream")
    ->check(CLI::IsMember({"up", "down"}))->required();
  margin->add_option("FILE", margin_file, "The file name of margin test result")->required();
  margin->callback([&]() { do_margin(rt_id, margin_type, margin_file); });

  app.require_subcommand(/* min */ 1, /* max */ 1);

  CLI11_PARSE(app, argc, argv);

  return 0;
}
