#pragma once
#include <fcntl.h>
#include <semaphore.h>
#include <string>

class Exclusion {
  sem_t* sem = nullptr;
  bool allowed = false;

 public:
  bool error() const {
    return !allowed;
  }

  Exclusion(const std::string& path = "/logsem") {
    sem = sem_open(path.c_str(), O_CREAT | O_EXCL, 0644, 1);
    if (sem == SEM_FAILED && errno == EEXIST) {
      sem = sem_open(path.c_str(), 0);
    }
    if (sem != SEM_FAILED && sem != NULL) {
      sem_wait(sem);
      allowed = true;
    } else {
      sem = nullptr;
    }
  }
  ~Exclusion() {
    if (sem != nullptr) {
      sem_post(sem);
      sem_close(sem);
    }
  }
  // Cannot be safely transferred
  Exclusion(const Exclusion& other) = delete;
  Exclusion(Exclusion&& other) = delete;
};
