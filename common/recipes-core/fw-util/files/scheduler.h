#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_
#include <string>
class Scheduler {
  private:
    std::string exec_and_print(const std::string& cmd) const;
    bool is_number(const std::string& task_id) const;
    std::string get_task_id(const std::string& task_id) const;
    bool is_platform_supported() const;
  public:
    Scheduler() = default;
    int show_task() const;
    int del_task(const std::string& task_id) const;
    int add_task(const std::string& fru, const std::string& comp, const std::string& image, const std::string& time) const;
};

#endif
