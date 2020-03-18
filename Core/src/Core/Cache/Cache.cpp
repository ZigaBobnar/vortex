#include <Core/Cache/Cache.h>
#include <Core/Exception/CacheException.h>
#ifndef VORTEX_HAS_FEATURE_REDIS
#include <Core/Cache/Backends/RedisBackend.h>
#endif

namespace Vortex {
	namespace Core {
		namespace Cache {
			Cache::Cache() {

			}

			void Cache::initialize(const Maze::Object& cache_config) {
				cache_config_ = cache_config;
				initialized_ = false;

#ifndef VORTEX_HAS_FEATURE_REDIS
				Backends::RedisBackend* redis_backend = static_cast<Backends::RedisBackend*>(Backends::redis_exports.get_backend_instance());
				redis_backend->set_config(cache_config_["config"].get_object()["Redis"].get_object());

				if (redis_backend->is_enabled()) {
					redis_backend->connect();

					available_backends_.push_back(std::make_pair<std::string, ICacheBackend*>(
						Backends::redis_exports.backend_name,
						static_cast<ICacheBackend*>(redis_backend)
					));
				}
#endif




				initialized_ = true;
			}

			const bool Cache::is_initialized() const {
				return initialized_;
			}

			std::string Cache::get(const std::string& key) const {
				return get_backend()->get(key);
			}

			void Cache::set(const std::string& key, const std::string& value, int expire_seconds) const {
				get_backend()->set(key, value, expire_seconds);
			}

			bool Cache::exists(const std::string& key) const {
				return get_backend()->exists(key);
			}

			void Cache::remove(const std::string& key) const {
				get_backend()->remove(key);
			}

			void Cache::set_expiry(const std::string& key, int seconds) const {
				get_backend()->set_expiry(key, seconds);
			}

			ICacheBackend* Cache::get_backend() const {
				return get_backend(default_backend_);
			}

			ICacheBackend* Cache::get_backend(const std::string& backend_name) const {
				if (!initialized_) {
					throw Exception::CacheException("Cache backends are not initialized");
				}

				if (available_backends_.size() == 0) {
					throw Exception::CacheException("No cache backends are available");
				}

				if (available_backends_[0].first == backend_name) {
					return available_backends_[0].second;
				}

				for (int i = 1; i < this->available_backends_.size(); ++i) {
					if (this->available_backends_[i].first == backend_name) {
						return this->available_backends_[i].second;
					}
				}

				throw Exception::CacheException("Cache backend '" + backend_name + "' is not available");
			}
		}  // namespace Cache
	}  // namespace Core
}  // namespace Vortex
