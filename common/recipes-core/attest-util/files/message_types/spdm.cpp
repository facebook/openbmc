#include "spdm.hpp"
#include <stdint.h>
#include <utils.hpp>
#include <unordered_map>
#include <openbmc/pal.h>
#include <openbmc/obmc-mctp.h>
#include <chrono>
#include <ctime>

#define DEFAULT_EID 0x8
#define MAX_PAYLOAD_SIZE 4099 // including Message Header and body

/**
 *  These are the supported output types of the responses.
 *  They consist of the lowercase name of the output type and the
 *  function used to output the desired response format.
 *  CLI11 automatically checks to make sure the option passed in
 *  is present in this list and handles the error if it isn't.
 */
typedef void (*OutputFunctionType)(std::string, std::string);
static const std::unordered_map<std::string, OutputFunctionType>
    acceptedOutputs = {
        {"raw", handleResponseRaw},
        {"json", handleResponseJson}};

CLI::App* SpdmMessage::setupSubcommand(
    CLI::App& app,
    shared_ptr<SubcommandOptions> opt) {
  // Add the SPDM subcommand
  auto sub = app.add_subcommand(
      "spdm",
      "Sends an spdm message to the designated device and returns the response.");

  // Bus on which the device exists
  sub->add_option("--bus", opt->bus, "Bus ID");

  // Slave address of device
  sub->add_option("-a,--addr", opt->addr, "I2C Slave address of the device");

  // EID of the target device.
  sub->add_option("-e,--eid", opt->device, "Endpoint ID")->required();

  // The SPDM message is read in from a file, CLI11 also handles checking to
  // make sure the file exists and automatically handles reporting an error.
  sub->add_option("-f,--file", opt->inputFileName, "File name")
      ->check(CLI::ExistingFile);

  // The SPDM message can also be read in from the command line via one of the arguments.
  sub->add_option("-m,--message", opt->inputString, "Raw SPDM message encoded as a base64 string");

  // Print verbose debug info to stdout when applicable.
  sub->add_flag("-v, --verbose", opt->debugOutput, "Enable verbose output");

  // The user can specify the output type. The type is automatically checked
  // to make sure it is an acceptable type and errors are reported accordingly.
  opt->outputType = "json";
  sub->add_option("-o,--output", opt->outputType, "Output type")
      ->check(CLI::IsMember(acceptedOutputs));

  // Allow benchmarking of SPDM message by repeatedly sending message.
  // Message will be sent N times.
  opt->benchmarkCount = 0;
  sub->add_option("-b,--benchmark", opt->benchmarkCount, "Benchmark the SPDM message by sending N times and report average time");

  return sub;
}

// SPDM payload must start with 0x05 according to spec.
static bool isPayloadValid(vector<uint8_t> payload) {
  if (payload.size() == 0 || payload[0] != 0x05)
    return false;
  return true;
}

void benchmarkSpdmMessage(uint8_t bus, uint16_t slv_addr, uint8_t eid, vector<uint8_t> &message, uint8_t runCount) {
  uint8_t rbuf[MAX_PAYLOAD_SIZE] = {0};
  uint16_t addr = 0;
  int rlen = 0;

  std::chrono::milliseconds wallTimeSum = std::chrono::milliseconds(0);
  std::clock_t cpuTimeSum = 0;

  pal_get_bmc_ipmb_slave_addr(&addr, bus);

  std::cout << "Starting spdm round trip benchmark -- Running " << +runCount << " times..." << std::endl;

  for (size_t index = 0; index < runCount; ++index) {
    auto wallStart = std::chrono::high_resolution_clock::now();
    std::clock_t cpuStart = std::clock();

    send_spdm_cmd(bus, eid, &message[0], message.size(), rbuf, &rlen);

    auto wallEnd = std::chrono::high_resolution_clock::now();
    std::clock_t cpuEnd = std::clock();

    auto wallClockInterval = std::chrono::duration_cast<std::chrono::milliseconds>(wallEnd - wallStart);
    std::clock_t cpuClockInterval = cpuEnd - cpuStart;
    wallTimeSum += wallClockInterval;
    cpuTimeSum += cpuClockInterval;

    std::cout << "Run : " << index << std::endl;
    std::cout << "\tWall-clock time: " << wallClockInterval.count() << std::endl;
    std::cout << "\tCPU time: " << ((float)cpuClockInterval) / (CLOCKS_PER_SEC / 1000) << std::endl;
  }

  std::cout << std::endl;

  wallTimeSum /= runCount;
  double cpuAverageMilli = ((float)cpuTimeSum / (float)runCount) / (CLOCKS_PER_SEC / 1000);

  std::cout << "Average wall clock time is: " << wallTimeSum.count() << " Milliseconds." << std::endl;
  std::cout << "Average CPU time is: " << cpuAverageMilli << " Milliseconds." << std::endl;
  return;
}

