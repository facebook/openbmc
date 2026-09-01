#include <phosphor-logging/commit.hpp>
#include <phosphor-logging/lg2.hpp>
#include <redfish_client/core/instinct_cper_mapper.hpp>
#include <redfish_client/core/log_entry_mapper_utils.hpp>
#include <sdbusplus/async/timer.hpp>
#include <xyz/openbmc_project/Logging/CPER/Types/common.hpp>
#include <xyz/openbmc_project/Logging/Extension/CPER/Processed/common.hpp>
#include <xyz/openbmc_project/Logging/Extension/CPER/Raw/common.hpp>
#include <xyz/openbmc_project/State/CPER/event.hpp>

namespace redfish_client::core
{

namespace
{
namespace CPER = sdbusplus::error::xyz::openbmc_project::state::CPER;
namespace CPERExtension =
    sdbusplus::common::xyz::openbmc_project::logging::extension::cper;
namespace CPERTypes = sdbusplus::common::xyz::openbmc_project::logging::cper;

constexpr auto unknownSource = "Unknown Source";

std::string getSource(auto& maybeLinks)
{
    if (!maybeLinks.hasValue())
    {
        return unknownSource;
    }

    auto& origin = maybeLinks.value().getOriginOfCondition();

    if (origin.hasValue() && origin.value().getOdataId().hasValue())
    {
        return origin.value().getOdataId().value();
    }

    return unknownSource;
}

std::string getNotificationType(auto& maybeCPER)
{
    if (!maybeCPER.hasValue())
    {
        return {};
    }

    if (!maybeCPER.value().getNotificationType().hasValue())
    {
        return {};
    }

    return maybeCPER.value().getNotificationType().value();
}

std::string getSectionType(auto& maybeCPER)
{
    if (!maybeCPER.hasValue())
    {
        return {};
    }

    if (!maybeCPER.value().getSectionType().hasValue())
    {
        return {};
    }

    return maybeCPER.value().getSectionType().value();
}

std::pair<std::string, std::string> getOemVendorAndData(auto& maybeCPER)
{
    if (!maybeCPER.hasValue())
    {
        return {};
    }

    if (!maybeCPER.value().getOem().hasValue())
    {
        return {};
    }

    const nlohmann::json oemJson =
        nlohmann::json(maybeCPER.value().getOem().value());

    if (!oemJson.is_object() || oemJson.empty())
    {
        return {};
    }

    const auto& vendorEntry = oemJson.items().begin();

    const std::string vendor = vendorEntry.key();
    const std::string data = vendorEntry.value().dump();

    return {vendor, data};
}

} // anonymous namespace

bool InstinctCperMapper::canHandle(
    redfish_binding::LogEntry::LogEntry& entry) const
{
    return entry.getCPER().hasValue();
}

void InstinctCperMapper::map(redfish_binding::LogEntry::LogEntry& entry)
{
    auto& maybeCPER = entry.getCPER();

    if (!maybeCPER.hasValue())
    {
        return;
    }

    CPERData data{};

    data.host = host;

    auto& maybeLinks = entry.getLinks();
    data.source = getSource(maybeLinks);
    data.notificationType = getNotificationType(maybeCPER);
    data.sectionType = getSectionType(maybeCPER);

    auto oemData = extractOemData(entry);
    data.vendor = oemData->vendor;
    data.oemData = oemData->jsonData;

    auto& maybeURI = entry.getAdditionalDataURI();
    if (maybeURI.hasValue())
    {
        data.additionalDataURI = maybeURI.value();
    }

    // Hand off all processing and committing to async task
    ctx.spawn(commitCPER(ctx, std::move(data)));
}

auto InstinctCperMapper::commitCPER(sdbusplus::async::context& ctx,
                                    CPERData data)
    -> sdbusplus::async::task<void>
{
    std::optional<std::vector<uint8_t>> maybeBinary;
    if (!data.additionalDataURI.empty())
    {
        constexpr size_t maxRetries = 3;
        constexpr size_t retryIntervalMs = 1000;

        const std::string fullUrl =
            std::format("http://{}{}", data.host, data.additionalDataURI);

        auto httpHandle = std::make_unique<AsyncHttpHandle>(fullUrl);

        for (size_t i = 0; i < maxRetries; ++i)
        {
            if (i != 0)
            {
                co_await sdbusplus::async::sleep_for(
                    ctx, std::chrono::milliseconds(retryIntervalMs));
            }

            auto response = co_await httpHandle->tryGet(ctx);

            if (!response.has_value())
            {
                continue;
            }

            if (response->code != 200)
            {
                continue;
            }

            maybeBinary = std::vector<uint8_t>(response->body.begin(),
                                               response->body.end());
            break;
        }

        if (!maybeBinary.has_value())
        {
            lg2::warning("Exhausted retries fetching CPER binary from {URI}",
                         "URI", fullUrl.c_str());
        }
    }

    // Build CPERProcessed properties
    CPERExtension::Processed::properties_t cperProperties{};
    cperProperties.diagnostic_data_type = CPERTypes::Types::ContentType::CPER;

    if (!data.notificationType.empty())
    {
        cperProperties.notification_type = data.notificationType;
    }

    if (!data.sectionType.empty())
    {
        cperProperties.section_type = data.sectionType;
    }

    if (!data.vendor.empty() && !data.oemData.empty())
    {
        cperProperties.oem = {
            {data.vendor, data.oemData},
        };
    }

    // Build and extend CPER event
    auto cperEvent = CPER::GenericCPERFault("SOURCE", data.source, "CPER", "");
    cperEvent.extend(cperProperties);

    // Attach raw binary if successfully fetched
    if (maybeBinary.has_value())
    {
        CPERExtension::Raw::properties_t rawProperties{};
        rawProperties.data = std::move(maybeBinary.value());
        cperEvent.extend(rawProperties);
    }

    auto eventId = lg2::commit(std::move(cperEvent));
    co_return;
}

} // namespace redfish_client::core
