#include "spdm.hpp"
#include <stdint.h>
#include <utils.hpp>
#include <unordered_map>

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
      ->required()
      ->check(CLI::ExistingFile);

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

/*
 * Currently just an interface for testing
 * Does not send the message over MCTP.
 */
void SpdmMessage::sendMessage(SubcommandOptions const& opt) {
  string encodedResponse = "DUMMYRESPONSE";
  string errorMessage = "";

  // Read in file. (Existence already checked)
  std::ifstream messageStream(opt.inputFileName);
  std::stringstream buffer;
  buffer << messageStream.rdbuf();
  string encodedMessage = buffer.str();

  // decode from base64.
  vector<uint8_t> message = decodeBase64(encodedMessage);

  // Validate Payload
  if (isPayloadValid(message)) {
    // send message over MCTP

    // Re-encode response to base64

    errorMessage = "Success";
  } else {
    errorMessage = "Payload is not an SPDM message";
  }

  // Handle response
  acceptedOutputs.at(opt.outputType)(encodedResponse, errorMessage);
}
