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

MebCxlFwComponent meb_cxl1("meb", "cxl1", MEB_BIC_BUS, MEB_BIC_EID, CXL1_COMP);
MebCxlFwComponent meb_cxl2("meb", "cxl2", MEB_BIC_BUS, MEB_BIC_EID, CXL2_COMP);
MebCxlFwComponent meb_cxl3("meb", "cxl3", MEB_BIC_BUS, MEB_BIC_EID, CXL3_COMP);
MebCxlFwComponent meb_cxl4("meb", "cxl4", MEB_BIC_BUS, MEB_BIC_EID, CXL4_COMP);
MebCxlFwComponent meb_cxl5("meb", "cxl5", MEB_BIC_BUS, MEB_BIC_EID, CXL5_COMP);
MebCxlFwComponent meb_cxl6("meb", "cxl6", MEB_BIC_BUS, MEB_BIC_EID, CXL6_COMP);
MebCxlFwComponent meb_cxl7("meb", "cxl7", MEB_BIC_BUS, MEB_BIC_EID, CXL7_COMP);
MebCxlFwComponent meb_cxl8("meb", "cxl8", MEB_BIC_BUS, MEB_BIC_EID, CXL8_COMP);
