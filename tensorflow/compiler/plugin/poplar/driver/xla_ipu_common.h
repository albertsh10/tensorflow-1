/* Copyright 2017 Graphcore Ltd
 */

/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#ifndef TENSORFLOW_COMPILER_PLUGIN_POPLAR_DRIVER_XLA_IPU_COMMON_H_
#define TENSORFLOW_COMPILER_PLUGIN_POPLAR_DRIVER_XLA_IPU_COMMON_H_

#include "tensorflow/core/framework/types.h"

namespace tensorflow {

const char* const DEVICE_XLA_IPU = "IPU";
const char* const DEVICE_IPU_XLA_JIT = "XLA_IPU_JIT";
const char* const PLATFORM_NAME = "Poplar";

constexpr std::array<DataType, 6> kIpuAllTypes = {
    {DT_INT32, DT_INT64, DT_FLOAT, DT_HALF, DT_BOOL, DT_RESOURCE}};

}  // namespace tensorflow

#endif