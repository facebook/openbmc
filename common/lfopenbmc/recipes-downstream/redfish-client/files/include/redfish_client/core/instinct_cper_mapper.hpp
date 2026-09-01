#pragma once

#include <redfish_client/core/log_entry_mapper.hpp>
#include <redfish_client/core/async_http_client.hpp>
#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/task.hpp>

namespace redfish_client::core {

class InstinctCperMapper : public LogEntryMapper {
public:
  explicit InstinctCperMapper(
    sdbusplus::async::context& ctx,
    std::string host)
    : ctx(ctx), host(std::move(host)) {}

  bool canHandle(
    redfish_binding::LogEntry::LogEntry& entry) const override;

  void map(
    redfish_binding::LogEntry::LogEntry& entry) override;

private:
  sdbusplus::async::context& ctx;
  std::string host;

  struct CPERData
  {
      std::string host;
      std::string source;
      std::string notificationType;
      std::string sectionType;
      std::string vendor;
      std::string oemData;
      std::string additionalDataURI;
  };

  static auto commitCPER(
      sdbusplus::async::context& ctx,
      CPERData data)
      -> sdbusplus::async::task<void>;
};

} // redfish_client::core