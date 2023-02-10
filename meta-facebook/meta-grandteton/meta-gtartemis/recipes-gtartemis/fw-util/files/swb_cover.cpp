#include "swb_common.hpp"

SwbBicFwComponent acb_bic("acb", "bic", ACB_BIC_BUS, ACB_BIC_EID, BIC_COMP);
SwbBicFwComponent meb_bic("meb", "bic", MEB_BIC_BUS, MEB_BIC_EID, BIC_COMP);
SwbPexFwComponent acb_pesw0("acb", "pesw0", ACB_BIC_BUS, ACB_BIC_EID, PEX0_COMP, 0);
SwbPexFwComponent acb_pesw1("acb", "pesw1", ACB_BIC_BUS, ACB_BIC_EID, PEX1_COMP, 1);

PLDMNicComponent swb_nic0("swb", "swb_nic0", "SWB_NIC0", 0x10, 3);
PLDMNicComponent swb_nic1("swb", "swb_nic1", "SWB_NIC1", 0x11, 3);
PLDMNicComponent swb_nic2("swb", "swb_nic2", "SWB_NIC2", 0x12, 3);
PLDMNicComponent swb_nic3("swb", "swb_nic3", "SWB_NIC3", 0x13, 3);
PLDMNicComponent swb_nic4("swb", "swb_nic4", "SWB_NIC4", 0x14, 3);
PLDMNicComponent swb_nic5("swb", "swb_nic5", "SWB_NIC5", 0x15, 3);
PLDMNicComponent swb_nic6("swb", "swb_nic6", "SWB_NIC6", 0x16, 3);
PLDMNicComponent swb_nic7("swb", "swb_nic7", "SWB_NIC7", 0x17, 3);
