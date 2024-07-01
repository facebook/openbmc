#include "swb_common.hpp"
#include <openbmc/kv.hpp>

SwbBicFwRecoveryComponent bic_recovery("swb", "bic_recovery", 3, 0x0A, BIC_COMP);
class swb_cover_config {
    public:
      swb_cover_config() {
        std::string present = kv::get("swb_nic_present", kv::region::temp);
        if (present == "0") {
          static SwbPLDMNicComponent swb_nic0("swb", "swb_nic0", "SWB_NIC0", 0x10, SWB_BUS_ID);
          static SwbPLDMNicComponent swb_nic1("swb", "swb_nic1", "SWB_NIC1", 0x11, SWB_BUS_ID);
          static SwbPLDMNicComponent swb_nic2("swb", "swb_nic2", "SWB_NIC2", 0x12, SWB_BUS_ID);
          static SwbPLDMNicComponent swb_nic3("swb", "swb_nic3", "SWB_NIC3", 0x13, SWB_BUS_ID);
          static SwbPLDMNicComponent swb_nic4("swb", "swb_nic4", "SWB_NIC4", 0x14, SWB_BUS_ID);
          static SwbPLDMNicComponent swb_nic5("swb", "swb_nic5", "SWB_NIC5", 0x15, SWB_BUS_ID);
          static SwbPLDMNicComponent swb_nic6("swb", "swb_nic6", "SWB_NIC6", 0x16, SWB_BUS_ID);
          static SwbPLDMNicComponent swb_nic7("swb", "swb_nic7", "SWB_NIC7", 0x17, SWB_BUS_ID);
        }
      }
};
swb_cover_config _swb_cover_config;
