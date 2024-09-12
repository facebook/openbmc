
#include "pldm-update.hpp"

#include <sdbusplus/bus.hpp>
#include <libpldm/utils.h>
#include <libpldm/firmware_update.h>

#include <fcntl.h>    // open
#include <unistd.h>   // open, read
#include <filesystem> // path, copy
#include <thread>     // sleep_for
#include <chrono>     // seconds

class FileDescriptor {
public:
    explicit FileDescriptor(int fd) : fd(fd) {}
    ~FileDescriptor() { if (fd >= 0) close(fd); }
    operator int() const { return fd; }  // Allow using this class wherever an int is expected.
private:
    int fd;
};

bool
is_pldmd_service_running(std::string& _pldmdBusName)
{
    try {
        // use system1 to method "GetUnit"
        auto bus = sdbusplus::bus::new_default();
        auto service = "org.freedesktop.systemd1";
        auto objpath = "/org/freedesktop/systemd1";
        auto target = "pldmd.service";

        // get pldmd.service object path
        // append target to message
        auto msg = bus.new_method_call(
            service,
            objpath,
            "org.freedesktop.systemd1.Manager",
            "GetUnit");
        msg.append(target);
        auto pldmdObjPath = sdbusplus::message::object_path();
        auto reply = bus.call(msg);
        reply.read(pldmdObjPath);

        // get property ActiveState
        // append pldmd.service object path to message and ActiveState
        msg = bus.new_method_call(
            service,
            pldmdObjPath.str.c_str(),
            "org.freedesktop.DBus.Properties",
            "Get"
        );
        msg.append("org.freedesktop.systemd1.Unit", "ActiveState");
        // use variant for dynamic type
        auto activeState = std::variant<std::string>();
        reply = bus.call(msg);
        reply.read(activeState);

        // result
        if (std::get<std::string>(activeState) != "active")
        {
            return false;
        }

        // get Bus Name of pldmd.service
        msg = bus.new_method_call(
            service,
            pldmdObjPath.str.c_str(),
            "org.freedesktop.DBus.Properties",
            "Get"
        );
        msg.append("org.freedesktop.systemd1.Service", "BusName");
        auto pldmdBusName = std::variant<std::string>();
        reply = bus.call(msg);
        reply.read(pldmdBusName);
        _pldmdBusName = std::get<std::string>(pldmdBusName);

        std::cout << "pldmd.service : "
            << _pldmdBusName
            << std::endl;

    } catch (const sdbusplus::exception_t& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }

    return true;
}

size_t
get_compute_software_id(const std::string& file)
{
    FileDescriptor fd(open(file.c_str(), O_RDONLY));
    if (fd < 0) {
        std::cerr << "Cannot open "
                << file
                << std::endl;
        return -1;
    }

    auto offset = sizeof(pldm_package_header_information);
    auto pkgData = std::vector<uint8_t>(offset);
    auto pkgHdr = reinterpret_cast<pldm_package_header_information*>(pkgData.data());

    // read Package Header Info
    if (read(fd, pkgData.data(), offset) != (int)offset) {
        std::cerr << "Can not read "
                << file
                << std::endl;
        return -1;
    }
    auto pkgVerLen = pkgHdr->package_version_string_length;
    pkgData.resize(offset + pkgVerLen);
    if (read(fd, pkgData.data() + offset, pkgVerLen) != (int)pkgVerLen) {
        std::cerr << "Can not read "
                << file
                << std::endl;
        return -1;
    }
    auto str = std::string(
        reinterpret_cast<const char*>(pkgData.data()+offset), pkgVerLen);

    return std::hash<std::string>{}(str);
}

bool
check_pldmd_software_object(std::string pldmdBusName, size_t softwareId)
{
    try {
        auto bus = sdbusplus::bus::new_default();
        auto objPath = std::string(
            "/xyz/openbmc_project/software/" + std::to_string(softwareId));
        auto interface = "org.freedesktop.DBus.Introspectable";
        auto method = "Introspect";

        auto msg = bus.new_method_call(
            pldmdBusName.c_str(),
            objPath.c_str(),
            interface,
            method);
        auto reply = bus.call(msg);

    } catch (const sdbusplus::exception_t& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
    return true;
}

void
active_update(std::string pldmdBusName, size_t softwareId)
{
    try {
        auto bus = sdbusplus::bus::new_default();
        auto objPath = std::string(
            "/xyz/openbmc_project/software/" + std::to_string(softwareId));
        auto interface = "xyz.openbmc_project.Software.Activation";
        auto property = "RequestedActivation";
        auto value = std::variant<std::string>{
            "xyz.openbmc_project.Software.Activation.RequestedActivations.Active"};

        auto msg = bus.new_method_call(
            pldmdBusName.c_str(),
            objPath.c_str(),
            "org.freedesktop.DBus.Properties",
            "Set");

        msg.append(interface, property, value);
        auto reply = bus.call(msg);

    } catch (const sdbusplus::exception_t& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

uint8_t
get_progress(std::string pldmdBusName, size_t softwareId)
{
    try {
        auto bus = sdbusplus::bus::new_default();
        auto objPath = std::string(
            "/xyz/openbmc_project/software/" + std::to_string(softwareId));
        auto interface = "xyz.openbmc_project.Software.ActivationProgress";
        auto property = "Progress";

        auto msg = bus.new_method_call(
            pldmdBusName.c_str(),
            objPath.c_str(),
            "org.freedesktop.DBus.Properties",
            "Get");
        msg.append(interface, property);
        auto progress = std::variant<uint8_t>();
        auto reply = bus.call(msg);
        reply.read(progress);

        return std::get<uint8_t>(progress);

    } catch (const sdbusplus::exception_t& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 0xFF;
    }
}


void
PldmUpdateApp::pldm_update(const std::string& file)
{
    auto softwareId = get_compute_software_id(file);
    auto pldmdBusCtlName = pldmdBusName;
    std::cout << "pldm_update: " << file << std::endl;
    std::cout << "softwareId: " << softwareId << std::endl;

    // copy file to /tmp/pldm_images/
    try {
        auto src = std::filesystem::path(file);
        auto dest = std::filesystem::path("/tmp/pldm_images/");
        std::filesystem::copy(src, dest / src.filename());
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    // wait for pldmd.service process
    bool is_processed = false;
    for (int retry = 0; retry <= 5; ++retry) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        is_processed = check_pldmd_software_object(pldmdBusCtlName, softwareId);
        if (is_processed)
            break;
        if (retry == 5) {
            std::cerr
                << "Time out : "
                << "pldmd.service did not process image."
                << std::endl;
            return;
        }
    }

    active_update(pldmdBusCtlName, softwareId);
    for (int time = 0; time <= 200; ++time) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto progress = get_progress(pldmdBusCtlName, softwareId);

        if (progress == 0xFF) {
            break;
        } else if (time == 200) {
            std::cout
                << std::endl
                << "Time out : "
                << "pldmd.service did not finish update.";
            break;
        } else if (progress <= 100) {
            std::cout << "\rCurrent progress: "
                << static_cast<int>(progress)
                << "/100 ("
                << time
                << "s)"
                << std::flush;
            if (progress == 100)
                break;
        }
    }

    std::cout << std::endl;
}

int
main(int argc, char** argv)
{
    PldmUpdateApp app("\nPLDM update tool. (Work with pldmd.service)\n");

    // check pldmd.service first
    if (!is_pldmd_service_running(app.pldmdBusName)) {
        std::cerr << "pldmd.service is not running." << std::endl;
        return -1;
    }

    app.add_options();
    app.run(argc, argv);
    return 0;
}