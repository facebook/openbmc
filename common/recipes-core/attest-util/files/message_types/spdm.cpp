#include "spdm.hpp"
#include <stdint.h>
#include <utils.hpp>
#include <unordered_map>
#include <openbmc/pal.h>
#include <openbmc/obmc-mctp.h>

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

  return sub;
}

// SPDM payload must start with 0x05 according to spec.
static bool isPayloadValid(vector<uint8_t> payload) {
  if (payload.size() == 0 || payload[0] != 0x05)
    return false;
  return true;
}

vector<uint8_t> sendSpdmMessage(uint8_t bus, uint8_t eid, vector<uint8_t> &message, bool debugOutput) {
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
  send_spdm_cmd(bus, addr, DEFAULT_EID, eid, &message[0], message.size(), rbuf, &rlen);

  if (debugOutput == true) {
    std::cout << "Raw Response Length: " << std::dec << rlen << std::endl;
    std::cout << "Raw Response: " << std::endl;
    printHexValues(rbuf, rlen);
  }

  for(int index = 0; index < rlen; ++index)
    returnMessage.push_back(rbuf[index]);

  return returnMessage;
}


void SpdmMessage::sendMessage(SubcommandOptions const& opt) {
  string encodedResponse = "DUMMYRESPONSE";
  string errorMessage = "";
  string encodedMessage;
  vector<uint8_t> returnMessage;


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

  // decode from base64.
  vector<uint8_t> message = decodeBase64(encodedMessage);

  if (opt.debugOutput == true) {
    std::cout << "Decoded message is:" << std::endl;
    printHexValues(&(message[0]), message.size());
  }

  // Validate Payload
  if (isPayloadValid(message)) {
    if(opt.debugOutput == true)
      std::cout << "Message is valid SPDM message!" << std::endl;

    // send message over MCTP
    returnMessage = sendSpdmMessage(8, opt.device, message, opt.debugOutput);

    if(returnMessage.size() > 0) {
      // Remove the MCTP header.
      returnMessage.erase(returnMessage.begin(), returnMessage.begin()+3);
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
