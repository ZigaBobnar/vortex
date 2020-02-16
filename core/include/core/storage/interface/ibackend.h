#ifndef VORTEX_CORE_STORAGE_INTERFACE_IBACKEND_H
#define VORTEX_CORE_STORAGE_INTERFACE_IBACKEND_H

#include <string>
#include <maze/object.h>
#include <core/storage/interface/idatabase.h>
#include <core/storage/interface/icollection.h>

namespace vortex {
namespace core {
namespace storage {
namespace interface {

class IBackend {
public:
  IBackend();
  IBackend(const maze::object& config);

  IDatabase get_database(std::string database_name);
  ICollection get_collection(std::string database_name, std::string collection_name);
};

}  // namespace interface
}  // namespace storage
}  // namespace core
}  // namespace vortex

#endif  // VORTEX_CORE_STORAGE_INTERFACE_IBACKEND_H
