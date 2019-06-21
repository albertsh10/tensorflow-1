/* Copyright 2019 Graphcore Ltd

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

#include "tensorflow/compiler/plugin/poplar/driver/tools/custom_ops/ipu_inter_copy.h"
#include "tensorflow/compiler/plugin/poplar/kernels/custom_kernels_util.h"
#include "tensorflow/compiler/plugin/poplar/kernels/poplibs_ops.pb.h"

namespace xla {
namespace poplarplugin {
namespace {
Shape MakeIpuInterCopyShape(absl::Span<HloInstruction* const> operands) {
  if (operands.size() > 1) {
    std::vector<Shape> shapes(operands.size());
    absl::c_transform(operands, shapes.begin(),
                      [](HloInstruction* inst) { return inst->shape(); });
    return ShapeUtil::MakeTupleShape(shapes);
  } else {
    CHECK_EQ(operands.size(), 1);
    return operands[0]->shape();
  }
}
}  // namespace

HloIpuInterCopy::HloIpuInterCopy(absl::Span<HloInstruction* const> operands)
    : HloPoplarInstruction(MakeIpuInterCopyShape(operands), operands,
                           GetPoplibsCustomOpTargetString(
                               PoplibsOp::Poputil, PoplibsOp::IpuInterCopy)) {}

absl::flat_hash_set<int64> HloIpuInterCopy::AllocatingIndices() const {
  return {};
}

absl::flat_hash_map<int64, int64> HloIpuInterCopy::LayoutDependencies() const {
  return {};
}
uint64 HloIpuInterCopy::NumberOfInplaceOperands() const { return 0; }

bool HloIpuInterCopy::IsPopOpsElementwise() const { return false; }

std::unique_ptr<HloInstruction> HloIpuInterCopy::CloneWithNewOperandsImpl(
    const Shape& shape, absl::Span<HloInstruction* const> new_operands,
    HloCloneContext* context) const {
  return CreateIpuInterCopy(new_operands);
}

std::unique_ptr<HloInstruction> CreateIpuInterCopy(
    absl::Span<HloInstruction* const> operands) {
  return absl::make_unique<HloIpuInterCopy>(operands);
}

}  // namespace poplarplugin
}  // namespace xla