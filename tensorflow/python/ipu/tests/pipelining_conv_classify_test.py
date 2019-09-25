# Copyright 2019 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# =============================================================================
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import os
import numpy as np

from tensorflow.keras import layers
from tensorflow.compiler.plugin.poplar.ops import gen_ipu_ops
from tensorflow.compiler.plugin.poplar.tests import test_utils as tu
from tensorflow.python.framework import ops
from tensorflow.python.framework import test_util
from tensorflow.python.ops import array_ops
from tensorflow.python.ops import init_ops
from tensorflow.python.ops import math_ops
from tensorflow.python.ops import nn
from tensorflow.python.ops import variable_scope
from tensorflow.python.ops import variables
from tensorflow.python.platform import googletest
from tensorflow.python.training import gradient_descent
from tensorflow.python.ipu import ipu_compiler
from tensorflow.python.ipu import ipu_outfeed_queue
from tensorflow.python.ipu import pipelining_ops


def next_feed_id():
  result = 'feed' + str(next_feed_id.feed_count)
  next_feed_id.feed_count += 1
  return result


next_feed_id.feed_count = 0

# Various graph constructor helpers


def _get_variable(name, shape, init):
  return variable_scope.get_variable(
      name, shape, initializer=init, dtype=np.float16)


def block(name, first_stride, out_filters, count, x):

  for i in range(count):
    sc = x
    shape_in = x.shape
    stride = first_stride if (i == 0) else 1

    with variable_scope.variable_scope(name + "/" + str(i) + "/1"):
      x = conv(x, 3, stride, out_filters)
      x = nn.relu(x)

    with variable_scope.variable_scope(name + "/" + str(i) + "/2"):
      x = conv(x, 3, 1, out_filters)

      # shortcut
      if (stride != 1):
        sc = array_ops.strided_slice(
            sc, [0, 0, 0, 0], sc.shape, strides=[1, stride, stride, 1])
      pad = int(x.shape[3] - shape_in[3])
      if (pad != 0):
        sc = array_ops.pad(sc, paddings=[[0, 0], [0, 0], [0, 0], [0, pad]])

      x = nn.relu(x + sc)

  return x


def fc(x, num_units_out):
  return layers.Dense(
      num_units_out,
      kernel_initializer=init_ops.constant_initializer(0.1),
      bias_initializer=init_ops.constant_initializer(0.0))(x)


def max_pool(x, ksize=3, stride=2):
  return layers.MaxPooling2D(ksize, stride, padding='SAME')(x)


def conv(x, ksize, stride, filters_out):
  return layers.Conv2D(
      filters_out,
      ksize,
      stride,
      'SAME',
      kernel_initializer=init_ops.constant_initializer(0.1),
      bias_initializer=init_ops.constant_initializer(0.0))(x)


class PipeliningConvClassifyTest(test_util.TensorFlowTestCase):
  @test_util.deprecated_graph_mode_only
  def testTwoConvs(self):
    # Check that we get all classifications for a simple conv

    def stage1(x, label):
      with variable_scope.variable_scope("stage1", use_resource=True):
        x = conv(x, 3, 1, 16)
        x = nn.relu(x)
        return x, label

    def stage2(x, label):
      with variable_scope.variable_scope("stage2", use_resource=True):
        x = conv(x, 3, 1, 100)
        x = nn.relu(x)
        return x, label

    def stage3(x, label):
      with variable_scope.variable_scope("stage3", use_resource=True):
        x = math_ops.reduce_mean(x, axis=[1, 2])
        loss = math_ops.reduce_mean(
            nn.sparse_softmax_cross_entropy_with_logits(
                logits=x, labels=label))
        return loss

    def optimizer_stage(loss):
      opt = gradient_descent.GradientDescentOptimizer(0.01).minimize(loss)
      return loss, opt

    outfeed_queue = ipu_outfeed_queue.IPUOutfeedQueue(next_feed_id())

    # Run the pipeline twice.
    def model_pipeline(x, lr):
      return pipelining_ops.pipeline([stage1, stage2, stage3],
                                     12,
                                     inputs=[x, lr],
                                     outfeed_queue=outfeed_queue,
                                     optimizer_stage=optimizer_stage)

    with ops.device('cpu'):
      x = array_ops.placeholder(np.float32, shape=[1, 4, 4, 2])
      l = array_ops.placeholder(np.int32, shape=[1])
      evts = gen_ipu_ops.ipu_event_trace()

    with ops.device("/device:IPU:0"):
      compiled_model_pipeline = ipu_compiler.compile(
          model_pipeline, inputs=[x, l])

    tu.move_variable_initialization_to_cpu()
    outfeed_op = outfeed_queue.dequeue()
    tu.configure_ipu_system(pipelining=True, text_report=False)

    with tu.ipu_session() as sess:

      report = tu.ReportJSON(self, None)
      sess.run(variables.global_variables_initializer())
      sess.run(evts)
      sess.run(compiled_model_pipeline, {x: np.ones(x.shape), l: [1]})
      log = sess.run(evts)
      report.parse_events(log)

      # 1 conv in each of 2 stages = 2
      self.assertAllEqual(report.get_ml_type_counts(), [0, 2, 1, 2])

  @test_util.deprecated_graph_mode_only
  def testResnetLike(self):
    # Check that we get all classifications for a small resnet correct

    def stage1(img, label):
      with variable_scope.variable_scope("stage1", use_resource=True):
        x = conv(img, 7, 2, 16)
        x = nn.relu(x)
        x = max_pool(x, ksize=3, stride=2)
        return x, label

    def stage2(x, label):
      with variable_scope.variable_scope("stage2", use_resource=True):
        x = block("b", 2, 64, 1, x)
        return x, label

    def stage3(x, label):
      with variable_scope.variable_scope("stage3", use_resource=True):
        x = math_ops.reduce_mean(x, axis=[1, 2])
        x = fc(x, 100)
        loss = math_ops.reduce_mean(
            nn.sparse_softmax_cross_entropy_with_logits(
                logits=x, labels=label))
        return loss

    def optimizer_stage(loss):
      opt = gradient_descent.GradientDescentOptimizer(0.01).minimize(loss)
      return loss, opt

    outfeed_queue = ipu_outfeed_queue.IPUOutfeedQueue(next_feed_id())

    # Run the pipeline twice.
    def model_pipeline(x, lr):
      return pipelining_ops.pipeline([stage1, stage2, stage3],
                                     12,
                                     inputs=[x, lr],
                                     outfeed_queue=outfeed_queue,
                                     optimizer_stage=optimizer_stage)

    with ops.device('cpu'):
      x = array_ops.placeholder(np.float32, shape=[1, 4, 4, 2])
      l = array_ops.placeholder(np.int32, shape=[1])
      evts = gen_ipu_ops.ipu_event_trace()

    with ops.device("/device:IPU:0"):
      compiled_model_pipeline = ipu_compiler.compile(
          model_pipeline, inputs=[x, l])

    tu.move_variable_initialization_to_cpu()
    outfeed_op = outfeed_queue.dequeue()
    tu.configure_ipu_system(pipelining=True, text_report=False)

    with tu.ipu_session() as sess:

      report = tu.ReportJSON(self, None)
      sess.run(variables.global_variables_initializer())
      sess.run(evts)
      sess.run(compiled_model_pipeline, {x: np.ones(x.shape), l: [1]})
      log = sess.run(evts)
      report.parse_events(log)

      # 1 conv in stage1, 2 conv in stage2, 1 matmul in stage3 = 4
      self.assertAllEqual(report.get_ml_type_counts(), [0, 4, 3, 4])


if __name__ == "__main__":
  googletest.main()
