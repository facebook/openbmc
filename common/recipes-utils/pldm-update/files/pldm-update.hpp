#pragma once

#include <string>
#include <iostream>
#include <stdexcept>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>

struct PldmUpdateLock
{
  int fd = -1;
  PldmUpdateLock() 
  {
    fd = open("/tmp/pldm-update-ag.lock", O_CREAT | O_RDWR, 0666);
    if (fd < 0)
    {
      std::cerr << "Cannot create/open /tmp/pldm-update-ag.lock" << std::endl;
      throw std::runtime_error("Cannot create!");
    }
    if (flock(fd, LOCK_EX | LOCK_NB) < 0)
    {
      close(fd);
      fd = -1;
      throw std::runtime_error(
        "Update aborted: The same process is still ongoing.");
    }
  }
  ~PldmUpdateLock()
  {
    if (fd < 0)
      return;
    flock(fd, LOCK_UN);
    close(fd);
  }
};

void pldm_update(const std::string& file);