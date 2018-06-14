#!/bin/bash

cd "`dirname $0`/.."
echo "current directory: '$PWD'" >&2

config="$1"

if [[ -z $config ]]; then
    if [[ `uname` == Darwin ]]; then
        echo "running on macOS" >&2
        config="etc/mcts_cpu.conf"
    else
        ldd bazel-bin/mcts/mcts_main | grep libcuda > /dev/null
        has_cuda=$?
        if [[ $has_cuda == "0" ]]; then
            echo "mcts_main was built with CUDA support" >&2
        else
            echo "mcts_main wasn't built with CUDA support" >&2
        fi

        ldd bazel-bin/mcts/mcts_main | grep libnvinfer > /dev/null
        has_tensorrt=$?
        if [[ $has_tensorrt == "0" ]]; then
            echo "mcts_main was built with TensorRT support" >&2
        else
            echo "mcts_main wasn't built with TensorRT support" >&2
        fi

        num_gpu=0
        if [[ $has_cuda == "0" ]]; then
            num_gpu=`nvidia-smi -L | wc -l`
            echo "found $num_gpu GPU(s)" >&2
        fi

        if [[ $has_cuda == "0" && $num_gpu -gt 0 ]]; then
            if [[ $has_tensorrt == "0" ]]; then
                config="etc/mcts_${num_gpu}gpu.conf"
            else
                config="etc/mcts_${num_gpu}gpu_notensorrt.conf"
            fi
        else
            config="etc/mcts_cpu.conf"
        fi
    fi
fi

echo "use config file '$config'" >&2

export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PWD/bazel-bin/external/org_tensorflow/tensorflow"

echo "log to '$PWD/log'" >&2
mkdir -p log

echo "start mcts_main" >&2
exec bazel-bin/mcts/mcts_main --config_path="$config" --gtp --log_dir=log --v=1
