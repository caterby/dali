#include "dali/operators/decoder/host/decoded_image_cache.h"
#include <fstream>
#include <mutex>
#include <unordered_map>
#include "dali/core/error_handling.h"
#include "dali/core/span.h"
#include "dali/kernels/alloc.h"
#include "dali/pipeline/data/backend.h"
#include "dali/core/mm/memory.h"

namespace dali {

DecodedImageCache::DecodedImageCache(std::size_t cache_size, std::size_t image_size_threshold, bool stats_enabled)
    : cache_size_(cache_size)
    , image_size_threshold_(image_size_threshold)
    , stats_enabled_(stats_enabled) {

  buffer_ = mm::memory::alloc_raw_unique<uint8_t, mm::memory_kind::host>(cache_size_);
  tail_ = buffer_.get();
  buffer_end_ = buffer_.get() + cache_size_;
  std::cout << "cache size is " << cache_size_ / (1024 * 1024) << " MB" << std::endl;

}

DecodedImageCache::~DecodedImageCache() {
}

bool DecodedImageCache::IsCached(const ImageKey& image_key) const {
  std::lock_guard<std::mutex> lock(mutex_);
  return cache_.find(image_key) != cache_.end();
}

ImageShape& DecodedImageCache::GetShape(const ImageKey& image_key){
  std::lock_guard<std::mutex> lock(mutex_);
  const auto it = cache_.find(image_key);
  return it->second.shape;
}

DecodedImage& DecodedImageCache::Read(const ImageKey& image_key) const {
  std::cout << "##### The Read function in DecodedImageCache#####" << std::endl;
  
  std::lock_guard<std::mutex> lock(mutex_);
  
  std::cout << "Read: image_key[" << image_key << "]" << std::endl;
  const auto it = cache_.find(image_key);
  if (it == cache_.end())
    return false;
  const auto& data = it->second;
  const auto n = data.num_elements();

  if (stats_enabled_) stats_[image_key].reads++;
  return data;
}

DecodedImage& DecodedImageCache::Get(const ImageKey& image_key) const {
  std::cout << "+++++ The Get function in decoder/host/image_cache_blob.cc +++++" << std::endl;
  std::lock_guard<std::mutex> lock(mutex_);
  std::cout << "Get: image_key[" << image_key << "]" << std::endl;
  const auto it = cache_.find(image_key);
  if (it == cache_.end())
    return {};
  if (stats_enabled_) stats_[image_key].reads++;
  auto ret = it->second;  // make a copy _before_ leaving the mutex
  return ret;
}

// Allocate and get the cache
std::shared_ptr<DecodedImageCache> DecodedImageCache::Get(int node_id,
                                                          const std::string& cache_policy,
                                                          std::size_t cache_size,
                                                          std::size_t cache_threshold) {
  std::lock_guard<std::mutex> lock(mutex_);
  const DecodedImageCacheParams params{cache_policy, cache_size, cache_threshold};
  auto &instance = caches_[node_id];
  auto cache = instance.cache.lock();

  if (!cache) {
    caches_[device_id] = {cache, params};
    return cache;
  }
  return cache;
}

// Get the cache based on the node_id
std::shared_ptr<DecodedImageCache> DecodedImageCache::Get(int node_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  return caches_[device_id].cache.lock();
}


// Check if the cache initialized
bool DecodedImageCache::IsInitialized(int node_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = caches_.find(node_id);

  return it != caches_.end();
}


void DecodedImageCache::Add(const ImageKey& image_key, const uint8_t* data,
                            const ImageShape& data_shape) {
  	std::cout << "##### The Add function  #####" << std::endl;
  	std::lock_guard<std::mutex> lock(mutex_);

  	const std::size_t data_size = volume(data_shape);
  	if (stats_enabled_) stats_[image_key].decodes++;
  	if (data_size < image_size_threshold_) return;

  	if (cache_.find(image_key) != cache_.end())
    		return;

  	if (bytes_left() < data_size) {
    		std::cout << "##### WARNING: not enough space in cache. #####" << std::endl;
    		if (stats_enabled_) is_full = true;
    			return;
  	}

  	size_ = data_size;
	//cout << "will write from file " << from_file << " size " << bytes_to_write << endl;
	if (fd_ < 0){
		errno = EINVAL;
		cerr << "File " << name_ << " has invalid decriptor" << endl;
		return -1;
	}
	ftruncate(fd_, bytes_to_write);

	//mmap the shm file to get ptr
	void *ptr = nullptr;
	if ((ptr = mmap(0, bytes_to_write, PROT_WRITE, MAP_SHARED, fd_, 0)) == MAP_FAILED){
		cerr << "mmap error" << endl;
		return -1;
	}


	// write to shared memory segment
	// We will mmap the file to read from, because
	// in DALI, the file to be read will be mmaped first.
	void *ptr_from = nullptr;
	int fd_from = -1;
	if ((fd_from = open(from_file.c_str(), O_RDONLY)) < 0) {
		cerr << "Open failed" << endl;
		return -1;
	}
	if ((ptr_from = mmap(0, bytes_to_write, PROT_READ, MAP_SHARED, fd_from, 0)) == MAP_FAILED){
		cerr << "mmap error" << endl;
		return -1;
	}
	std::shared_ptr<void> p_;
	p_ = shared_ptr<void>(ptr_from, [=](void*) {
		munmap(ptr_from, bytes_to_write);
	});

	//Do the memcpy now
	// memcpy(void* dest, const void* src, size)
	memcpy(ptr, p_.get(), bytes_to_write);
	//cout << "memcpy done" << endl;
	int ret = 0;

	// Now unmap both files
	if ((ret = munmap(ptr, bytes_to_write)) == -1){
		cerr << "Munmap failed" << endl;
		return -1;
	}

	if ((ret = munmap(ptr_from, bytes_to_write)) == -1){
		cerr << "Munmap failed" << endl;
		return -1;
	}

	close(fd_from);
  	//close the tmp file
  	close(fd_);



  	cache_[image_key] = {tail_, data_shape};
  	tail_ += data_size;

  	if (stats_enabled_) stats_[image_key].is_cached = true;
}


}  // namespace dali
