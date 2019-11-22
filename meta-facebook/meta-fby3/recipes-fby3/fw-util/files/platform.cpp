#include "bic_fw.h"
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

// Register BIC FW and BL components
/****************
 *param1 - string fru
 *param2 - string board
 *param3 - string comp
 *param4 - uint8_t slot id
 *param5 - uint8_t intf
*****************/
BicFwComponent bicfw0("slot0", "sb", "bic", 0, NONE_INTF);
BicFwBlComponent bicfwbl0("slot0", "sb", "bicbl", 0, NONE_INTF);
#endif
