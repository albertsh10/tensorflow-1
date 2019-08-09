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

#ifndef TENSORFLOW_COMPILER_PLUGIN_POPLAR_DRIVER_TOOLS_CUSTOM_OPS_DROPOUT_H
#define TENSORFLOW_COMPILER_PLUGIN_POPLAR_DRIVER_TOOLS_CUSTOM_OPS_DROPOUT_H

#include "tensorflow/compiler/plugin/poplar/driver/tools/custom_ops/hlo_poplar_instruction.h"

namespace xla {
namespace poplarplugin {

class HloUserOpInstruction : public HloPoplarInstruction {
 public:
  explicit HloUserOpInstruction(absl::Span<HloInstruction* const> operands,
                                const Shape& shape, const std::string& gp_path,
                                void*, void*, void*);

  absl::flat_hash_set<int64> AllocatingIndices() const override;
  absl::flat_hash_map<int64, int64> LayoutDependencies() const override;
  uint64 NumberOfInplaceOperands() const override;

  bool IsPopOpsElementwise() const;

  size_t NumInputs() const { return num_inputs_; }

  void* GetPointerToFunc() const { return function_ptr_; }

  const std::string& GetPath() const { return gp_path; }

 protected:
  std::vector<string> ExtraPoplarAttributesToStringImpl(
      const HloPrintOptions& options) const override;

 private:
  std::unique_ptr<HloInstruction> CloneWithNewOperandsImpl(
      const Shape& shape, absl::Span<HloInstruction* const>,
      HloCloneContext*) const override;

  // The pointer to the function provided by the user via the shared library.
  void* function_ptr_;

  // The pointer to the function provided by the user via the shared library.
  void* elementwise_ptr_;

  // The pointer to the function provided by the user via the shared library.
  void* allocate_input_ptr_;

  // The number of inputs to this operation.
  size_t num_inputs_;

  std::string gp_path;
};

std::unique_ptr<HloInstruction> CreateUserOp(
    absl::Span<HloInstruction* const> operands, const Shape& shape,
    const std::string& gp_path, void*, void*, void*);

}  // namespace poplarplugin
}  // namespace xla

#endif  // TENSORFLOW_COMPILER_PLUGIN_POPLAR_DRIVER_TOOLS_CUSTOM_OPS_DROPOUT_H
