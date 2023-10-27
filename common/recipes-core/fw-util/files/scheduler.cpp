#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <syslog.h>
#include <fcntl.h>
#include <semaphore.h>
#include <regex>
#include <openbmc/pal.h>
#include "scheduler.h"

using namespace std;

constexpr auto SEM_PATH = "/scheduled_sem";
constexpr auto SCHEDULE_LIST_PATH = "/var/run/schedule.list";

struct sem_release {
    void operator()(sem_t *sem) const { sem_post(sem); sem_close(sem); }
};

using sem_hndl = unique_ptr<sem_t, sem_release>;

sem_hndl sem_acquire(const string& sem_path) {
  sem_t *sem = sem_open(sem_path.c_str(), O_CREAT | O_EXCL, 0644, 1);
  if ( sem == SEM_FAILED ) {
    if (errno == EEXIST) {
      sem = sem_open(sem_path.c_str(), 0);
    } else {
      return nullptr;
    }
  }

  sem_wait(sem);
  return sem_hndl{sem};
}

string Scheduler::get_task_id(const std::string& task_str) const {
  const regex job_id_rgx("\\d+");
  smatch match;
  regex_search(task_str, match, job_id_rgx);
  return match.str();
}

bool Scheduler::is_number(const string& task_id) const {
  if ( task_id.empty() == true ) {
    return false;
  }
  return all_of(task_id.begin(), task_id.end(), ::isdigit);
}

string Scheduler::exec_and_print(const string& cmd) const {

  //use unique_ptr<T, del> so it can be automatically closed when we leave the scope.
  unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);

  if ( pipe == nullptr ) {
    throw std::runtime_error("Failed to run popen function!");
  }

  char buf[256] = {0};
  string result("");
  while ( auto len = fread(buf, sizeof(char), sizeof(buf), pipe.get()) ) {
    result.append(buf, len);
  }

  return result;
}

bool Scheduler::is_platform_supported() const {

  if ( access("/usr/bin/at", X_OK) != 0 ) {
    return false;
  }

  return true;
}

bool Scheduler::get_task_ids(unordered_set<string>& ids) const
{
  /*
   * Use /usr/bin/atq to get current job id.
   */
  static constexpr auto atq_cmd = "/usr/bin/atq 2>&1";

  if ( is_platform_supported() == false ) {
    cerr << "The function is not supported on this platform!" << endl;
    return false;
  }

  //store task id
  string resp_msg = exec_and_print(atq_cmd);
  string tmp("");
  string::iterator it = resp_msg.begin();
  size_t spos = 0, epos = 0;
  while ( (epos = resp_msg.find('\n', spos)) != string::npos ) {
    tmp.assign( it + spos, it + epos );
    spos = epos + 1;
    ids.insert(get_task_id(tmp));
  }

  return true;
}

bool Scheduler::get_running_task_frus(unordered_set<string>& ongoing_frus) const
{
  /*
   * 1. Use /usr/bin/atq to get current job id.
   * 2. Match to /var/run/schedule.list
   * 3. Get all fru's name which are updating.
   */
  unordered_set<string> existed_task_id;
  if (get_task_ids(existed_task_id) == false) {
    cerr << "Failed to get current task IDs" << endl;
    return false;
  }

  fstream skd_list(SCHEDULE_LIST_PATH, fstream::in);
  if ( skd_list.is_open() == false ) {
    cerr << "Failed to open fstream:" << SCHEDULE_LIST_PATH << endl;
    return false;
  }

  string content("");
  string task_id("");
  while ( getline(skd_list, content) ) {
    task_id = get_task_id(content);
    if ( existed_task_id.find(task_id) != existed_task_id.end() ) {
      regex fru_reg("/usr/bin/fw-util (.*) --update ");
      smatch match;
      regex_search(content, match, fru_reg);
      ongoing_frus.insert(match[1]);
    }
  }

  return true;
}

bool Scheduler::is_task_fru_ongoing(const string& fru) const
{
  uint8_t id;
  if (pal_get_fru_id((char *)fru.c_str(), &id)) {
    cerr << "Failed to acquire the fru:" << fru
         << " status update status." << endl;
    return false;
  }

  return pal_is_fw_update_ongoing(id);
}

int Scheduler::show_task() const {

  auto sem_holder = sem_acquire(SEM_PATH);
  if ( sem_holder == nullptr ) {
    cerr << "Failed to acquire the permission" << endl;
    return false;
  }

  unordered_set<string> existed_task_id;
  if (get_task_ids(existed_task_id) == false) {
    cerr << "Failed to get current task IDs" << endl;
    return -1;
  }

  //atq only shows the job id.
  //Users don't know what commands are going to be executed.
  //Combine the result of atq and SCHEDULE_LIST and provide it to users
  //sync the current list that is provided by atq to SCHEDULE_LIST at the same time.
  fstream skd_list(SCHEDULE_LIST_PATH, fstream::in);
  if ( skd_list.is_open() == false ) {
    return -1;
  }

  //create a tmp file
  string tmp_file = tmpnam(nullptr);
  fstream skd_list_tmp(tmp_file, fstream::in | fstream::out | fstream::app);
  if ( skd_list_tmp.is_open() == false ) {
    cerr << "The fstream isn't associated to a tmp file" << endl;
    return -1;
  }

  //update SCHEDULE_LIST according to the result of atq
  string content("");
  string task_id("");
  while ( getline(skd_list, content) ) {
    task_id = get_task_id(content);
    if ( existed_task_id.find(task_id) != existed_task_id.end() ) {
      skd_list_tmp << content << endl;
      cout << content << endl;
    }
  }

  //reinit the stream and rewind the position of skd_list_tmp
  skd_list.close();
  skd_list.clear();
  skd_list.open(SCHEDULE_LIST_PATH, fstream::out | fstream::trunc);
  skd_list_tmp.seekg (0, skd_list_tmp.beg);
  skd_list << skd_list_tmp.rdbuf();

  //rm tmp file
  remove(tmp_file.c_str());

  return 0;
}

