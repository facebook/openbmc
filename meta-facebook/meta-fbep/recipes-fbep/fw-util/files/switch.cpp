#include "fw-util.h"
#include <openbmc/pal.h>

using namespace std;

class PAXComponent0 : public Component {
  public:
    PAXComponent0(string fru, string comp)
      : Component(fru, comp) {}
    int print_version();
    int update(string image);
};

int PAXComponent0::print_version()
{
  int ret;
  char ver[64] = {0};

  cout << "PAX0 IMG Version: ";
  pal_set_pax_proc_ongoing(PAX0, 10);
  ret = pal_get_pax_version(PAX0, ver);
  if (ret < 0)
    cout << "NA" << endl;
  else
    cout << string(ver) << endl;

  pal_set_pax_proc_ongoing(PAX0, 0);

  return ret;
}

int PAXComponent0::update(string image)
{
  int ret;
  string cmd("switchtec fw-update /dev/i2c-12@0x18 ");

  cmd = cmd + image;
  pal_set_pax_proc_ongoing(PAX0, 30*60);
  ret = system(cmd.c_str());
  pal_set_pax_proc_ongoing(PAX0, 0);

  return ret < 0? -1: 0;
}

class PAXComponent1 : public Component {
  public:
    PAXComponent1(string fru, string comp)
      : Component(fru, comp) {}
    int print_version();
    int update(string image);
};

int PAXComponent1::print_version()
{
  int ret;
  char ver[64] = {0};

  cout << "PAX1 IMG Version: ";
  pal_set_pax_proc_ongoing(PAX1, 10);
  ret = pal_get_pax_version(PAX1, ver);
  if (ret < 0)
    cout << "NA" << endl;
  else
    cout << string(ver) << endl;

  pal_set_pax_proc_ongoing(PAX1, 0);

  return ret;
}

int PAXComponent1::update(string image)
{
  int ret;
  string cmd("switchtec fw-update /dev/i2c-12@0x19 ");

  cmd = cmd + image;
  pal_set_pax_proc_ongoing(PAX1, 30*60);
  ret = system(cmd.c_str());
  pal_set_pax_proc_ongoing(PAX1, 0);

  return ret < 0? -1: 0;
}

class PAXComponent2 : public Component {
  public:
    PAXComponent2(string fru, string comp)
      : Component(fru, comp) {}
    int print_version();
    int update(string image);
};

int PAXComponent2::print_version()
{
  int ret;
  char ver[64] = {0};

  cout << "PAX2 IMG Version: ";
  pal_set_pax_proc_ongoing(PAX2, 10);
  ret = pal_get_pax_version(PAX2, ver);
  if (ret < 0)
    cout << "NA" << endl;
  else
    cout << string(ver) << endl;

  pal_set_pax_proc_ongoing(PAX2, 0);

  return ret;
}

int PAXComponent2::update(string image)
{
  int ret;
  string cmd("switchtec fw-update /dev/i2c-12@0x1a ");

  cmd = cmd + image;
  pal_set_pax_proc_ongoing(PAX2, 30*60);
  ret = system(cmd.c_str());
  pal_set_pax_proc_ongoing(PAX2, 0);

  return ret < 0? -1: 0;
}

class PAXComponent3 : public Component {
  public:
    PAXComponent3(string fru, string comp)
      : Component(fru, comp) {}
    int print_version();
    int update(string image);
};

int PAXComponent3::print_version()
{
  int ret;
  char ver[64] = {0};

  cout << "PAX3 IMG Version: ";
  pal_set_pax_proc_ongoing(PAX3, 10);
  ret = pal_get_pax_version(PAX3, ver);
  if (ret < 0)
    cout << "NA" << endl;
  else
    cout << string(ver) << endl;

  pal_set_pax_proc_ongoing(PAX3, 0);

  return ret;
}

int PAXComponent3::update(string image)
{
  int ret;
  string cmd("switchtec fw-update /dev/i2c-12@0x1b ");

  cmd = cmd + image;
  pal_set_pax_proc_ongoing(PAX3, 30*60);
  ret = system(cmd.c_str());
  pal_set_pax_proc_ongoing(PAX3, 0);

  return ret < 0? -1: 0;
}

PAXComponent0 pax0("mb", "pax0");
PAXComponent1 pax1("mb", "pax1");
PAXComponent2 pax2("mb", "pax2");
PAXComponent3 pax3("mb", "pax3");
