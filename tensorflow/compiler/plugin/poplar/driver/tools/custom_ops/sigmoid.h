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

#ifndef TENSORFLOW_COMPILER_PLUGIN_POPLAR_DRIVER_TOOLS_CUSTOM_OPS_SIGMOID_H_
#define TENSORFLOW_COMPILER_PLUGIN_POPLAR_DRIVER_TOOLS_CUSTOM_OPS_SIGMOID_H_

#include "tensorflow/compiler/plugin/poplar/driver/tools/custom_ops/hlo_poplar_instruction.h"

namespace xla {
namespace poplarplugin {

class HloSigmoidInstruction : public HloPoplarInstruction {
 public:
  explicit HloSigmoidInstruction(HloInstruction* operand);

  const HloInstruction* input() const;

  absl::flat_hash_set<int64> AllocatingIndices() const override;
  absl::flat_hash_map<int64, int64> LayoutDependencies() const override;
  uint64 NumberOfInplaceOperands() const override;

  bool IsPopOpsElementwise() const;

 protected:
  std::vector<std::string> ExtraPoplarAttributesToStringImpl(
      const HloPrintOptions& options) const override;

 private:
  std::unique_ptr<HloInstruction> CloneWithNewOperandsImpl(
      const Shape& shape, absl::Span<HloInstruction* const>,
      HloCloneContext*) const override;
};

std::unique_ptr<HloInstruction> CreateSigmoid(HloInstruction* operand);

class HloSigmoidGradInstruction : public HloPoplarInstruction {
 public:
  explicit HloSigmoidGradInstruction(HloInstruction* out, HloInstruction* grad);

  const HloInstruction* out() const;
  const HloInstruction* grad() const;

  absl::flat_hash_set<int64> AllocatingIndices() const override;
  absl::flat_hash_map<int64, int64> LayoutDependencies() const override;
  uint64 NumberOfInplaceOperands() const override;

  bool IsPopOpsElementwise() const;

 protected:
  std::vector<std::string> ExtraPoplarAttributesToStringImpl(
      const HloPrintOptions& options) const override;

 private:
  std::unique_ptr<HloInstruction> CloneWithNewOperandsImpl(
      const Shape& shape, absl::Span<HloInstruction* const>,
      HloCloneContext*) const override;
};

std::unique_ptr<HloInstruction> CreateSigmoidGrad(HloInstruction* relu,
                                                  HloInstruction* grad);

}  // namespace poplarplugin
}  // namespace xla

#endif
