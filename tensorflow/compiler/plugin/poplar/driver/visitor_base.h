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

#ifndef TENSORFLOW_COMPILER_PLUGIN_POPLAR_DRIVER_VISITOR_BASE_H_
#define TENSORFLOW_COMPILER_PLUGIN_POPLAR_DRIVER_VISITOR_BASE_H_

#include "tensorflow/compiler/xla/service/dfs_hlo_visitor.h"
#include "tensorflow/compiler/plugin/poplar/driver/ops.h"

namespace xla {
namespace poplarplugin {

struct CompilerResources;

/*
 * The base visitor handles all operations that are element-wise.
 * This includes all explicitly element-wise ops, and also operations
 * Select, Convert, Clamp, Rng, Constant.  All of these have no element
 * to element dependencies.
 */
class BaseVisitor : public DfsHloVisitor {
public:
  BaseVisitor(poplar::Graph* graph, CompilerResources&);

  virtual const Shape& GetOutputShape(HloInstruction*) const;

  Status HandleElementwiseUnary(HloInstruction* inst) override;

  Status HandleElementwiseBinary(HloInstruction* inst) override;

  Status HandleConvert(HloInstruction* inst) override;

  Status HandleClamp(HloInstruction* inst,
                     HloInstruction* min,
                     HloInstruction* arg,
                     HloInstruction* max) override;

  Status HandleSelect(HloInstruction* inst,
                      HloInstruction* pred,
                      HloInstruction* on_true,
                      HloInstruction* on_false) override;

  Status HandleConcatenate(
          HloInstruction* inst,
          tensorflow::gtl::ArraySlice<HloInstruction*> operands) override;

  Status HandleCopy(HloInstruction* inst) override;

  Status HandleDot(HloInstruction* inst,
                   HloInstruction* lhs,
                   HloInstruction* rhs) override;

  Status HandleConvolution(HloInstruction* inst,
                           HloInstruction* lhs,
                           HloInstruction* rhs,
                           const Window& window) override;

  Status HandleCrossReplicaSum(HloInstruction* crs) override;

  Status HandleRng(HloInstruction* inst,
                   RandomDistribution distribution) override;

  Status HandleReverse(HloInstruction* inst,
                       HloInstruction* operand) override;

  Status HandleSort(HloInstruction* inst,
                    HloInstruction* operand) override;

  Status HandleConstant(HloInstruction* inst,
                        const Literal& literal) override;

  Status HandleGetTupleElement(HloInstruction* inst,
                               HloInstruction* operand) override;

  Status HandleReduce(HloInstruction* inst,
                      HloInstruction* arg,
                      HloInstruction* init_value,
                      tensorflow::gtl::ArraySlice<int64> dimensions,
                      HloComputation* function) override;

  Status HandleBitcast(HloInstruction* inst) override;

  Status HandleBroadcast(HloInstruction* inst) override;

  Status HandleReshape(HloInstruction* inst) override;

  Status HandleTranspose(HloInstruction* inst) override;

  Status HandleFusion(HloInstruction* inst) override;

  Status HandleCall(HloInstruction* inst) override;

  Status HandleCustomCall(HloInstruction* inst,
                          tensorflow::gtl::ArraySlice<HloInstruction*> operands,
                          tensorflow::StringPiece custom_call_target) override;

  Status HandleSlice(HloInstruction* inst,
                     HloInstruction* operand) override;

  Status HandleDynamicSlice(HloInstruction* dynamic_slice,
                            HloInstruction* operand,
                            HloInstruction* start_indices) override;

  Status HandleDynamicUpdateSlice(HloInstruction* inst,
                                  HloInstruction* operand,
                                  HloInstruction* update,
                                  HloInstruction* start_indices) override;

  Status HandleTuple(HloInstruction* inst,
                     tensorflow::gtl::ArraySlice<HloInstruction*> operands) override;

  Status HandleMap(HloInstruction* inst,
                   tensorflow::gtl::ArraySlice<HloInstruction*> operands,
                   HloComputation* function,
                   tensorflow::gtl::ArraySlice<HloInstruction*> static_operands) override;

  Status HandleReduceWindow(HloInstruction* inst,
                            HloInstruction* operand,
                            const Window& window,
                            HloComputation* function) override;

  Status HandleSelectAndScatter(HloInstruction* inst) override;

  Status HandleWhile(HloInstruction* inst) override;

  Status HandlePad(HloInstruction* inst) override;

  Status HandleReducePrecision(HloInstruction* reduce_precision) override;

  Status HandleInfeed(HloInstruction* inst) override;

  Status HandleOutfeed(HloInstruction* inst) override;

  Status HandleSend(HloInstruction* inst) override;

  Status HandleRecv(HloInstruction* inst) override;

  Status HandleBatchNormInference(HloInstruction* batchNormInference) override;

  Status HandleBatchNormTraining(HloInstruction* batchNormTraining) override;

  Status HandleBatchNormGrad(HloInstruction* batchNormGrad) override;

  TensorMap tensor_map;

  poplar::program::Sequence sequence;

protected:
  Status Unimplemented(HloInstruction* inst);

  poplar::Graph* graph_;

  CompilerResources& resources_;

};

}  // namespace poplarplugin
}  // namespace xla

#endif
