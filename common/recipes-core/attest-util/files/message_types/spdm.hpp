#pragma once

#include <memory>
#include <string>
#include "CLI/CLI.hpp"
#include "utils.hpp"

using std::shared_ptr;

class SpdmMessage {
 public:
  // These are the current input parameters for sending SPDM messages.
  struct SubcommandOptions {
    uint8_t device;

    // The bus on which this device is.
    // XXX this might be made required. The default of 8 is
    // to keep some backward compatibility till then.
    uint8_t bus = 8;
    // The I2C slave address on which this device is.
    // XXX this might be made required. The defult of 0x64 is
    // to keep some backward compatibility till then.
    uint16_t addr = 0x64;
    uint8_t benchmarkCount = 0;
    std::string inputFileName = "";
    std::string inputString = "";
    std::string outputType;
    bool debugOutput = false;;
  };

  /**
   * Setup the accepted subcommand options for sending SPDM messages.
   * New options for sending SPDM payloads should be added here.
   */
  static CLI::App* setupSubcommand(
      CLI::App& app,
      shared_ptr<SubcommandOptions> opt);

  /**
   * This callback function sends the SPDM payload over MCTP. Thos callback
   * is called from within the parse function for the command line
   * interface.
   */
  static void sendMessage(SubcommandOptions const& opt);
};
