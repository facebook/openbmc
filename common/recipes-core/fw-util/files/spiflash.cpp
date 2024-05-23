#include "spiflash.h"
#include <fstream>
#include <thread>
#include <chrono>
#include <syslog.h>
#include <openbmc/libgpio.h>
#include <sys/stat.h>

using namespace std;

int MTDComponent::getFlashSize(const std::string& mtdIndex)
{
  char command[256];  // Buffer for storing the command to be executed
  char buffer[1024];  // Buffer for storing the output from the command
  FILE* pipe;         // File pointer to manage the pipe
  int flashSize = -1; // Variable to store the flash size, initialized to -1

  // Clear command and buffer before use
  memset(command, 0, sizeof(command));
  memset(buffer, 0, sizeof(buffer));

  // Construct the command that fetches flash chip information
  snprintf(command, sizeof(command), "flashrom -p linux_mtd:dev=%s | grep -i 'flash chip' | awk '{for (i=1; i<=NF; i++) if ($i ~ /kB/) print $(i-1)}'", mtdIndex.c_str());

  // Execute the command using popen, which opens a process by creating a pipe, running a shell, and reading the output
  pipe = popen(command, "r");
  if (!pipe) {
    std::cerr << "Failed to use flashrom get flash memory message!" << std::endl;
    return -1;
  }

  // Read the output and parse the numbers, removing the brackets
  if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    // Move the pointer to the start of the number in the buffer
    char* numStart = buffer;
    char* bufferEnd = buffer + sizeof(buffer) - 1;  // Pointer to the end of the buffer
    while (numStart < bufferEnd && *numStart && !isdigit(*numStart)) {
      numStart++;  // Skip to the first digit
    }
    flashSize = atoi(numStart); // Convert the string to an integer
  } else {
    std::cerr << "Failed to read flash memory size by using flashrom!" << std::endl;
  }

  pclose(pipe);

  // If a valid flash size was found, convert it from kB to bytes and return
  return flashSize > 0 ? flashSize * 1024 : -1;
}

int MTDComponent::update_by_flashrom(const std::string& image)
{
  std::string dev, cmd, comp = this->component(); // Retrieve the current component name
  struct stat st;  // Used to obtain information about the file
  int ret, flashSize, paddingSize;

  // Get the device name associated with the MTD device
  if (!sys().get_mtd_name(_mtd_name, dev)) {
    std::cerr << "Failed to get the MTD device!" << std::endl;
    return FW_STATUS_FAILURE;
  }

  // Extract the numeric part of the MTD device identifier
  std::string dev_number = dev.substr(dev.find("mtd") + 3);
  if (dev_number.empty()) {
    std::cerr << "Failed to get the MTD device number!" << std::endl;
    return FW_STATUS_FAILURE;
  }

  // Get the flash size of the device
  flashSize = getFlashSize(dev_number);
  if (flashSize <= 0) {
    std::cerr << "Failed to get the flash memory size!" << std::endl;
    return FW_STATUS_FAILURE;
  }

  // Temporary file to store the new firmware image
  std::string tempFile = "/tmp/temp_image.bin";
  std::ifstream src(image, std::ios::binary);
  std::ofstream dst(tempFile, std::ios::binary | std::ios::trunc);

  // Check if both source and destination files are open
  if (!src.is_open() || !dst.is_open()) {
    std::cerr << "Failed to open file!" << std::endl;
    return FW_STATUS_FAILURE;
  }

  // Copy the contents from source to destination
  dst << src.rdbuf();
  // src and dst are automatically closed when going out of scope

  // Check Image data status
  if (stat(tempFile.c_str(), &st) != 0) {
    std::cerr << "Failed to get Image data status!" << std::endl;
    return FW_STATUS_FAILURE;
  }

  // Check if padding is needed to match the flash size
  if ((paddingSize = flashSize - st.st_size) > 0) {
    std::ofstream padFile(tempFile, std::ios::binary | std::ios::app);
    if (!padFile) {
      std::cerr << "Failed to open Image!" << std::endl;
      return FW_STATUS_FAILURE;
    }
    std::vector<char> padding(paddingSize, '\xFF'); // Allocate a buffer filled with 0xFF to match the expected padding size
    padFile.write(padding.data(), padding.size());  // Write the padding data to the end of the file
    padFile.close();
  }
  syslog(LOG_CRIT, "Component %s upgrade initiated", comp.c_str());

  // Construct the command to write the new firmware to the device
  cmd = "flashrom -p linux_mtd:dev=" + dev_number + " -w " + tempFile;
  ret = sys().runcmd(cmd);
  if (ret == 0) {
    syslog(LOG_CRIT, "Component %s upgrade completed", comp.c_str());
    return FW_STATUS_SUCCESS;
  }
  return FW_STATUS_FAILURE;
}

int MTDComponent::update_by_flashcp(const std::string& image)
{
  string dev;
  string cmd;
  string comp = this->component();
  int ret;

  if (!sys().get_mtd_name(_mtd_name, dev)) {
    return FW_STATUS_FAILURE;
  }
  syslog(LOG_CRIT, "Component %s upgrade initiated", comp.c_str());

  sys().output << "Flashing to device: " << dev << endl;
  cmd = "flashcp -v " + image + " " + dev;
  ret = sys().runcmd(cmd);
  if (ret == 0) {
    syslog(LOG_CRIT, "Component %s upgrade completed", comp.c_str());
    return FW_STATUS_SUCCESS;
  }
  return FW_STATUS_FAILURE;
}

