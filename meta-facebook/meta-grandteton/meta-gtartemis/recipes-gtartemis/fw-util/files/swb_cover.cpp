#include "swb_common.hpp"

namespace pldm_signed_info
{
GTSwbBicFwComponent acb_bic("acb", "bic", ACB_BIC_BUS, ACB_BIC_EID, BIC_COMP,
    signed_header_t(gt_swb_comps, BIC_COMP, ASPEED));
GTSwbBicFwComponent meb_bic("meb", "bic", MEB_BIC_BUS, MEB_BIC_EID, BIC_COMP,
    signed_header_t(gt_swb_comps, BIC_COMP, ASPEED));
AcbPeswFwComponent acb_pesw0("acb", "pesw0", ACB_BIC_BUS, ACB_BIC_EID, PEX0_COMP);
AcbPeswFwComponent acb_pesw1("acb", "pesw1", ACB_BIC_BUS, ACB_BIC_EID, PEX1_COMP);
}
