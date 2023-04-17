#include "swb_common.hpp"

namespace pldm_signed_info
{
GTSwbBicFwComponent acb_bic("cb", "bic", ACB_BIC_BUS, ACB_BIC_EID, BIC_COMP,
    signed_header_t(gt_swb_comps, BIC_COMP, ASPEED));
GTSwbBicFwComponent meb_bic("mc", "bic", MEB_BIC_BUS, MEB_BIC_EID, BIC_COMP,
    signed_header_t(gt_swb_comps, BIC_COMP, ASPEED));
AcbPeswFwComponent acb_pesw0("cb", "pesw0", ACB_BIC_BUS, ACB_BIC_EID, PEX0_COMP);
AcbPeswFwComponent acb_pesw1("cb", "pesw1", ACB_BIC_BUS, ACB_BIC_EID, PEX1_COMP);
}

MebCxlFwComponent meb_cxl1("mc", "cxl1", MEB_BIC_BUS, MEB_BIC_EID, CXL1_COMP);
MebCxlFwComponent meb_cxl2("mc", "cxl2", MEB_BIC_BUS, MEB_BIC_EID, CXL2_COMP);
MebCxlFwComponent meb_cxl3("mc", "cxl3", MEB_BIC_BUS, MEB_BIC_EID, CXL3_COMP);
MebCxlFwComponent meb_cxl4("mc", "cxl4", MEB_BIC_BUS, MEB_BIC_EID, CXL4_COMP);
MebCxlFwComponent meb_cxl5("mc", "cxl5", MEB_BIC_BUS, MEB_BIC_EID, CXL5_COMP);
MebCxlFwComponent meb_cxl6("mc", "cxl6", MEB_BIC_BUS, MEB_BIC_EID, CXL6_COMP);
MebCxlFwComponent meb_cxl7("mc", "cxl7", MEB_BIC_BUS, MEB_BIC_EID, CXL7_COMP);
MebCxlFwComponent meb_cxl8("mc", "cxl8", MEB_BIC_BUS, MEB_BIC_EID, CXL8_COMP);

