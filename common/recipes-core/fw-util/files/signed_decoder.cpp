#include "signed_decoder.hpp"
#include <syslog.h>   // for syslog
#include <fcntl.h>    // for open()
#include <unistd.h>   // for close()
#include <sys/stat.h> // for stat()
#include <openssl/evp.h> // for evp_md_ctx*

#define MD5_READ_BYTES    (1024)

using namespace std;

int InfoChecker::check_md5(const string &image_path, long offset, long size, uint8_t* data)
{
  int fd, byte_num, read_bytes, ret = 0;
  char read_buf[MD5_READ_BYTES] = {0};
  uint8_t md5_digest[EVP_MAX_MD_SIZE] = {0};
  EVP_MD_CTX* ctx = NULL;

  if (size <= 0) {
    return MD5_ERR::SIZE_ERROR;
  }

  fd = open(image_path.c_str(), O_RDONLY);
  if (fd < 0) {
    return MD5_ERR::FILE_ERROR;
  }

  ctx = EVP_MD_CTX_create();
  EVP_MD_CTX_init(ctx);
  ret = EVP_DigestInit(ctx, EVP_md5());
  if (ret == 0) {
    ret = MD5_ERR::MD5_INIT_ERROR;
    goto exit;
  }

  lseek(fd, offset, SEEK_SET);
  while (size > 0) {

    if (size < MD5_READ_BYTES) {
      read_bytes = size;
      size = 0;
    } else {
      read_bytes = MD5_READ_BYTES;
      size -= MD5_READ_BYTES;
    }
    byte_num = read(fd, read_buf, read_bytes);

    ret = EVP_DigestUpdate(ctx, read_buf, byte_num);
    if (ret == 0) {
      syslog(LOG_WARNING, "%s(): failed to update context to calculate MD5 of %s.", __func__, image_path.c_str());
      ret = MD5_ERR::MD5_UPDATE_ERROR;
      goto exit;
    }
  }

  ret = EVP_DigestFinal(ctx, md5_digest, NULL);
  if (ret == 0) {
    ret = MD5_ERR::MD5_FINAL_ERROR;
    goto exit;
  }

  if (memcmp(md5_digest, data, MD5_SIZE) != 0) {
    ret = MD5_ERR::MD5_CHECKSUM_ERROR;
    goto exit;
  }

  ret = MD5_ERR::MD5_SUCCESS;
exit:
  close(fd);
  return ret;
}

int InfoChecker::check_header_info(const signed_header_t& img_info)
{
  if (comp_info.project_name.compare(img_info.project_name) != 0) {
    syslog(LOG_WARNING, "%s PROJECT_NAME_NOT_MATCH\n", __func__);
    return INFO_ERR::PROJECT_NAME_NOT_MATCH;
  }

  if (comp_info.board_id != img_info.board_id) {
    syslog(LOG_WARNING, "%s BOARD_NOT_MATCH\n", __func__);
    return INFO_ERR::BOARD_NOT_MATCH;
  }

  if (comp_info.component_id != COMPONENT_VERIFY_SKIPPED &&
    comp_info.component_id != img_info.component_id) {
    syslog(LOG_WARNING, "%s COMPONENT_NOT_MATCH\n", __func__);
    return INFO_ERR::COMPONENT_NOT_MATCH;
  }

  if (comp_info.vendor_id != img_info.vendor_id) {
    syslog(LOG_WARNING, "%s SOURCE_NOT_MATCH\n", __func__);
    return INFO_ERR::SOURCE_NOT_MATCH;
  }

  return INFO_ERR::EP_SUCCESS;
}

struct error_proof_t {
  uint8_t board_id : 5;
  uint8_t stage_id : 3;
  uint8_t component_id;
  uint8_t vendor_id;
};

void SignComponent::trim_function(string &str)
{
  size_t found;
  string whitespaces(" \t\f\v\n\r");

  found = str.find_last_not_of(whitespaces);
  if (found != string::npos) {
    str.erase(found+1);
  } else {
    str.clear();
  }
}

signed_header_t SignComponent::get_info(FW_IMG_INFO& file_info)
{
  signed_header_t info{};

  auto err = reinterpret_cast<error_proof_t*>(file_info.Error_Proof);
  info.board_id     = err->board_id;
  info.stage_id     = err->stage_id;
  info.component_id = err->component_id;
  info.vendor_id    = err->vendor_id;

  auto str = string((char*)file_info.Project_Code);
  str.resize(PROJECT_CODE_SIZE);
  trim_function(str);
  info.project_name = str;

  return info;
}

