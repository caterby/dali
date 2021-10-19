// Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>
#include "dali/core/convert.h"
#include "dali/core/convert_test_static.h"
#include "dali/core/math_util.h"

namespace dali {


TEST(ConvertSat, float2int) {
  for (int exp = -10; exp < 100; exp++) {
    for (float sig = -256; sig <= 256; sig++) {
      float f = ldexpf(sig, exp);
      float integral;
      float fract = modff(f, &integral);
      if (fract == 0.5f || fract == -0.5f)
        continue;
      double rounded = roundf(f);
      int64_t clamped = clamp<double>(rounded, -128, 127);
      ASSERT_EQ(ConvertSat<int8_t>(f), clamped) << " with f = " << f;
      clamped = clamp<double>(rounded, 0, 255);
      ASSERT_EQ(ConvertSat<uint8_t>(f), clamped) << " with f = " << f;
      clamped = clamp<double>(rounded, -0x8000, 0x7fff);
      ASSERT_EQ(ConvertSat<int16_t>(f), clamped) << " with f = " << f;
      clamped = clamp<double>(rounded, 0, 0xffff);
      ASSERT_EQ(ConvertSat<uint16_t>(f), clamped) << " with f = " << f;
      clamped = clamp<double>(rounded, int32_t(~0x7fffffff), 0x7fffffff);
      ASSERT_EQ(ConvertSat<int32_t>(f), clamped) << " with f = " << f;
      clamped = clamp<double>(rounded, 0, 0xffffffffu);
      ASSERT_EQ(ConvertSat<uint32_t>(f), clamped) << " with f = " << f;
    }
  }
}

TEST(ConvertNorm, int2int) {
  EXPECT_EQ((ConvertNorm<uint8_t, uint8_t>(0)), 0);
  EXPECT_EQ((ConvertNorm<uint8_t, int8_t>(127)), 255);
}

TEST(ConvertNorm, float2int) {
  EXPECT_EQ(ConvertNorm<uint8_t>(0.0f), 0);
  EXPECT_EQ(ConvertNorm<uint8_t>(0.499f), 127);
  EXPECT_EQ(ConvertNorm<uint8_t>(1.0f), 255);
  EXPECT_EQ(ConvertNorm<int8_t>(1.0f), 127);
  EXPECT_EQ(ConvertNorm<int8_t>(0.499f), 63);
  EXPECT_EQ(ConvertNorm<int8_t>(-1.0f), -127);


  EXPECT_EQ(ConvertNorm<uint16_t>(0.0f), 0);
  EXPECT_EQ(ConvertNorm<uint16_t>(1.0f), 0xffff);
  EXPECT_EQ(ConvertNorm<int16_t>(1.0f), 0x7fff);
  EXPECT_EQ(ConvertNorm<int16_t>(-1.0f), -0x7fff);
}

TEST(ConvertSatNorm, float2int) {
  EXPECT_EQ(ConvertSatNorm<uint8_t>(2.0f), 255);
  EXPECT_EQ(ConvertSatNorm<uint8_t>(0.499f), 127);
  EXPECT_EQ(ConvertSatNorm<uint8_t>(-2.0f), 0);
  EXPECT_EQ(ConvertSatNorm<int8_t>(2.0f), 127);
  EXPECT_EQ(ConvertSatNorm<int8_t>(0.499f), 63);
  EXPECT_EQ(ConvertSatNorm<int8_t>(-2.0f), -128);
  EXPECT_EQ(ConvertSatNorm<uint8_t>(0.4f/255), 0);
  EXPECT_EQ(ConvertSatNorm<uint8_t>(0.6f/255), 1);

  EXPECT_EQ(ConvertSatNorm<int16_t>(2.0f), 0x7fff);
  EXPECT_EQ(ConvertSatNorm<int16_t>(-2.0f), -0x8000);
}

TEST(ConvertNorm, int2float) {
  EXPECT_EQ((ConvertNorm<float, uint8_t>(255)), 1.0f);
  EXPECT_NEAR((ConvertNorm<float, uint8_t>(127)), 1.0f*127/255, 1e-7f);
  EXPECT_EQ((ConvertNorm<float, int8_t>(127)), 1.0f);
  EXPECT_NEAR((ConvertNorm<float, int8_t>(64)), 1.0f*64/127, 1e-7f);
}

TEST(Clamp, int64_2_float16) {
  int64_t big_num = 0x0FFFFFFFFFFFFFFF;
  EXPECT_EQ(static_cast<float>(clamp<float16>(big_num)), clamp<float16>(clamp<float>(big_num)));
  EXPECT_EQ(65504.0f, clamp<float16>(big_num));
  EXPECT_EQ(-65504.0f, clamp<float16>(-big_num));
}


}  // namespace dali
