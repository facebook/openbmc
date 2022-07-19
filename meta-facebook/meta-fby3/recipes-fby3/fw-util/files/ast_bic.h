#ifndef _AST_BIC_H_
#define _AST_BIC_H_

#include "bic_fw.h"
#include "server.h"
#include "expansion.h"

using namespace std;

class AstBicFwRecoveryComponent : public BicFwComponent {
  uint8_t slot_id = 0;
  uint8_t fw_comp = 0;
  string name;  
  Server server;
  ExpansionBoard expansion;
  private:
    int ast_bic_recovery(string image, bool force);
  public:
    AstBicFwRecoveryComponent(string fru, string comp, uint8_t _slot_id, string _name, uint8_t _fw_comp)
      : BicFwComponent(fru, comp, _slot_id), slot_id(_slot_id), fw_comp(_fw_comp), name(_name), server(_slot_id, fru), expansion(_slot_id, fru, _name, _fw_comp) {}
    int update(string image);
    int fupdate(string image);
};



#endif  //_AST_BIC_H_
