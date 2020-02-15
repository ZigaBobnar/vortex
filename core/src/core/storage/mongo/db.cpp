#include <core/storage/mongo/db.h>

namespace vortex {
namespace core {
namespace storage {
namespace mongo {

Db::Db(mongocxx::database database) : database_(database) {

}

Collection Db::get_collection(std::string collection_name) {
  return Collection(database_[collection_name]);
}

std::vector<std::string> Db::list_collections() {
  std::vector<std::string> collections;

  auto colls = database_.list_collections();
  for (auto it = colls.begin(); it != colls.end(); it++) {
    collections.push_back((*it)["name"].get_utf8().value.to_string());
  }

  return collections;
}

void Db::drop_database() {
  database_.drop();
}

}  // namespace mongo
}  // namespace storage
}  // namespace core
}  // namespace vortex
