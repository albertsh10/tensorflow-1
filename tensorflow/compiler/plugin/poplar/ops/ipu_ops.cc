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

#include "tensorflow/core/framework/common_shape_fns.h"
#include "tensorflow/core/framework/op.h"

namespace tensorflow {

REGISTER_OP("IpuConfigureHardware")
    .Attr("config: string")
    .SetIsStateful()
    .Doc("IPU Hardware configuration.");

REGISTER_OP("IpuResetSeed")
    .Attr("device: string")
    .Attr("seed: int")
    .SetIsStateful()
    .Doc("Reset IPU seed.");

REGISTER_OP("IpuEventTrace")
    .Output("out: string")
    .SetIsStateful()
    .Doc("Fetch IPU trace events.");

REGISTER_OP("IpuModelUsed")
    .Output("out: bool")
    .SetIsStateful()
    .Doc(
        "Return true if running on the model or false if real hardware is"
        " used.");

REGISTER_OP("IpuGetConfiguration")
    .Output("out: string")
    .Doc("Return serialized IpuOptions structs.");

REGISTER_OP("IpuGetNumDevices")
    .Attr("device: string")
    .Output("out: int64")
    .SetShapeFn(shape_inference::ScalarShape)
    .Doc("Return the number of IPUs for a specific device string.");
}  // namespace tensorflow
