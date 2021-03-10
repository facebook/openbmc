/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <iostream>
#include <memory>
#include "CLI/CLI.hpp"
#include "message_types/spdm.hpp"
#include "utils.hpp"

/*
 * Template for adding new message types. The template will generate a compiler
 * error if the message class being added doesn't have a setupSubcommand and
 * sendMessage method.
 */
template <typename MessageType>
void addNewMessageType(CLI::App& app) {
  auto opts = std::make_shared<typename MessageType::SubcommandOptions>();
  auto subCommand = MessageType::setupSubcommand(app, opts);
  subCommand->callback([opts]() { MessageType::sendMessage(*opts); });
}

int main(int argc, char* argv[]) {
  CLI::App app{"MCTP Attestation Tool"};
  app.require_subcommand(1);

  // Print version info.
  auto versionCallback = [](int dummy) {
    std::cout << VERSION << std::endl;
    // Throw success so parsing stops as soon as this flag is found.
    throw CLI::Success();
  };
  app.add_flag_function("-v,--version", versionCallback, "Get version.");

  // Add supported payloads here.
  addNewMessageType<SpdmMessage>(app);

  // This does the actual parsing and calls the callbacks.
  CLI11_PARSE(app, argc, argv);
}