int Scheduler::add_task(const string& fru, const string& comp, const string& image, const string& time) const {
  string fwutil_cmd = "/usr/bin/fw-util " + fru + " --update " + comp + " " + image;
  string scheduled_cmd = " | at " + time + " 2>&1";
  string exec_cmd = "echo \"" + fwutil_cmd + "\"" + scheduled_cmd;

  if ( is_platform_supported() == false ) {
    cerr << "The function is not supported on this platform!" << endl;
    return -1;
  }

  auto sem_holder = sem_acquire(SEM_PATH);
  if ( sem_holder == nullptr ) {
    cerr << "Failed to acquire the permission" << endl;
    return -1;
  }

  /*
   * If /var/run/schedule.list exist, means that maybe
   * there were some fru doing update.
   */
  if (access(SCHEDULE_LIST_PATH, F_OK) == 0) {
    unordered_set<string> ongoing_frus;
    if (get_running_task_frus(ongoing_frus)==false) {
      cerr << "Failed to get the running task's fru" << endl;
      return -1;
    } else {
      if (ongoing_frus.find(fru) != ongoing_frus.end()) {
        cerr << string("Fru : " + fru + " firmware update ongoing.") << endl;
        return -1;
      }
    }
  }

  /*
   * /var/run/schedule.list might not fully indicate running task
   * check again with /tmp/cache/fru%d_fwupd
   */
  if (is_task_fru_ongoing(fru)) {
    cerr << string("Fru : " + fru + " firmware update ongoing.") << endl;
    return -1;
  }

  //extract the substring with the prefix job
  string resp_msg = exec_and_print(exec_cmd);
  size_t pos = 0;
  if ( (pos = resp_msg.find("job")) == string::npos ) {
    cerr << "[Error] " << resp_msg;
    return -1;
  }

  //check the string is match
  string job_msg(resp_msg.begin() + pos, resp_msg.end()-1);
  const regex job_str_rgx("^job\\s+\\d+\\s+at\\s+.+\\d{4}$");
  if ( regex_match(job_msg, job_str_rgx) != true ) {
    cerr << "[Error] " << resp_msg;
    return -1;
  }

  ofstream outfile(SCHEDULE_LIST_PATH, ofstream::app);
  if ( outfile.is_open() == false ) {
    cerr << "The ofstream isn't associated to the list" << endl;
    return -1;
  }

  outfile << job_msg << ":" << fwutil_cmd <<'\n';

  string task_id = get_task_id(job_msg);
  syslog(LOG_CRIT, "Add task %s. %s will be applied to the component of %s at %s", task_id.c_str(), image.c_str(), comp.c_str(), time.c_str());
  cout << "Add task " << task_id << ". " << image << " will be applied to the component of " << comp << " at " << time << endl;
  return 0;
}

int Scheduler::del_task(const string& task_id) const {
  string atrm_cmd = "/usr/bin/atrm " + task_id + " 2>&1";

  if ( is_platform_supported() == false ) {
    cerr << "The function is not supported on this platform!" << endl;
    return -1;
  }

  auto sem_holder = sem_acquire(SEM_PATH);
  if ( sem_holder == nullptr ) {
    cerr << "Failed to acquire the permission" << endl;
    return -1;
  }

  //it should be a number
  if ( is_number(task_id) == false ) {
    cerr << "task id should be a decimal number" << endl;
    return -1;
  }

  //if the command is run successfully, resp_msg would be empty
  string resp_msg = exec_and_print(atrm_cmd);
  if ( resp_msg.empty() == false ) {
    cerr << resp_msg;
    return -1;
  } else {
    syslog(LOG_CRIT, "Task %s is deleted", task_id.c_str());
    cout << "Task " << task_id << " is deleted" << endl;
  }

  //read the list
  //if skd_list open fails, it means the file is not created by add_task
  fstream skd_list(SCHEDULE_LIST_PATH, fstream::in);
  if ( skd_list.is_open() == false ) {
    cerr << "No scheduled commands are ready to run" << endl;
    return -1;
  }

  //create a tmp file
  string tmp_file = tmpnam(nullptr);
  fstream skd_list_tmp(tmp_file, fstream::in | fstream::out | fstream::app);
  if ( skd_list_tmp.is_open() == false ) {
    cerr << "The fstream isn't associated to a tmp file" << endl;
    return -1;
  }

  string content("");
  while ( getline(skd_list, content) ) {
    //skip the target line and write the rest content to skd_list_tmp
    if ( get_task_id(content) != task_id ) {
      skd_list_tmp << content << endl;
    }
  }

  //reinit the stream and rewind the position of skd_list_tmp
  skd_list.close();
  skd_list.clear();
  skd_list.open(SCHEDULE_LIST_PATH, fstream::out | fstream::trunc);
  skd_list_tmp.seekg (0, skd_list_tmp.beg);
  skd_list << skd_list_tmp.rdbuf();

  //rm tmp file
  remove(tmp_file.c_str());

  return 0;
}
