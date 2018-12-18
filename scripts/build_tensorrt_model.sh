#!/bin/bash

set -x
set -e

base_dir=`dirname $0`

train_dir="$1"
checkpoint="$2"

max_batch_size=4
if [ $# -ge 3 ]; then
    max_batch_size="$3"
fi

python -m tensorflow.python.tools.freeze_graph --input_meta_graph="$train_dir/meta_graph" --input_checkpoint="$train_dir/$checkpoint" --input_binary --output_graph="$train_dir/$checkpoint.frozen.pb" --output_node_names=policy,value,global_step

python $base_dir/graph_transform.py "$train_dir/$checkpoint.frozen.pb" "$train_dir/$checkpoint.transformed.pb"

python -m tensorflow.python.tools.optimize_for_inference --input="$train_dir/$checkpoint.transformed.pb" --output="$train_dir/$checkpoint.optimized.pb" --frozen_graph=True --input_name=inputs --output_name=policy,value,global_step

convert-to-uff tensorflow -o "$train_dir/$checkpoint.uff" --input-file "$train_dir/$checkpoint.optimized.pb" -O "policy" -O "value"

$base_dir/../bazel-bin/model/build_tensorrt_model --logtostderr --model_path="$train_dir/$checkpoint" --data_type=FP32 --max_batch_size=$max_batch_size

python $base_dir/get_global_step.py "$train_dir/$checkpoint.frozen.pb" > "$train_dir/$checkpoint.FP32.PLAN.step"
