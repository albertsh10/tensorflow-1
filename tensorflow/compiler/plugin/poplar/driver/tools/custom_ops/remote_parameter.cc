/* Copyright 2020 The TensorFlow Authors. All Rights Reserved.

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

#include "tensorflow/compiler/plugin/poplar/driver/tools/custom_ops/remote_parameter.h"

#include <algorithm>
#include <memory>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "tensorflow/compiler/plugin/poplar/kernels/custom_kernels_util.h"
#include "tensorflow/compiler/plugin/poplar/kernels/ops.pb.h"
#include "tensorflow/compiler/xla/service/hlo_casting_utils.h"
#include "tensorflow/compiler/xla/shape_util.h"

namespace xla {
namespace poplarplugin {

namespace {
Shape ComputePerReplicaLoadShape(Shape remote_buffer_shape,
                                 uint64 replication_factor) {
  if (replication_factor < 2) {
    return remote_buffer_shape;
  }

  const int64 grain_size = 4 / ShapeUtil::ByteSizeOfPrimitiveType(
                                   remote_buffer_shape.element_type());

  // Pad the element count appropriately
  const int64 element_count =
      grain_size *
      tensorflow::MathUtil::CeilOfRatio<int64>(
          tensorflow::MathUtil::CeilOfRatio<int64>(
              ShapeUtil::ElementsIn(remote_buffer_shape), grain_size),
          replication_factor);

  return ShapeUtil::MakeShape(remote_buffer_shape.element_type(),
                              {element_count});
}

Shape ComputePerReplicaLoadShape(absl::Span<HloInstruction* const> rbuffers,
                                 std::vector<uint64> replication_factors) {
  CHECK_EQ(rbuffers.size(), replication_factors.size());
  std::vector<Shape> result_shape(rbuffers.size());
  for (int64 i = 0; i != rbuffers.size(); ++i) {
    result_shape[i] = ComputePerReplicaLoadShape(rbuffers[i]->shape(),
                                                 replication_factors[i]);
  }

  return rbuffers.size() == 1 ? result_shape[0]
                              : ShapeUtil::MakeTupleShape(result_shape);
}

Shape ComputePerReplicaStoreShape(
    absl::Span<HloInstruction* const> rbuffers_and_values,
    std::vector<uint64> replication_factors) {
  auto rbuffers =
      rbuffers_and_values.subspan(0, rbuffers_and_values.size() / 2);

  std::vector<Shape> result_shape(rbuffers.size());
  absl::c_transform(
      rbuffers, result_shape.begin(),
      [](HloInstruction* const rbuffer) { return rbuffer->shape(); });

  return rbuffers.size() == 1 ? result_shape[0]
                              : ShapeUtil::MakeTupleShape(result_shape);
}
}  // namespace

HloRemoteParameterLoad::HloRemoteParameterLoad(
    absl::Span<HloInstruction* const> rbuffers,
    std::vector<uint64> replication_factors)
    : HloPoplarInstruction(
          ComputePerReplicaLoadShape(rbuffers, replication_factors), rbuffers,
          PoplarOp::RemoteParameterLoad,
          absl::StrJoin(replication_factors, ".")),
      replication_factors_(replication_factors) {
  CHECK_EQ(rbuffers.size(), replication_factors.size());
}

absl::flat_hash_set<int64> HloRemoteParameterLoad::AllocatingIndices() const {
  return {};
}

absl::flat_hash_map<int64, int64> HloRemoteParameterLoad::LayoutDependencies()
    const {
  return {};
}

uint64 HloRemoteParameterLoad::NumberOfInplaceOperands() const { return 0; }

bool HloRemoteParameterLoad::IsPopOpsElementwise() const { return false; }

std::unique_ptr<HloInstruction>
HloRemoteParameterLoad::CloneWithNewOperandsImpl(
    const Shape& shape, absl::Span<HloInstruction* const> operands,
    HloCloneContext*) const {
  return CreateHloRemoteParameterLoad(operands, replication_factors_);
}

std::vector<std::string>
HloRemoteParameterLoad::ExtraPoplarAttributesToStringImpl(
    const HloPrintOptions& options) const {
  return {"replication_factors=" + absl::StrJoin(replication_factors_, ", ")};
}

std::unique_ptr<HloInstruction> CreateHloRemoteParameterLoad(
    absl::Span<HloInstruction* const> rbuffers,
    std::vector<uint64> replication_factors) {
  return absl::make_unique<HloRemoteParameterLoad>(rbuffers,
                                                   replication_factors);
}

HloRemoteParameterStore::HloRemoteParameterStore(
    absl::Span<HloInstruction* const> rbuffers_and_values,
    std::vector<uint64> replication_factors)
    : HloPoplarInstruction(
          ComputePerReplicaStoreShape(rbuffers_and_values, replication_factors),
          rbuffers_and_values, PoplarOp::RemoteParameterStore,
          absl::StrJoin(replication_factors, ".")),
      replication_factors_(replication_factors) {
  // The first half of the operands are the remote buffers, the second half
  // are the corresponding values to store in the buffers.
  CHECK_GE(rbuffers_and_values.size(), 2);
  CHECK_EQ(rbuffers_and_values.size() % 2, 0);
  CHECK_EQ(rbuffers_and_values.size() / 2, replication_factors.size());
  set_custom_call_has_side_effect(true);
}

absl::flat_hash_set<int64> HloRemoteParameterStore::AllocatingIndices() const {
  return {};
}

absl::flat_hash_map<int64, int64> HloRemoteParameterStore::LayoutDependencies()
    const {
  return {};
}

uint64 HloRemoteParameterStore::NumberOfInplaceOperands() const {
  // The remote buffers are in-place, but not the values.
  return RemoteBuffers().size();
}

bool HloRemoteParameterStore::IsPopOpsElementwise() const { return false; }

absl::Span<HloInstruction* const> HloRemoteParameterStore::RemoteBuffers()
    const {
  return absl::MakeSpan(operands()).first(operand_count() / 2);
}

absl::Span<HloInstruction* const> HloRemoteParameterStore::ValuesToStore()
    const {
  return absl::MakeSpan(operands()).last(operand_count() / 2);
}

std::unique_ptr<HloInstruction>
HloRemoteParameterStore::CloneWithNewOperandsImpl(
    const Shape& shape, absl::Span<HloInstruction* const> operands,
    HloCloneContext*) const {
  return CreateHloRemoteParameterStore(operands, replication_factors_);
}

std::vector<std::string>
HloRemoteParameterStore::ExtraPoplarAttributesToStringImpl(
    const HloPrintOptions& options) const {
  return {"replication_factors=" + absl::StrJoin(replication_factors_, ", ")};
}

std::unique_ptr<HloInstruction> CreateHloRemoteParameterStore(
    absl::Span<HloInstruction* const> rbuffers_and_values,
    std::vector<uint64> replication_factors) {
  return absl::make_unique<HloRemoteParameterStore>(rbuffers_and_values,
                                                    replication_factors);
}

namespace {
StatusOr<std::unique_ptr<HloInstruction>> HloRemoteParameterLoadFactoryFunc(
    HloCustomCallInstruction* call) {
  if (call->operand_count() != 1) {
    return FailedPrecondition(
        "Expected remote buffer load to have one operand, but got %d",
        call->operand_count());
  }
  if (call->mutable_operand(0)->opcode() != HloOpcode::kParameter) {
    return FailedPrecondition("Can only remote buffer load from a parameter");
  }
  auto attribute_map = IPUCustomKernelsUtil::AttributeMap(call);
  TF_ASSIGN_OR_RETURN(uint64 replication_factor,
                      attribute_map.GetAttributeAsUInt64("replication_factor"));

  return CreateHloRemoteParameterLoad(call->operands(), {replication_factor});
}

static HloPoplarInstructionFactory remote_parameter_load_factory(
    PoplarOp::RemoteParameterLoad, HloRemoteParameterLoadFactoryFunc);

StatusOr<std::unique_ptr<HloInstruction>> HloRemoteParameterStoreFactoryFunc(
    HloCustomCallInstruction* call) {
  if (call->operand_count() != 2) {
    return FailedPrecondition(
        "Expected remote buffer store to have two operands, but got %d",
        call->operand_count());
  }
  if (call->mutable_operand(0)->opcode() != HloOpcode::kParameter) {
    return FailedPrecondition("Can only remote buffer store to a parameter");
  }
  auto attribute_map = IPUCustomKernelsUtil::AttributeMap(call);
  TF_ASSIGN_OR_RETURN(uint64 replication_factor,
                      attribute_map.GetAttributeAsUInt64("replication_factor"));

  return CreateHloRemoteParameterStore(
      {call->mutable_operand(0), call->mutable_operand(1)},
      {replication_factor});
}

static HloPoplarInstructionFactory remote_parameter_store_factory(
    PoplarOp::RemoteParameterStore, HloRemoteParameterStoreFactoryFunc);
}  // namespace

HloCreateBuffer::HloCreateBuffer(const Shape& shape, bool is_remote)
    : HloPoplarInstruction(shape, {}, PoplarOp::CreateBuffer, is_remote),
      is_remote_(is_remote) {
  CHECK(!shape.IsTuple());
  // Set the instruction to have side effect to prevent it from being merged
  // with other similarly shaped buffers.
  set_custom_call_has_side_effect(true);
}

absl::flat_hash_set<int64> HloCreateBuffer::AllocatingIndices() const {
  return {};
}

absl::flat_hash_map<int64, int64> HloCreateBuffer::LayoutDependencies() const {
  return {};
}

uint64 HloCreateBuffer::NumberOfInplaceOperands() const { return 0; }

bool HloCreateBuffer::IsPopOpsElementwise() const { return false; }

std::unique_ptr<HloInstruction> HloCreateBuffer::CloneWithNewOperandsImpl(
    const Shape& shape, absl::Span<HloInstruction* const> operands,
    HloCloneContext*) const {
  CHECK_EQ(operands.size(), 0);
  return CreateHloCreateBuffer(shape, IsRemoteBuffer());
}

std::vector<std::string> HloCreateBuffer::ExtraPoplarAttributesToStringImpl(
    const HloPrintOptions& options) const {
  std::vector<std::string> attributes;
  attributes.push_back("is_remote=" + std::to_string(is_remote_));
  return attributes;
}

std::unique_ptr<HloInstruction> CreateHloCreateBuffer(const Shape& shape,
                                                      bool is_remote) {
  return absl::make_unique<HloCreateBuffer>(shape, is_remote);
}

namespace {
StatusOr<std::unique_ptr<HloInstruction>> HloCreateBufferFactoryFunc(
    HloCustomCallInstruction* call) {
  auto attribute_map = IPUCustomKernelsUtil::AttributeMap(call);
  TF_ASSIGN_OR_RETURN(bool is_remote,
                      attribute_map.GetAttributeAsBool("is_remote"));
  return CreateHloCreateBuffer(call->shape(), is_remote);
}
static HloPoplarInstructionFactory create_buffer_factory(
    PoplarOp::CreateBuffer, HloCreateBufferFactoryFunc);
}  // namespace

HloBufferLoadSlice::HloBufferLoadSlice(const Shape& shape,
                                       HloInstruction* const buffer,
                                       HloInstruction* const offset)
    : HloPoplarInstruction(shape, {buffer, offset}, PoplarOp::BufferLoadSlice) {
}

std::unique_ptr<HloInstruction> HloBufferLoadSlice::CloneWithNewOperandsImpl(
    const Shape& shape, absl::Span<HloInstruction* const> operands,
    HloCloneContext*) const {
  CHECK_EQ(operands.size(), 2);
  return absl::make_unique<HloBufferLoadSlice>(shape, operands[0], operands[1]);
}

std::vector<std::string> HloBufferLoadSlice::ExtraPoplarAttributesToStringImpl(
    const HloPrintOptions& options) const {
  return {};
}

std::unique_ptr<HloInstruction> CreateBufferLoadSlice(
    const Shape& shape, HloInstruction* const buffer,
    HloInstruction* const offset) {
  return absl::make_unique<HloBufferLoadSlice>(shape, buffer, offset);
}

HloBufferStoreSlice::HloBufferStoreSlice(HloInstruction* const buffer,
                                         HloInstruction* const slice,
                                         HloInstruction* const offset)
    : HloPoplarInstruction(buffer->shape(), {buffer, slice, offset},
                           PoplarOp::BufferStoreSlice) {
  set_custom_call_has_side_effect(true);
}

std::unique_ptr<HloInstruction> HloBufferStoreSlice::CloneWithNewOperandsImpl(
    const Shape& shape, absl::Span<HloInstruction* const> operands,
    HloCloneContext*) const {
  CHECK_EQ(operands.size(), 3);
  return absl::make_unique<HloBufferStoreSlice>(operands[0], operands[1],
                                                operands[2]);
}

std::vector<std::string> HloBufferStoreSlice::ExtraPoplarAttributesToStringImpl(
    const HloPrintOptions& options) const {
  return {};
}

std::unique_ptr<HloInstruction> CreateBufferStoreSlice(
    HloInstruction* const buffer, HloInstruction* const slice,
    HloInstruction* const offset) {
  return absl::make_unique<HloBufferStoreSlice>(buffer, slice, offset);
}
}  // namespace poplarplugin
}  // namespace xla
