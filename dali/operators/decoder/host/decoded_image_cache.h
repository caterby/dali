#ifndef DALI_OPERATORS_DECODER_HOST_DECODED_IMAGE_CACHE_H_
#define DALI_OPERATORS_DECODER_HOST_DECODED_IMAGE_CACHE_H_

#include <cuda_runtime.h>
#include <string>
#include "dali/core/api_helper.h"
#include "dali/core/tensor_shape.h"
#include "dali/core/tensor_view.h"

namespace dali {

class DecodedImageCache {
 public:
  using ImageKey = std::string;
  using ImageShape = TensorShape<3>;
  using DecodedImage = TensorView<StorageCPU, uint8_t, 3>;
   
  explicit DecodedImageCache(const OpSpec& spec);

  static DecodedImageCache& Instance() {
    static DecodedImageCache instance;
    return instance;
  }


  ~DecodedImageCache() = default;

  /**
   * Check whether an image is present in the cache
   * image_key is a string which represents the key of the an image in the cache
   */
  bool IsCached(const ImageKey& image_key);

  /**
   * Get image dimensions, 3 dimenssions(H, W, C)
   * image_key is a string which represents the key of the an image in the cache
   */
  ImageShape& GetShape(const ImageKey& image_key);

    /**
     * Read from cache
     * image_key is a string which represents the key of the an image in the cache
     * destination_data represents the destination buffer
     */
  bool Read(const ImageKey& image_key, void* destination_data);

  /**
   * Add entry to cache.
   * image_key is a string which represents the key of the an image in the cache
   * data_shape dimensions of the data to be stored
   */
  void Add(const ImageKey& image_key, const uint8_t *data, const ImageShape& data_shape);

  /**
   * Get a cache entry describing an image
   * image_key is a string which represents the key of the an image in the cache
   * return Pointer and shape of the cached image; if not found, data is null
   */
  DecodedImage Get(const ImageKey &image_key);

 /**
   * Allocate and get cache
   * Will return the previously allocated cached if the parameters
   * are the same.
   */
  std::shared_ptr<DecodedImageCache> Get(
    int node_id,
    const std::string& cache_policy,
    std::size_t cache_size,
    std::size_t cache_threshold = 0);

 /**
   * Check whether the cache for a given node_id is already initialized
   */
  bool IsInitialized(int node_id);

 protected:
    inline std::size_t bytes_left() const {
        return static_cast<std::size_t>(buffer_end_ - tail_);
    }

    std::size_t cache_size_ = 0;
    std::size_t image_size_threshold_ = 0;
    bool stats_enabled_ = false;
    uint8_t* buffer_end_ = nullptr;
    uint8_t* tail_ = nullptr;

    std::unordered_map<ImageKey, DecodedImage> cache_;
    mutable std::mutex mutex_;

    struct Stats {
        std::size_t decodes = 0;
        std::size_t reads = 0;
        bool is_cached = false;
    };
    mutable std::unordered_map<ImageKey, Stats> stats_;
    bool is_full = false;
    std::size_t total_seen_images_ = 0;

    int node_id_;

    // Strcut: contains parameters to initialize the cache in a node 
    struct DecodedImageCacheParams {
        std::string cache_policy;
        std::size_t cache_size;
        std::size_t cache_threshold;
    };

    // Struct: represents the structure of a cache in each node
    struct DecodedImageCacheInstance {
        std::weak_ptr<DecodedImageCache> cache;
        DecodedImageCacheParams params;
    };

    /**
      * Use the a map to represent the structure of the distributed cache
      * map<int, DecodedImageCacheInstance>
      * int: node_id, used to find the DecodedImageCacheInstance
      * DecodedImageCacheInstance: value of the map, which is a structure 
      */
    std::map<int, DecodedImageCacheInstance> caches_; 

};

}  // namespace dali

#endif  // DALI_OPERATORS_DECODER_HOST_DECODED_IMAGE_CACHE_H_
