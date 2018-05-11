![PhoenixGo](images/logo.jpg?raw=true)

**PhoenixGo** is an Go AI program which implement the AlphaGo Zero paper
"[Mastering the game of Go without human knowledge](https://deepmind.com/documents/119/agz_unformatted_nature.pdf)".
It is also known as "BensonDarr" in FoxGo, "cronus" in CGOS,
and the champion of "World AI Go Tournament 2018" held in Fuzhou China.

If you use PhoenixGo in your project, please consider mentioning in your README.

If you use PhoenixGo in your research, please consider citing the library as follows:

```
@misc{PhoenixGo2018,
  author = {Qinsong Zeng and Jianchang Zhang and Zhanpeng Zeng and Yongsheng Li and Ming Chen}
  title = {PhoenixGo},
  year = {2018},
  journal = {GitHub repository},
  howpublished = {\url{https://github.com/Tencent/PhoenixGo}}
}
```

## Building and Running

### On Linux

#### Requirements

* GCC with C++11 support
* Bazel (0.11.1 is known-good)
* (Optional) CUDA and cuDNN (for GPU support)
* (Optional) TensorRT (for accelerating computation on GPU, 3.0.4 is known-good)

#### Building

Clone the repository and configure the building:

```
git clone https://github.com/Tencent/PhoenixGo.git
cd PhoenixGo
./configure
```

`./configure` will ask where CUDA and TensorRT have been installed, specify them if need.

Then build with bazel:

```
bazel build //mcts:mcts_main
```

Dependices such as Tensorflow will be downloaded automatically. The building prosess may take a long time.

#### Running

Download and extract the trained network:

```
wget https://github.com/Tencent/PhoenixGo/releases/download/trained-network-20b-v1/trained-network-20b-v1.tar.gz
tar xvzf trained-network-20b-v1.tar.gz
```

Run in gtp mode with a config file (depend on the number of GPUs and using TensorRT or not):

```
bazel-bin/mcts/mcts_main --config_path=etc/{config} --gtp --logtostderr --v=1
```

The engine supports the GTP protocol, means it could be used with a GUI with GTP capability,
such as [Sabaki](http://sabaki.yichuanshen.de).

`--logtostderr` let `mcts_main` log messages to stderr, if you want to log to files,
change `--logtostderr` to `--log_dir={log_dir}`

You could modify your config file following [#configure-guide](#configure-guide).

#### Distribute mode

PhoenixGo support running with distributed workers, if there are GPUs on different machine.

Build the distribute worker:

```
bazel build //dist:dist_zero_model_server
```

Run `dist_zero_model_server` on distributed worker, **one for each GPU**.

```
CUDA_VISIBLE_DEVICES={gpu} bazel-bin/dist/dist_zero_model_server --server_address"0.0.0.0:{port}" --logtostderr
```

Fill `ip:port` of workers in the config file (`etc/mcts_dist.conf` is an example config for 32 workers),
and run the distributed master:

```
bazel-bin/mcts/mcts_main --config_path=etc/{config} --gtp --logtostderr --v=1
```

### On Windows

Work in progress.

## Configure Guide

Here are some important options in the config file:

* `num_eval_threads`: should equal to the number of GPUs
* `num_search_threads`: should a bit larger than `num_eval_threads * eval_batch_size`
* `timeout_ms_per_step`: how many time will used for each move
* `max_simulations_per_step`: how many simulations will do for each move
* `gpu_list`: use which GPUs, sperated by comma
* `model_config -> train_dir`: directory where trained network stored
* `model_config -> checkpoint_path`: use which checkpoint, get from `train_dir/checkpoint` if not set
* `model_config -> enable_tensorrt`: use TensorRT or not
* `model_config -> tensorrt_model_path`: use which TensorRT model, if `enable_tensorrt`
* `max_search_tree_size`: the maximum number of tree nodes, change it depends on memory size
* `max_children_per_node`: the maximum children of each node, change it depends on memory size
* `enable_background_search`: pondering in opponent's time
* `early_stop`: genmove may return before `timeout_ms_per_step`, if the result would not change any more
* `unstable_overtime`: think `timeout_ms_per_step * time_factor` more if the result still unstable
* `behind_overtime`: think `timeout_ms_per_step * time_factor` more if winrate less than `act_threshold`

Options for distribute mode:

* `enable_dist`: enable distribute mode
* `dist_svr_addrs`: `ip:port` of distributed workers, multiple lines, one `ip:port` in each line
* `dist_config -> timeout_ms`: RPC timeout

Options for async distribute mode:

> Async mode is used when there are huge number of distributed workers (more than 200),
> which need too many eval threads and search threads in sync mode.
> `etc/mcts_async_dist.conf` is an example config for 256 workers.

* `enable_async`: enable async mode
* `enable_dist`: enable distribute mode
* `dist_svr_addrs`: multiple lines, comma sperated lists of `ip:port` for each line
* `num_eval_threads`: should equal to number of `dist_svr_addrs` lines
* `eval_task_queue_size`: tunning depend on number of distribute workers
* `num_search_threads`: tunning depend on number of distribute workers


Read `mcts/mcts_config.proto` for more config options.

## Command Line Options

`mcts_main` accept options from command line:

* `--config_path`: path of config file
* `--gtp`: run as a GTP engine, if disable, gen next move only
* `--init_moves`: initial moves on the go board
* `--gpu_list`: override `gpu_list` in config file
* `--listen_port`: work with `--gtp`, run gtp engine on port in TCP protocol
* `--allow_ip`: work with `--listen_port`, list of client ip allowed to connect
* `--fork_per_request`: work with `--listen_port`, fork for each request or not

Glog options are also supported:

* `--logtostderr`: log message to stderr
* `--log_dir`: log to files in this directory
* `--minloglevel`: log level, 0 - INFO, 1 - WARNING, 2 - ERROR
* `--v`: verbose log, `--v=1` for turning on some debug log, `--v=0` to turning off

`mcts_main --help` for more command line options.
