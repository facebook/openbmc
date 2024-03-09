#include "utils/dbus.hpp"
#include "utils/json.hpp"
#include "utils/mapper.hpp"
#include "utils/register.hpp"

#include <sdbusplus/async.hpp>

#include <string>
#include <variant>
#include <vector>

namespace mfgtool::cmds::inventory
{
PHOSPHOR_LOG2_USING;

struct command
{
    void init(CLI::App& app)
    {
        auto cmd = app.add_subcommand("inventory", "Get inventory");
        init_callback(cmd, *this);
    }

    auto run(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
    {
        using namespace std::literals;
        namespace item = dbuspath::inventory::item;
        namespace asset = dbuspath::inventory::asset;
        using utils::mapper::subtree_for_each_interface;

        auto result = json::empty_map();

        co_await subtree_for_each_interface(
            ctx, item::ns_path, asset::interface,
            // TODO: We are using asset::interface here because a lot of the
            //       entity-manager content appears to be missing the
            //       Inventory.Item.  We need to get this fixed.

            [&](const auto& path, const auto& service,
                const auto& interface) -> sdbusplus::async::task<> {
            if (!interface.starts_with(InventoryIfacePrefix))
            {
                co_return;
            }

            // Insert the interface into the JSON so it shows up even if it
            // doesn't have any properties.
            auto& iface_result =
                result[strip_path(path)][strip_intf(interface)];
            iface_result = json::empty_map();

            debug("Getting properties.");
            for (const auto& [property, value] :
                 co_await sdbusplus::async::proxy()
                     .service(service)
                     .path(path)
                     .interface(interface)
                     .template get_all_properties<InventoryTypes>(ctx))
            {
                // Ignore the entity-manager Probe statement because nobody
                // is going to be interested in that.
                if (property == "Probe")
                {
                    continue;
                }
                std::visit([&](const auto& v) { iface_result[property] = v; },
                           value);
            }
        });

        json::display(result);
    }

    auto strip_path(const auto& p)
    {
        namespace item = dbuspath::inventory::item;
        return p.substr(std::string(item::ns_path).length() + 1);
    }

    auto strip_intf(const auto& i)
    {
        return i.substr(std::string(InventoryIfacePrefix).length() + 1);
    }

    static constexpr auto InventoryIfacePrefix =
        "xyz.openbmc_project.Inventory";

    using InventoryTypes =
        std::variant<bool, size_t, int64_t, uint16_t, std::string,
                     std::vector<uint8_t>, std::vector<std::string>>;
};
MFGTOOL_REGISTER(command);

} // namespace mfgtool::cmds::inventory
