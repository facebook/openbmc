#include "swb_common.hpp"

SwbBicFwRecoveryComponent bic_recovery("swb", "bic_recovery", 3, 0x0A, BIC_COMP);

namespace pldm_signed_info
{
GTSwbBicFwComponent bic("swb", "bic", 3, 0x0A, BIC_COMP,
                        signed_header_t(gt_swb_comps, BIC_COMP, ASPEED));

GTSwbPexFwComponent swb_pex0("swb", "pex0", 3, 0x0A, PEX0_COMP,
                            signed_header_t(gt_swb_comps, PEX0_COMP, BROADCOM));
GTSwbPexFwComponent swb_pex1("swb", "pex1", 3, 0x0A, PEX1_COMP,
                            signed_header_t(gt_swb_comps, PEX1_COMP, BROADCOM));
GTSwbPexFwComponent swb_pex2("swb", "pex2", 3, 0x0A, PEX2_COMP,
                            signed_header_t(gt_swb_comps, PEX2_COMP, BROADCOM));
GTSwbPexFwComponent swb_pex3("swb", "pex3", 3, 0x0A, PEX3_COMP,
                            signed_header_t(gt_swb_comps, PEX3_COMP, BROADCOM));
}