int SignComponent::is_image_signed(const string& image_path, bool force)
{
  int fd, ret;
  struct stat file_stat;
  FW_IMG_INFO file_info;
  off_t info_offs;

  if (stat(image_path.c_str(), &file_stat) < 0) {
    syslog(LOG_WARNING, "%s failed to open %s to check file infomation.\n", __func__, image_path.c_str());
    return FORMAT_ERR::INVALID_SIGNATURE;
  }

  if (force) {
    image_size = (long)file_stat.st_size;
    return FORMAT_ERR::SUCCESS;
  }

  fd = open(image_path.c_str(), O_RDONLY);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s Cannot open %s for reading\n", __func__, image_path.c_str());
    return FORMAT_ERR::INVALID_SIGNATURE;
  }

  info_offs = file_stat.st_size - SIGN_INFO_SIZE;
  if (lseek(fd, info_offs, SEEK_SET) != info_offs) {
    close(fd);
    syslog(LOG_WARNING, "%s Cannot seek %s\n", __func__, image_path.c_str());
    return FORMAT_ERR::INVALID_SIGNATURE;
  }

  if (read(fd, &file_info, SIGN_INFO_SIZE) != SIGN_INFO_SIZE) {
    close(fd);
    syslog(LOG_WARNING, "%s Cannot read %s\n", __func__, image_path.c_str());
    return FORMAT_ERR::INVALID_SIGNATURE;
  }
  close(fd);
  image_size = (long)info_offs;

  ret = check_md5(image_path, 0, info_offs, file_info.MD5_1);
  if (ret != 0) {
    syslog(LOG_WARNING, "%s MD5-1 check failed, error code: %d.\n", __func__, -ret);
    return FORMAT_ERR::INVALID_SIGNATURE;
  }

  ret = check_md5(image_path, info_offs, SIGN_INFO_SIZE-MD5_SIZE, file_info.MD5_2);
  if (ret != 0) {
    syslog(LOG_WARNING, "%s MD5-2 check failed, error code: %d.\n", __func__, -ret);
    return FORMAT_ERR::INVALID_SIGNATURE;
  }

  ret = check_header_info(get_info(file_info));
  if (ret != 0) {
    syslog(LOG_WARNING, "%s Info check failed, error code: %d.\n", __func__, -ret);
    return FORMAT_ERR::INVALID_SIGNATURE;
  }

  memcpy(md5, file_info.MD5_1, MD5_SIZE);
  return FORMAT_ERR::SUCCESS;
}

int SignComponent::get_image(string& image_path, bool force)
{
  int src = -1, dst = -1, ret = FORMAT_ERR::SUCCESS;
  vector<uint8_t> data(image_size);

  /*
   *  Copy file with image_size
   */
  src = open(image_path.c_str(), O_RDONLY);
  if (src < 0) {
    syslog(LOG_WARNING, "%s Cannot open %s for reading\n", __func__, image_path.c_str());
    return FORMAT_ERR::INVALID_SIGNATURE;
  }

  dst = open(temp_image_path.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (dst < 0) {
    close(src);
    syslog(LOG_WARNING, "%s Cannot open %s for writing\n", __func__, temp_image_path.c_str());
    return FORMAT_ERR::INVALID_SIGNATURE;
  }

  if (read(src, data.data(), data.size()) != (int)image_size) {
    syslog(LOG_WARNING, "%s Cannot read %s\n", __func__, image_path.c_str());
    ret = FORMAT_ERR::INVALID_SIGNATURE;
    goto file_exit;
  }

  if (write(dst, data.data(), data.size()) != (int)image_size) {
    syslog(LOG_WARNING, "%s Cannot write %s\n", __func__, temp_image_path.c_str());
    ret = FORMAT_ERR::INVALID_SIGNATURE;
    goto file_exit;
  }
  close(src);
  close(dst);
  src = -1;
  dst = -1;

  // chmod to 0400 (read only)
  if (chmod(temp_image_path.c_str(), S_IRUSR) < 0) {
    syslog(LOG_WARNING, "%s Cannot change %s mode\n", __func__, temp_image_path.c_str());
    ret = FORMAT_ERR::INVALID_SIGNATURE;
    goto file_exit;
  }

  if (!force) {
    ret = check_md5(temp_image_path, 0, image_size, md5);
    if (ret != 0) {
      syslog(LOG_WARNING, "%s MD5-1 check failed, error code: %d.\n", __func__, -ret);
      ret = FORMAT_ERR::INVALID_SIGNATURE;
      goto file_exit;
    }
  }

  image_path = temp_image_path;

file_exit:
  if (src >= 0)
    close(src);
  if (dst >= 0)
    close(dst);
  return ret;
}

int SignComponent::delete_image()
{
  return remove(temp_image_path.c_str());
}

int SignComponent::signed_image_update(string image, bool force)
{
  int ret = 0;

  ret = is_image_signed(image, force);
  if (ret && !force) {
    printf("Firmware not valid. error(%d)\n", -ret);
    return -1;
  }

  ret = get_image(image, force);
  if (ret) {
    printf("Get copy file. error(%d)\n", -ret);
    return -1;
  }

  ret = component_update(image, force);
  if (ret) {
    printf("%s\n", image.c_str());
    printf("Update component with temp file failed. error(%d)\n", ret);
  }

  int delete_ret = delete_image();
  if (delete_ret) {
    printf("Remove temp file failed. error(%d)\n", -delete_ret);
  }

  return ret;
}
