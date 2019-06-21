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

#ifndef TENSORFLOW_COMPILER_PLUGIN_POPLAR_DRIVER_COMPILER_RESOURCES_H_
#define TENSORFLOW_COMPILER_PLUGIN_POPLAR_DRIVER_COMPILER_RESOURCES_H_

#include "tensorflow/compiler/plugin/poplar/driver/compiler_annotations.h"
#include "tensorflow/compiler/plugin/poplar/driver/compiler_information.h"
#include "tensorflow/compiler/plugin/poplar/driver/ops/conv_graph_caching.h"
#include "tensorflow/compiler/plugin/poplar/driver/ops/dot_graph_caching.h"
#include "tensorflow/compiler/plugin/poplar/driver/ops/norm_graph_caching.h"
#include "tensorflow/compiler/plugin/poplar/driver/passes/convolution_classifier.h"
#include "tensorflow/compiler/plugin/poplar/driver/tools/mapping_helper.h"
#include "tensorflow/compiler/plugin/poplar/driver/visitors/visitor_subcomputation.h"

#include <poplar/OptionFlags.hpp>
#include <poplin/Convolution.hpp>
#include <poplin/MatMul.hpp>
#include <poprand/RandomGen.hpp>
#include <poputil/GraphFunction.hpp>

namespace xla {
namespace poplarplugin {

using ComputationMap =
    std::map<const HloComputation*, std::shared_ptr<SubComputationVisitor>>;

// This structure contains additional information required to lower the graph
// from an XLA graph to a poplar graph.
struct CompilerResources {
  poplar::Graph main_graph;

  absl::optional<poplar::Graph> replicated_graph;

  std::vector<poplar::Graph> shard_graphs;

  ComputationMap computation_map;

  CompilerAnnotations annotations;

  CompilerInformation information;

  poplin::PlanningCache convolution_cache;

  poplin::matmul::PlanningCache dot_cache;

  const poplar::OptionFlags default_conv_options;

  const poplar::OptionFlags default_pooling_options;

  bool disable_graph_convolution_caching;

  uint32 replication_factor;

  bool merge_infeed_io_copies;

  std::map<std::string, TensorMap> tensor_maps;

  LinearMapperState linear_mapping_state;

  conv_graph_caching::ConvolutionGraphCache conv_graph_cache;

  conv_graph_caching::BwdWeightGraphCache bwd_weight_graph_cache;

  conv_graph_caching::ConvolutionScaledInplaceGraphCache
      conv_scaled_inplace_graph_cache;

  conv_graph_caching::BiasApplyGraphCache bias_apply_graph_cache;

  norm_graph_caching::NormInferenceGraphCache norm_inf_graph_cache;

  norm_graph_caching::NormTrainingGraphCache norm_tr_graph_cache;

  norm_graph_caching::NormGradGraphCache norm_grad_graph_cache;

  norm_graph_caching::NormStatisticsGraphCache norm_statistics_graph_cache;

  dot_graph_caching::DotGraphCache dot_graph_cache;

  CompilerResources(const poplar::Device& dev,
                    const poplar::OptionFlags& conv_options,
                    const poplar::OptionFlags& pooling_options,
                    bool disable_graph_convolution_caching,
                    bool merge_infeed_io_copies, uint32 replication_factor,
                    int64 max_all_reduce_buffer_size,
                    int64 max_inter_ipu_copies_buffer_size, HloModule* module)
      : main_graph(dev),
        annotations(module),
        information(max_all_reduce_buffer_size,
                    max_inter_ipu_copies_buffer_size),
        default_conv_options(conv_options),
        default_pooling_options(pooling_options),
        disable_graph_convolution_caching(disable_graph_convolution_caching),
        replication_factor(replication_factor),
        merge_infeed_io_copies(merge_infeed_io_copies) {}
};

}  // namespace poplarplugin
}  // namespace xla

#endif