int MTDComponent::update(std::string image)
{
  // flash_method default is "flashcp"
  if (flash_method == "flashrom") {
    std::cout << "Update by using flasrom" << std::endl;
    return update_by_flashrom(image);
  } else {
    std::cout << "Update by using flashcp" << std::endl;
    return update_by_flashcp(image);
  }
}

int MTDComponent::dump(std::string image)
{
  string dev;
  size_t size, erasesize;
  static constexpr size_t BUFSIZE = 4096;
  auto KB = [](auto x){ return x / 1024; };

  if (!sys().get_mtd_name(_mtd_name, dev, size, erasesize)) {
    return FW_STATUS_FAILURE;
  }

  ifstream ifs(dev.c_str(), ios::in | ios::binary);
  if (!ifs.is_open()) {
    sys().error << "ERROR: Failed to open " << dev << endl;
    return FW_STATUS_FAILURE;
  }

  ofstream ofs(image.c_str(), ios::out | ios::binary);
  if (!ofs.is_open()) {
    ifs.close();
    sys().error << "ERROR: Failed to open " << image << endl;
    return FW_STATUS_FAILURE;
  }

  char buf[BUFSIZE];
  size_t remaining = size;
  uint64_t written;

  sys().output << "Reading from device: " << dev << endl;
  for (written = 0; written < size;) {
    ifs.read(buf, min(sizeof(buf), remaining));
    if (ifs.fail()) {
      if (errno == EINTR) {
        ifs.clear();
        continue;
      }
      sys().error << "ERROR: Failed to read " << dev << endl;
      return FW_STATUS_FAILURE;
    }
    ofs.write(buf, ifs.gcount());
    if (ofs.fail()) {
      if (errno == EINTR) {
        ofs.clear();
        ifs.seekg(-ifs.gcount(), ios::cur);
        continue;
      }
      sys().error << "ERROR: Failed to write " << image << endl;
      return FW_STATUS_FAILURE;
    }
    written += ifs.gcount();
    remaining -= ifs.gcount();

    cout << "\rReading kb: " << KB(written) << "/" << KB(size)
         << " (" << (written * 100 / size) << "%)" << flush;
  }
  cout << endl;
  ifs.close();
  ofs.close();

  return FW_STATUS_SUCCESS;
}


int SPIMTDComponent::update(std::string image)
{
  string cmd;
  std::ofstream ofs;
  int rc;

  ofs.open(spipath + "/bind");
  if (!ofs.is_open()) {
    return -1;
  }
  ofs << spidev;
  if (ofs.bad()) {
    ofs.close();
    sys().error << spidev << " Bind failed" << std::endl;
    return -1;
  }
  ofs.close();
  rc = MTDComponent::update(image);
  ofs.open(spipath + "/unbind");
  if (!ofs.is_open()) {
    sys().error << "ERROR: Cannot unbind " << spidev << " rc=" << rc << std::endl;
    return -1;
  }
  ofs << spidev;
  ofs.close();
  return rc;
}

int SPIMTDComponent::dump(std::string image)
{
  string cmd;
  std::ofstream ofs;
  int rc;

  ofs.open(spipath + "/bind");
  if (!ofs.is_open()) {
    return -1;
  }
  ofs << spidev;
  if (ofs.bad()) {
    ofs.close();
    sys().error << spidev << " Bind failed" << std::endl;
    return -1;
  }
  ofs.close();
  rc = MTDComponent::dump(image);
  ofs.open(spipath + "/unbind");
  if (!ofs.is_open()) {
    sys().error << "ERROR: Cannot unbind " << spidev << " rc=" << rc << std::endl;
    return -1;
  }
  ofs << spidev;
  ofs.close();
  return rc;
}

int GPIOSwitchedSPIMTDComponent::update(std::string  image)
{
  int rc;
  gpio_desc_t *desc = gpio_open_by_shadow(gpio_shadow.c_str());
  if (!desc) {
    return -1;
  }
  if (gpio_set_direction(desc, GPIO_DIRECTION_OUT) ||
      gpio_set_value(desc, access_level ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW)) {
    gpio_close(desc);
    return -1;
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));
  rc = SPIMTDComponent::update(image);
  if (gpio_set_value(desc, access_level ? GPIO_VALUE_LOW : GPIO_VALUE_HIGH) ||
      (change_direction && gpio_set_direction(desc, GPIO_DIRECTION_IN))) {
    gpio_close(desc);
    sys().error << "ERROR: Cannot release control of SPI Dev"  << std::endl;
    return -1;
  }
  gpio_close(desc);
  return rc;
}

int GPIOSwitchedSPIMTDComponent::dump(std::string  image)
{
  int rc;
  gpio_desc_t *desc = gpio_open_by_shadow(gpio_shadow.c_str());
  if (!desc) {
    return -1;
  }
  if (gpio_set_direction(desc, GPIO_DIRECTION_OUT) ||
      gpio_set_value(desc, access_level ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW)) {
    gpio_close(desc);
    return -1;
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));
  rc = SPIMTDComponent::dump(image);
  if (gpio_set_value(desc, access_level ? GPIO_VALUE_LOW : GPIO_VALUE_HIGH) ||
      (change_direction && gpio_set_direction(desc, GPIO_DIRECTION_IN))) {
    gpio_close(desc);
    sys().error << "ERROR: Cannot release control of SPI Dev"  << std::endl;
    return -1;
  }
  gpio_close(desc);
  return rc;
}
