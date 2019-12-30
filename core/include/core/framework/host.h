#ifndef VORTEX_CORE_FRAMEWORK_HOST_H
#define VORTEX_CORE_FRAMEWORK_HOST_H

#include <string>

namespace vortex {
namespace core {
namespace framework {

class framework;

class host {
 private:
  framework* framework_;

 public:
  host(framework* framework);

  void find(std::string hostname);

  std::string get_id();
  std::string get_host();
  std::string get_app();
};

}  // namespace framework
}  // namespace core
}  // namespace vortex

#endif  // VORTEX_CORE_FRAMEWORK_HOST_H