#ifndef DALI_OPERATORS_DECODER_HOST_HOST_DECODER_NEW_H_
#define DALI_OPERATORS_DECODER_HOST_HOST_DECODER_NEW_H_

#include <vector>

#include "dali/core/common.h"
#include "dali/core/error_handling.h"
#include "dali/pipeline/operator/operator.h"
#include "dali/util/crop_window.h"

namespace dali {

class HostDecoderNew : public Operator<CPUBackend> {
 public:
  explicit inline HostDecoderNew(const OpSpec &spec) :
      Operator<CPUBackend>(spec),
      output_type_(spec.GetArgument<DALIImageType>("output_type")),
      use_fast_idct_(spec.GetArgument<bool>("use_fast_idct"))
  {}

  inline ~HostDecoderNew() override = default;
  DISABLE_COPY_MOVE_ASSIGN(HostDecoderNew);

 protected:
  bool SetupImpl(std::vector<OutputDesc> &output_desc, const HostWorkspace &ws) override {
    return false;
  }

  void RunImpl(SampleWorkspace &ws) override;

  virtual CropWindowGenerator GetCropWindowGenerator(int data_idx) const {
    return {};
  }

  DALIImageType output_type_;
  bool use_fast_idct_ = false;
};

}  // namespace dali

#endif  // DALI_OPERATORS_DECODER_HOST_HOST_DECODER_NEW_H_
