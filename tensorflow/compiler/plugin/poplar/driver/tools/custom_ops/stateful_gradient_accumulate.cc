/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

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

#include "tensorflow/compiler/plugin/poplar/driver/tools/custom_ops/stateful_gradient_accumulate.h"

#include <memory>

#include "tensorflow/compiler/plugin/poplar/kernels/custom_kernels_util.h"
#include "tensorflow/compiler/plugin/poplar/kernels/ops.pb.h"

namespace xla {
namespace poplarplugin {

HloStatefulGradientAccumulate::HloStatefulGradientAccumulate(
    absl::Span<HloInstruction* const> operands, int32 num_mini_batches,
    PoplarOp op)
    : HloPoplarInstruction(GetHloPoplarInstructionShape(operands), operands, op,
                           num_mini_batches),
      num_mini_batches_(num_mini_batches) {}

absl::flat_hash_set<int64> HloStatefulGradientAccumulate::AllocatingIndices()
    const {
  return {};
}

absl::flat_hash_map<int64, int64>
HloStatefulGradientAccumulate::LayoutDependencies() const {
  return {};
}

uint64 HloStatefulGradientAccumulate::NumberOfInplaceOperands() const {
  return operand_count();
}

bool HloStatefulGradientAccumulate::IsPopOpsElementwise() const {
  return false;
}

std::unique_ptr<HloInstruction>
HloStatefulGradientAccumulate::CloneWithNewOperandsImpl(
    const Shape& shape, absl::Span<HloInstruction* const> new_operands,
    HloCloneContext*) const {
  return absl::make_unique<HloStatefulGradientAccumulate>(new_operands,
                                                          num_mini_batches_);
}

std::vector<std::string>
HloStatefulGradientAccumulate::ExtraPoplarAttributesToStringImpl(
    const HloPrintOptions& options) const {
  std::vector<std::string> attributes;
  attributes.push_back("num_mini_batches=" + std::to_string(num_mini_batches_));

  return attributes;
}

std::unique_ptr<HloInstruction> CreateStatefulGradientAccumulation(
    absl::Span<HloInstruction* const> operands, int32 num_mini_batches) {
  return absl::make_unique<HloStatefulGradientAccumulate>(operands,
                                                          num_mini_batches);
}

HloStatefulGradientAccumulateAndAllReduce::
    HloStatefulGradientAccumulateAndAllReduce(
        absl::Span<HloInstruction* const> operands, int32 num_mini_batches)
    : HloStatefulGradientAccumulate(
          operands, num_mini_batches,
          PoplarOp::StatefulGradientAccumulateAndAllReduce) {}

std::unique_ptr<HloInstruction>
HloStatefulGradientAccumulateAndAllReduce::CloneWithNewOperandsImpl(
    const Shape& shape, absl::Span<HloInstruction* const> new_operands,
    HloCloneContext*) const {
  return absl::make_unique<HloStatefulGradientAccumulateAndAllReduce>(
      new_operands, num_mini_batches_);
}

std::unique_ptr<HloInstruction> CreateStatefulGradientAccumulateAndAllReduce(
    absl::Span<HloInstruction* const> operands, int32 num_mini_batches) {
  return absl::make_unique<HloStatefulGradientAccumulateAndAllReduce>(
      operands, num_mini_batches);
}

HloPipelineStatefulGradientAccumulate::HloPipelineStatefulGradientAccumulate(
    absl::Span<HloInstruction* const> operands, int32 num_mini_batches)
    : HloStatefulGradientAccumulate(
          operands, num_mini_batches,
          PoplarOp::PipelineStatefulGradientAccumulate) {}

uint64 HloPipelineStatefulGradientAccumulate::NumberOfInplaceOperands() const {
  return 0;
}

std::unique_ptr<HloInstruction>
HloPipelineStatefulGradientAccumulate::CloneWithNewOperandsImpl(
    const Shape& shape, absl::Span<HloInstruction* const> new_operands,
    HloCloneContext*) const {
  return absl::make_unique<HloPipelineStatefulGradientAccumulate>(
      new_operands, num_mini_batches_);
}

std::unique_ptr<HloInstruction> CreatePipelineStatefulGradientAccumulation(
    absl::Span<HloInstruction* const> operands, int32 num_mini_batches) {
  return absl::make_unique<HloPipelineStatefulGradientAccumulate>(
      operands, num_mini_batches);
}

namespace {

StatusOr<std::unique_ptr<HloInstruction>>
HloStatefulGradientAccumulateFactoryFunc(HloCustomCallInstruction* call) {
  auto attribute_map = IPUCustomKernelsUtil::AttributeMap(call);
  // Get the attribute values
  TF_ASSIGN_OR_RETURN(int32 num_mini_batches,
                      attribute_map.GetAttributeAsInt("num_mini_batches"));

  return CreateStatefulGradientAccumulation(call->operands(), num_mini_batches);
}

static HloPoplarInstructionFactory stateful_gradient_accumulate_factory(
    PoplarOp::StatefulGradientAccumulate,
    HloStatefulGradientAccumulateFactoryFunc);

StatusOr<std::unique_ptr<HloInstruction>>
HloPipelineStatefulGradientAccumulateFactoryFunc(
    HloCustomCallInstruction* call) {
  auto attribute_map = IPUCustomKernelsUtil::AttributeMap(call);
  // Get the attribute values
  TF_ASSIGN_OR_RETURN(int32 num_mini_batches,
                      attribute_map.GetAttributeAsInt("num_mini_batches"));

  return CreatePipelineStatefulGradientAccumulation(call->operands(),
                                                    num_mini_batches);
}

static HloPoplarInstructionFactory
    pipeline_stateful_gradient_accumulate_factory(
        PoplarOp::PipelineStatefulGradientAccumulate,
        HloPipelineStatefulGradientAccumulateFactoryFunc);

}  // namespace

}  // namespace poplarplugin
}  // namespace xla
