![PhoenixGo](images/logo.jpg?raw=true)

**PhoenixGo** is a Go AI program which implements the AlphaGo Zero paper
"[Mastering the game of Go without human knowledge](https://deepmind.com/documents/119/agz_unformatted_nature.pdf)".
It is also known as "BensonDarr" and "金毛测试" in [FoxGo](http://weiqi.qq.com/), 
"cronus" in [CGOS](http://www.yss-aya.com/cgos/), and the champion of 
[World AI Go Tournament 2018](http://weiqi.qq.com/special/109) held in Fuzhou China.

If you use PhoenixGo in your project, please consider mentioning in your README.

If you use PhoenixGo in your research, please consider citing the library as follows:

```
@misc{PhoenixGo2018,
  author = {Qinsong Zeng and Jianchang Zhang and Zhanpeng Zeng and Yongsheng Li and Ming Chen and Sifan Liu}
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
* Bazel (**0.19.2 is known-good**)
* (Optional) CUDA and cuDNN for GPU support 
* (Optional) TensorRT (for accelerating computation on GPU, 3.0.4 is known-good)

The following environments have also been tested by independent contributors : 
[here](/docs/tested-versions.md). Other versions may work, but they have not been 
tested (especially for bazel).

#### Download and Install Bazel 

Before starting, you need to download and install bazel,
see [here](https://docs.bazel.build/versions/master/install.html).

For PhoenixGo, bazel (**0.19.2 is known-good**), read 
[Requirements](#requirements) for details

If you have issues on how to install or start bazel, you may want 
to try this all-in-one command line for easier building instead, see
[FAQ question](/docs/FAQ.md#b0-it-is-too-hard-to-install-bazel-or-start-bazel)

#### Building PhoenixGo with Bazel

Clone the repository and configure the building:

```
$ git clone https://github.com/Tencent/PhoenixGo.git
$ cd PhoenixGo
$ ./configure
```

`./configure` will start the bazel configure : ask where CUDA 
and TensorRT have been installed, specify them if need.

Then build with bazel:

```
$ bazel build //mcts:mcts_main
```

Dependices such as Tensorflow will be downloaded automatically. 
The building process may take a long time.

Recommendation : the bazel building uses a lot of RAM, 
if your building environment is lack of RAM, you may need to restart 
your computer and exit other running programs to free as much RAM 
as possible.

#### Running PhoenixGo

Download and extract the trained network:

```
$ wget https://github.com/Tencent/PhoenixGo/releases/download/trained-network-20b-v1/trained-network-20b-v1.tar.gz
$ tar xvzf trained-network-20b-v1.tar.gz
```

The PhoenixGo engine supports GTP 
[(Go Text Protocol)](https://senseis.xmp.net/?GoTextProtocol),
which means it can be used with a GUI with GTP capability, such as 
[Sabaki](http://sabaki.yichuanshen.de).
It can also run on command-line GTP server tools like 
[gtp2ogs](https://github.com/online-go/gtp2ogs).

But PhoenixGo does not support all GTP commands, see 
[FAQ question](/docs/FAQ.md/#a11-gtp-command-error--invalid-command).

There are 2 ways to run PhoenixGo engine

##### 1) start.sh : easy use

Run the engine : `scripts/start.sh`

`start.sh` will automatically detect the number of GPUs, run `mcts_main` 
with proper config file,
and write log files in directory `log`.

You could also use a customized config file (.conf) by running 
`scripts/start.sh {config_path}`.
If you want to do that, see also [#configure-guide](#configure-guide).

##### 2) mcts_main : fully control

If you want to fully control all the options of `mcts_main` (such 
as changing log destination, or if start.sh is not compatible for your 
specific use), you can run directly `bazel-bin/mcts/mcts_main` instead.

For a typical usage, these command line options should be added:
- `--gtp` to enable GTP mode
- `--config_path=replace/with/path/to/your/config/file` to specify the 
path to your config file
- it is also needed to edit your config file (.conf) and manually add 
the full path to ckpt, see 
[FAQ question](/docs/FAQ.md/#a5-ckptzerockpt-20b-v1fp32plan-error-no-such-file-or-directory).
You can also change options in config file, see 
[#configure-guide](#configure-guide).
- for other command line options , see also 
[#command-line-options](#command-line-options) 
for details, or run `./mcts_main --help` . A copy of the `--help` is 
provided for your convenience [here](/docs/mcts-main-help.md)

For example:

```
$ bazel-bin/mcts/mcts_main --gtp --config_path=etc/mcts_1gpu.conf --logtostderr --v=0
```

#### (Optional) : Distribute mode

PhoenixGo support running with distributed workers, if there are GPUs 
on different machine.

Build the distribute worker:

```
$ bazel build //dist:dist_zero_model_server
```

Run `dist_zero_model_server` on distributed worker, **one for each GPU**.

```
$ CUDA_VISIBLE_DEVICES={gpu} bazel-bin/dist/dist_zero_model_server --server_address="0.0.0.0:{port}" --logtostderr
```

Fill `ip:port` of workers in the config file (`etc/mcts_dist.conf` is an 
example config for 32 workers), and run the distributed master:

```
$ scripts/start.sh etc/mcts_dist.conf
```

### On macOS

**Note: Tensorflow stop providing GPU support on macOS since 1.2.0, so you are only able to run on CPU.**

#### Use Pre-built Binary

Download and extract 
[CPU-only version (macOS)](https://github.com/Tencent/PhoenixGo/releases/download/mac-x64-cpuonly-v1/PhoenixGo-mac-x64-cpuonly-v1.tgz)

Follow the document included in the archive : using_phoenixgo_on_mac.pdf

#### Building from Source

Same as Linux.

### On Windows

Recommendation: See [FAQ question](/docs/FAQ.md/#a4-syntax-error-windows), 
to avoid syntax errors in config file and command line options on Windows.

#### Use Pre-built Binary

##### GPU version :

The GPU version is much faster, but works only with compatible nvidia GPU.
It supports this environment : 
- CUDA 9.0 only
- cudnn 7.1.x (x is any number) or lower for CUDA 9.0
- no AVX, AVX2, AVX512 instructions supported in this release (so it is 
currently much slower than the linux version)
- there is no TensorRT support on Windows

Download and extract 
[GPU version (Windows)](https://github.com/Tencent/PhoenixGo/releases/download/win-x64-gpu-v1/PhoenixGo-win-x64-gpu-v1.zip)

Then follow the document included in the archive : how to install 
phoenixgo.pdf

note : to support special features like CUDA 10.0 or AVX512 for example, 
you can build your own build for windows, see 
[#79](https://github.com/Tencent/PhoenixGo/issues/79)

##### CPU-only version : 

If your GPU is not compatible, or if you don't want to use a GPU, you can download this 
[CPU-only version (Windows)](https://github.com/Tencent/PhoenixGo/releases/download/win-x64-cpuonly-v1/PhoenixGo-win-x64-cpuonly-v1.zip), 

Follow the document included in the archive : how to install 
phoenixgo.pdf

## Configure Guide

Here are some important options in the config file:

* `num_eval_threads`: should equal to the number of GPUs
* `num_search_threads`: should a bit larger than `num_eval_threads * eval_batch_size`
* `timeout_ms_per_step`: how many time will used for each move
* `max_simulations_per_step`: how many simulations(also called playouts) will do for each move
* `gpu_list`: use which GPUs, separated by comma
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
* `--init_moves`: initial moves on the go board, for example usage, see 
[FAQ question](/docs/FAQ.md/#a8-how-make-phoenixgo-start-at-other-position-at-move-1-and-after)
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
A copy of the `--help` is provided for your convenience 
[here](/docs/mcts-main-help.md)

## Analysis

For analysis purpose, an easy way to display the PV (variations for 
main move path) is `--logtostderr --v=1` which will display the main 
move path winrate and continuation of moves analyzed, see 
[FAQ question](/docs/FAQ.md/#a2-where-is-the-pv-analysis-) for details

It is also possible to analyse .sgf files using analysis tools such as :
- [GoReviewPartner](https://github.com/pnprog/goreviewpartner) : 
an automated tool to analyse and/or review one or many .sgf files 
(saved as .rsgf file). It supports PhoenixGo and other bots. See 
[FAQ question](/docs/FAQ.md/#a25-how-to-analyzereview-one-or-many-sgf-files-with-goreviewpartner) 
for details

## FAQ

You will find a lot of useful and important information, also most common 
problems and errors and how to fix them

Please take time to read the [FAQ](/docs/FAQ.md)
