#include "fw-util.h"

#define MD5_SIZE          (16)
#define PROJECT_CODE_SIZE (16)
#define FW_VERSION_SIZE   (13)
#define ERROR_PROOF       (3)
#define SIGN_INFO_SIZE    (MD5_SIZE+PROJECT_CODE_SIZE+FW_VERSION_SIZE+ERROR_PROOF+MD5_SIZE)

/*
 * Signed Image would be => Image + Sign_info
 *
 * Sign_info :
 *   MD5 - 1      (16 bytes)
 *   Project Name (16 bytes)
 *   Firmware Ver (13 bytes)
 *   Error Proof  ( 3 bytes)
 *   MD5 - 2      (16 bytes)
 *
 * MD5 - 1 calculate from Image
 * MD5 - 2 calculate from Sign_info (without MD5 - 2)
 * Error Proof:
 *   Board id     [ 4: 0]
 *   Stage id     [ 7: 5]
 *   Component id [15: 8]
 *   Instance id  [23:16]
 */

struct signed_header_t {

  std::string project_name{};
  uint8_t board_id{};
  uint8_t stage_id{};
  uint8_t component_id{};

  signed_header_t(){}
  signed_header_t(const std::string& pn, uint8_t bid, uint8_t sid, uint8_t cid):
            project_name(pn), board_id(bid), stage_id(sid), component_id(cid){}
  signed_header_t(const signed_header_t& info, uint8_t component_id):
            project_name(info.project_name), board_id(info.board_id),
            stage_id(info.stage_id), component_id(component_id) {}

};

class SignedDecoder {
  private:
    size_t image_size = 0;
    uint8_t md5[MD5_SIZE] = {0};
    std::string temp_image_path{};
    static void trim_function(std::string& str);

  protected:
    signed_header_t info;
    int check_project_code(uint8_t* data);
    static int check_md5(const std::string& image_path, long offset, long size, uint8_t* data);
    int check_error_proof(uint8_t* err_proof) const;

  public:
    SignedDecoder(const signed_header_t& info, const std::string &fru) : temp_image_path("/tmp/" + fru + "_signed_update.bin"), info(info) {}
    // check signed info and store.
    int is_image_signed(const std::string& image_path);
    // would change original file path to temp file path.
    int get_image(std::string& image_path);
    // delete temp file path.
    int delete_image();
};
