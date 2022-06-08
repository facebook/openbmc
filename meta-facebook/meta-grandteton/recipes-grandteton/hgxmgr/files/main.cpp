#include "restclient-cpp/restclient.h"
#include <iostream>

int main(int argc, char *argv[]) {
  RestClient::Response r = RestClient::get("http://url.com");

  std::cout << "CODE: " << r.code << std::endl;
  return 0;
}
