# Copyright 2017 The TensorFlow Authors. All Rights Reserved.
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
"""Operations and utilies related to the Graphcore IPU
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

# pylint: disable=wildcard-import,unused-import
from tensorflow.contrib.ipu.python import autoshard
from tensorflow.contrib.ipu.python import autoshard_cnn
from tensorflow.contrib.ipu.python import gradient_accumulation_optimizer
from tensorflow.contrib.ipu.python import internal
from tensorflow.contrib.ipu.python import ipu_compiler
from tensorflow.contrib.ipu.python import ipu_infeed_queue
from tensorflow.contrib.ipu.python import ipu_optimizer
from tensorflow.contrib.ipu.python import ipu_outfeed_queue
from tensorflow.contrib.ipu.python import loops
from tensorflow.contrib.ipu.python import ops
from tensorflow.contrib.ipu.python import popnn_embedding
from tensorflow.contrib.ipu.python import popnn_normalization
from tensorflow.contrib.ipu.python import popnn_rnn
from tensorflow.contrib.ipu.python import poprand
from tensorflow.contrib.ipu.python import popops_cross_replica_sum
from tensorflow.contrib.ipu.python import sharded_optimizer
from tensorflow.contrib.ipu.python import utils
# pylint: enable=wildcard-import,unused-import