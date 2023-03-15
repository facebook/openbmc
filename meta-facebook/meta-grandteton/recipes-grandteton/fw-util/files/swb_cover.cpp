#include "swb_common.hpp"

SwbBicFwRecoveryComponent bic_recovery("swb", "bic_recovery", SWB_BUS_ID, SWB_BIC_EID, BIC_COMP);
PLDMNicComponent swb_nic0("swb", "swb_nic0", "SWB_NIC0", 0x10, SWB_BUS_ID);
PLDMNicComponent swb_nic1("swb", "swb_nic1", "SWB_NIC1", 0x11, SWB_BUS_ID);
PLDMNicComponent swb_nic2("swb", "swb_nic2", "SWB_NIC2", 0x12, SWB_BUS_ID);
PLDMNicComponent swb_nic3("swb", "swb_nic3", "SWB_NIC3", 0x13, SWB_BUS_ID);
PLDMNicComponent swb_nic4("swb", "swb_nic4", "SWB_NIC4", 0x14, SWB_BUS_ID);
PLDMNicComponent swb_nic5("swb", "swb_nic5", "SWB_NIC5", 0x15, SWB_BUS_ID);
PLDMNicComponent swb_nic6("swb", "swb_nic6", "SWB_NIC6", 0x16, SWB_BUS_ID);
PLDMNicComponent swb_nic7("swb", "swb_nic7", "SWB_NIC7", 0x17, SWB_BUS_ID);

namespace pldm_signed_info
{
GTSwbBicFwComponent bic("swb", "bic", SWB_BUS_ID, SWB_BIC_EID, BIC_COMP,
                        signed_header_t(gt_swb_comps, BIC_COMP, ASPEED));

GTSwbPexFwComponent swb_pex0("swb", "pex0", SWB_BUS_ID, SWB_BIC_EID, PEX0_COMP,
                            signed_header_t(gt_swb_comps, PEX0_COMP, BROADCOM));
GTSwbPexFwComponent swb_pex1("swb", "pex1", SWB_BUS_ID, SWB_BIC_EID, PEX1_COMP,
                            signed_header_t(gt_swb_comps, PEX1_COMP, BROADCOM));
GTSwbPexFwComponent swb_pex2("swb", "pex2", SWB_BUS_ID, SWB_BIC_EID, PEX2_COMP,
                            signed_header_t(gt_swb_comps, PEX2_COMP, BROADCOM));
GTSwbPexFwComponent swb_pex3("swb", "pex3", SWB_BUS_ID, SWB_BIC_EID, PEX3_COMP,
                            signed_header_t(gt_swb_comps, PEX3_COMP, BROADCOM));

GTSwbVrComponent vr_pex0_vcc("swb", "pex01_vcc", "VR_PEX01_VCC", SWB_BUS_ID, SWB_BIC_EID, VR0_COMP,
                            signed_header_t(gt_swb_comps, VR0_COMP));
GTSwbVrComponent vr_pex1_vcc("swb", "pex23_vcc", "VR_PEX23_VCC", SWB_BUS_ID, SWB_BIC_EID, VR1_COMP,
                            signed_header_t(gt_swb_comps, VR1_COMP));

GTSwbCpldComponent swb_cpld("swb", "swb_cpld", LCMXO3_9400C, SWB_BUS_ID, 0x40, &cpld_pldm_wr,
                            SWB_BIC_EID, CPLD_COMP, signed_header_t(gt_swb_comps, CPLD_COMP, LATTICE));
}