vector<uint8_t> sendSpdmMessage(uint8_t bus, uint16_t slv_addr, uint8_t eid, vector<uint8_t> &message, bool debugOutput) {
  uint8_t rbuf[MAX_PAYLOAD_SIZE] = {0};
  uint16_t addr = 0;
  vector<uint8_t> returnMessage;
  int rlen = 0;

  if(debugOutput == true)
    std::cout << "Sending message on bus: "
              << +bus
              << " to eid: "
              << +eid
              << std::endl;

  pal_get_bmc_ipmb_slave_addr(&addr, bus);
  send_spdm_cmd(bus, eid, &message[0], message.size(), rbuf, &rlen);

  if (debugOutput == true) {
    std::cout << "Raw Response Length: " << std::dec << rlen << std::endl;
    std::cout << "Raw Response: " << std::endl;
    printHexValues(rbuf, rlen);
  }

  for(decltype(rlen) index = 0; index < rlen; ++index)
    returnMessage.push_back(rbuf[index]);

  return returnMessage;
}


void SpdmMessage::sendMessage(SubcommandOptions const& opt) {
  string errorMessage = "";
  string encodedMessage;
  vector<uint8_t> returnMessage;
  vector<string> messages;
  const string delimiter = ",";

  if (opt.inputFileName.length() != 0) {
    // Read in file. (Existence already checked)
    std::ifstream messageStream(opt.inputFileName);
    std::stringstream buffer;

    buffer << messageStream.rdbuf();
    encodedMessage = buffer.str();

  } else if (opt.inputString.length() != 0) {
    encodedMessage = opt.inputString;
  } else {
    throw CLI::CallForHelp();
  }

  messages = splitMessage(encodedMessage, delimiter);

  vector<vector<uint8_t>> decodedMessages;
  for (size_t index = 0; index < messages.size(); ++index) {
    // decode from base64.
    decodedMessages.push_back(decodeBase64(messages[index]));
  }

  if(opt.benchmarkCount > 0) {
    if(decodedMessages.size() != 1)
      throw CLI::ValidationError("Benchmark can only be run with a single spd message.");

    benchmarkSpdmMessage(opt.bus, opt.addr, opt.device, decodedMessages[0], opt.benchmarkCount);
    return;
  }

  for (size_t index = 0; index < messages.size(); ++index) {
    string encodedResponse = "DUMMYRESPONSE";
    vector<uint8_t> message = decodedMessages[index];

    if (opt.debugOutput == true)
      std::cout << "Attempting to send SPDM message: " << index << std::endl;

    if (opt.debugOutput == true) {
      std::cout << "Decoded message is:" << std::endl;
      printHexValues(&(message[0]), message.size());
    }

    // Validate Payload
    if (isPayloadValid(message)) {
      if(opt.debugOutput == true)
        std::cout << "Message is valid SPDM message!" << std::endl;

      // send message over MCTP
      returnMessage = sendSpdmMessage(opt.bus, opt.addr, opt.device, message, opt.debugOutput);

      if(returnMessage.size() > 0) {
        // Re-encode response to base64
        encodedResponse = encodeBase64(returnMessage);
        errorMessage = "Success";
      } else {
        errorMessage = "Empty Response";
      }
    } else {
      errorMessage = "Payload is not an SPDM message";
    }

    // Handle response
    acceptedOutputs.at(opt.outputType)(encodedResponse, errorMessage);
  }
}
