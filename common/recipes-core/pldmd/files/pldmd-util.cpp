#include <cstdio>
#include <vector>
#include <string>
#include "CLI/CLI.hpp"

#include <libpldm-oem/pldm.h>

std::ostream& operator<<(std::ostream& os, uint8_t b) {
  os << "0x" << std::hex << std::setw(2) << std::setfill('0') << +b;
  return os;
}

static void
show_request_info(uint8_t bus, int eid, const std::vector<uint8_t>& data)
{
  std::cout << "Request :"
    << "\n  Bus = " << int(bus)
    << "\n  Eid = " << int(eid)
    << "\n  Data = ";
  for (auto&d : data)
    std::cout << d << ' ';
  std::cout << std::endl;
}

static void
show_response_info(int ret, uint8_t* resp)
{
  std::cout << "Response: \n"
            << "  Return Code: " << ret << std::endl;
  if (ret == 0) {
    std::cout << "  PLDM Request Bit     : " << uint8_t(resp[0]>>7)
      << "\n  PLDM Instance ID     : " << uint8_t(resp[0]&0x3F)
      << "\n  PLDM Type            : " << uint8_t(resp[1]&0x3F)
      << "\n  PLDM Command         : " << resp[2]
      << "\n  PLDM Completion Code : " << resp[3]
      << std::endl;
  }
}

static void do_raw(uint8_t bus, int eid, std::vector<uint8_t>& data) {
  show_request_info(bus, eid, data);
  // bit 4:0 instance_id = 0,
  // bit5 - rsvd, bit6 - dgram (false) bit7 - req (true)
  data.insert(data.begin(), 0x80);

  uint8_t* resp = nullptr;
  size_t resp_len = 0;
  int ret = oem_pldm_send_recv(bus, eid,
                          data.data(), data.size(),
                          &resp, &resp_len);

  show_response_info(ret, resp);
  if (ret == 0) {
    std::cout << "  PLDM Data : ";
    for (size_t i = 4; i < resp_len; ++i)
      std::cout << resp[i] << ' ';
    std::cout << std::endl;
    if (resp != nullptr)
      free(resp);
  } else {
    std::cerr << "Failed code: " << ret << std::endl;
    exit(1);
  }
}

static void do_get_ver(uint8_t bus, int eid) {
  std::vector<uint8_t> data{0x05, 0x02};
  show_request_info(bus, eid, data);

  // bit 4:0 instance_id = 0,
  // bit5 - rsvd, bit6 - dgram (false) bit7 - req (true)
  data.insert(data.begin(), 0x80);
  uint8_t* resp = nullptr;
  size_t resp_len = 0;
  int ret = oem_pldm_send_recv(bus, eid,
                          data.data(), data.size(),
                          &resp, &resp_len);

  show_response_info(ret, resp);
  if (ret == 0 && resp[3] == 0) {
    std::cout << "  PLDM Data : ";
    for (size_t i = 4; i < resp_len; ++i)
      std::cout << resp[i] << ' ';
    std::cout << std::endl;
    if (resp)
      free(resp);
  } else {
    std::cerr << "Failed code: " << ret << " resp[3]: " << resp[3] << std::endl;
    exit(1);
  }
}

static void do_get_effecter(uint8_t bus, int eid, std::vector<uint8_t>& payload) {

  std::vector<uint8_t> data{};

  // 0:5 pldm_type = 0x2 (PLDM for Platform
  // Monitoring and Control) (6:7 hdr-version  0)
  data.push_back(0x02);
  // pldm_command 0x3A = GetStateEffecterStates
  data.push_back(0x3A);
  // Add data
  data.insert(data.end(), payload.begin(), payload.end());

  show_request_info(bus, eid, data);

  // bit 4:0 instance_id = 0,
  // bit5 - rsvd, bit6 - dgram (false) bit7 - req (true)
  data.insert(data.begin(), 0x80);

  uint8_t* resp = nullptr;
  size_t resp_len = 0;
  int ret = oem_pldm_send_recv(bus, eid,
                          data.data(), data.size(),
                          &resp, &resp_len);

  show_response_info(ret, resp);
  if (ret == 0 && resp[3] == 0) {
    size_t numEffectors = (size_t)resp[4];
    std::cout << "  PLDM Data :\n"
       << "    Effecter Count : " << numEffectors << std::endl;
    for (size_t i = 0; i < numEffectors; ++i) {
      std::cout << "    " << i << ". OperationalState : " << int(resp[5+i*3])
        << "\n    " << i << ". pendingState     : " << int(resp[6+i*3])
        << "\n    " << i << ". presentState     : " << int(resp[7+i*3])
        << std::endl;
    }
  } else {
    std::cerr << "Failed code: " << ret << " resp[4]: " << resp[4] << std::endl;
    exit(1);
  }
}

int main (int argc, char** argv)
{
  // init CLI app
  CLI::App app("\nPLDM daemon util. (Over MCTP)\n");
  app.failure_message(CLI::FailureMessage::help);

  // Allow flags/options to fallthrough from subcommands.
  app.fallthrough();

  // init options
  std::string optionDescription;

  // <bus number> option
  uint8_t bus;
  optionDescription = "(required) Setting Bus number.";
  app.add_option("-b, --bus", bus, optionDescription)->required();

  // <eid> option
  int eid = 0;
  optionDescription = "(default:0) Setting EID.";
  app.add_option("-e, --eid", eid, optionDescription);

  // <raw> option
  std::vector<uint8_t> data = {};
  optionDescription = "PLDM header without first byte (requset & instance id)";
  auto opt_raw = app.add_subcommand("raw", "Execute raw PLDM command");
  opt_raw->add_option("data", data, optionDescription)->required();
  opt_raw->callback([&]() {do_raw(bus, eid, data);});

  // <get-ver> option
  optionDescription = "Get Firmware Parameter ( Type:0x05 Cmd:0x02 )";
  auto opt_get_ver = app.add_subcommand("get-ver", optionDescription);
  opt_get_ver->callback([&]() { do_get_ver(bus, eid); });

  // <get-effector> option
  optionDescription = "Get StateEffecter States ( Type:0x02 Cmd:0x3A )";
  auto opt_get_effecterstat = app.add_subcommand("get-effecter", optionDescription);
  opt_get_effecterstat->add_option("data", data, "effector req value")->expected(2)->required();
  opt_get_effecterstat->callback([&]() {do_get_effecter(bus, eid, data);});

  app.require_subcommand(/* min */ 1, /* max */ 1);

  // parse and execute
  CLI11_PARSE(app, argc, argv);

  return 0;
}
