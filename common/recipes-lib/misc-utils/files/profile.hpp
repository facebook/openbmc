#ifndef _PROFILING_HPP_
#define _PROFILING_HPP_
#include <chrono>
#include <iostream>

namespace openbmc {

class Profile {
  using milli = std::chrono::milliseconds;
  std::string descriptor;
  std::chrono::time_point<std::chrono::high_resolution_clock> start =
      std::chrono::high_resolution_clock::now();
  std::ostream& dump;

 public:
  explicit Profile(const std::string& desc, std::ostream& os = std::cout)
      : descriptor(desc), dump(os) {}
  ~Profile() {
    auto stop = std::chrono::high_resolution_clock::now();
    auto dur = std::chrono::duration_cast<milli>(stop - start);
    dump << "PROFILE " << descriptor << " : " << dur.count() << " ms"
         << std::endl;
  }
};

} // namespace openbmc

#define PROFILE_SCOPE(name, ...) openbmc::Profile name(#name, ##__VA_ARGS__)

#endif
