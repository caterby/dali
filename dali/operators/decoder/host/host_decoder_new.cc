#include <opencv2/opencv.hpp>
#include <tuple>
#include <memory>
#include "dali/image/image_factory.h"
#include "dali/operators/decoder/host/host_decoder_new.h"
#include <chrono>

#define __DEBUG

#include "dali/operators/decoder/host/decoded_image_cache.h"
#include "/home/jieliu1992/veloc/src/common/debug.hpp"

namespace dali {

void HostDecoder::RunImpl(SampleWorkspace &ws) {
  //DBG("");
  const auto &input = ws.Input<CPUBackend>(0);
  auto &output = ws.Output<CPUBackend>(0);
  auto file_name = input.GetSourceInfo();

  //std::cout << "##### file_name is  #####" << file_name << std::endl;

  // Verify input
  DALI_ENFORCE(input.ndim() == 1,
                "Input must be 1D encoded jpeg string.");
  DALI_ENFORCE(IsType<uint8>(input.type()),
                "Input must be stored as uint8 data.");

  std::unique_ptr<Image> img;
  
  /*TODO Add the epoch and node_id information
   * If the epoch number is 0, initialize the cache based on the node_id, then cache the decoded images;
   * else: check whether the decoded image is cached in the local cache or the remote cache.
   * This part is the similiar with the raw data cache part.
   */

  auto start = std::chrono::steady_clock::now();
  try {
    img = ImageFactory::CreateImage(input.data<uint8>(), input.size(), output_type_);
    img->SetCropWindowGenerator(GetCropWindowGenerator(ws.data_idx()));
    img->SetUseFastIdct(use_fast_idct_);
    img->Decode();
  } catch (std::exception &e) {
    DALI_FAIL(e.what() + ". File: " + file_name);
  }
  const auto decoded = img->GetImage();
  const auto shape = img->GetShape();
  auto end = std::chrono::steady_clock::now();
  std::chrono::duration<double> diff = end - start;
  std::cout << "##### Decoding: #####" << file_name << ":" << diff.count() << std::endl;
  //TIMER_STOP(timer, "")
  //std::cout << "##### decoded after GetImage #####" << decoded.get() << std::endl;
  //std::cout << "##### shape after GetImage  #####" << shape << std::endl;

  //CachedDecoderImpl cdi (const OpSpec& spec);
  //cdi.CacheStore(file_name, decoded.get(), shape);

  output.Resize(shape);
  output.SetLayout("HWC");
  unsigned char *out_data = output.mutable_data<unsigned char>();
  std::memcpy(out_data, decoded.get(), volume(shape));
}

DALI_REGISTER_OPERATOR(decoders__Image, HostDecoder, CPU);
DALI_REGISTER_OPERATOR(ImageDecoder, HostDecoder, CPU);

}  // namespace dali
