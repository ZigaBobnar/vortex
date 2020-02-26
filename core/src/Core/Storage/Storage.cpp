#include <Core/Storage/Storage.h>
#include <Core/Framework.h>

namespace Vortex {
	namespace Core {
		namespace Storage {
			Storage::Storage(Framework* framework) : framework_(framework) {
				this->default_backend_ = "mongo";

				if (this->framework_->get_config().is_object("storage")) {
					maze::object storage_config = this->framework_->get_config()["storage"].get_object();

					if (storage_config.is_string("backend")) {
						this->default_backend_ = storage_config["backend"].get_string();
					}
				}
			}

			Interface::IBackend* Storage::get_backend() {
				return this->get_backend(this->default_backend_);
			}

			Interface::IBackend* Storage::get_backend(std::string backend_name) {
				if (this->available_backends_.size() == 0) {
					return nullptr;
				}

				if (this->available_backends_[0].first == backend_name) {
					return this->available_backends_[0].second;
				}

				for (int i = 1; i < this->available_backends_.size(); ++i) {
					if (this->available_backends_[i].first == backend_name) {
						return this->available_backends_[i].second;
					}
				}

				return nullptr;
			}
		}  // namespace Storage
	}  // namespace Core
}  // namespace Vortex
