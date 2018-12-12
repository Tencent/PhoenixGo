![PhoenixGo](images/logo.jpg?raw=true)

**PhoenixGo** is a Go AI program which implements the AlphaGo Zero paper
"[Mastering the game of Go without human knowledge](https://deepmind.com/documents/119/agz_unformatted_nature.pdf)".
It is also known as "BensonDarr" in FoxGo, "cronus" in CGOS,
and the champion of "World AI Go Tournament 2018" held in Fuzhou China.

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
* Bazel (0.11.1 is known-good)
* (Optional) CUDA and cuDNN (for GPU support)
* (Optional) TensorRT (for accelerating computation on GPU, 3.0.4 is known-good)

#### Building

Clone the repository and configure the building:

```
$ git clone https://github.com/Tencent/PhoenixGo.git
$ cd PhoenixGo
$ ./configure
```

`./configure` will ask where CUDA and TensorRT have been installed, specify them if need.

Then build with bazel:

```
$ bazel build //mcts:mcts_main
```

Dependices such as Tensorflow will be downloaded automatically. The building prosess may take a long time.

#### Running

Download and extract the trained network, then run:

```
$ wget https://github.com/Tencent/PhoenixGo/releases/download/trained-network-20b-v1/trained-network-20b-v1.tar.gz
$ tar xvzf trained-network-20b-v1.tar.gz
$ scripts/start.sh
```

`start.sh` will detect the number of GPUs, run `mcts_main` with proper config file, and write log files in directory `log`.
You could also use a customized config by running `scripts/start.sh {config_path}`.
See also [#configure-guide](#configure-guide).

Furthermore, if you want to fully control all the options of `mcts_main` (such as, changing log destination),
you could also run `bazel-bin/mcts/mcts_main` directly. See also [#command-line-options](#command-line-options).

The engine supports the GTP protocol, means it could be used with a GUI with GTP capability,
such as [Sabaki](http://sabaki.yichuanshen.de).

#### Distribute mode

PhoenixGo support running with distributed workers, if there are GPUs on different machine.

Build the distribute worker:

```
$ bazel build //dist:dist_zero_model_server
```

Run `dist_zero_model_server` on distributed worker, **one for each GPU**.

```
$ CUDA_VISIBLE_DEVICES={gpu} bazel-bin/dist/dist_zero_model_server --server_address="0.0.0.0:{port}" --logtostderr
```

Fill `ip:port` of workers in the config file (`etc/mcts_dist.conf` is an example config for 32 workers),
and run the distributed master:

```
$ scripts/start.sh etc/mcts_dist.conf
```

### On macOS

**Note: Tensorflow stop providing GPU support on macOS since 1.2.0, so you are only able to run on CPU.**

#### Use Pre-built Binary

Download and extract https://github.com/Tencent/PhoenixGo/releases/download/mac-x64-cpuonly-v1/PhoenixGo-mac-x64-cpuonly-v1.tgz

Follow the document: using_phoenixgo_on_mac.pdf

#### Building from Source

Same as Linux.

### On Windows

#### Use Pre-built Binary

Download and extract https://github.com/Tencent/PhoenixGo/releases/download/win-x64-gpu-v1/PhoenixGo-win-x64-gpu-v1.zip

Or CPU-only version https://github.com/Tencent/PhoenixGo/releases/download/win-x64-cpuonly-v1/PhoenixGo-win-x64-cpuonly-v1.zip

Follow the document: how to install phoenixgo.pdf

## Configure Guide

Here are some important options in the config file:

* `num_eval_threads`: should equal to the number of GPUs
* `num_search_threads`: should a bit larger than `num_eval_threads * eval_batch_size`
* `timeout_ms_per_step`: how many time will used for each move
* `max_simulations_per_step`: how many simulations will do for each move
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

## FAQ

**1. Where is the win rate?**

Print in the log, something like:

<pre>
I0514 12:51:32.724236 14467 mcts_engine.cc:157] 1th move(b): dp, <b>winrate=44.110905%</b>, N=654, Q=-0.117782, p=0.079232, v=-0.116534, cost 39042.679688ms, sims=7132, height=11, avg_height=5.782244, global_step=639200
</pre>

**2. There are too much log.**

Passing `--v=0` to `mcts_main` will turn off many debug log.
Moreover, `--minloglevel=1` and `--minloglevel=2` could disable INFO log and WARNING log.

Or, if you just don't want to log to stderr, replace `--logtostderr` to `--log_dir={log_dir}`,
then you could read your log from `{log_dir}/mcts_main.INFO`.

**3. How to run with Sabaki?**

Setting GTP engine in Sabaki's menu: `Engines -> Manage Engines`, fill `Path` with path of `start.sh`.
Click `Engines -> Attach` to use the engine in your game.
See also [#22](https://github.com/Tencent/PhoenixGo/issues/22).

**4. How make PhoenixGo think with longer/shorter time?**

Modify `timeout_ms_per_step` in your config file.

**5. How make PhoenixGo think with constant time per move?**

Modify your config file. `early_stop`, `unstable_overtime`, `behind_overtime` and
`time_control` are options that affect the search time, remove them if exist then
each move will cost constant time/simulations.

**6. GTP command `time_settings` doesn't work.**

Add these lines in your config:

```
time_control {
    enable: 1
    c_denom: 20
    c_maxply: 40
    reserved_time: 1.0
}
```

`c_denom` and `c_maxply` are parameters for deciding how to use the "main time".
`reserved_time` is how many seconds should reserved (for network latency) in "byo-yomi time".

**7. Syntax error (Windows)**

For windows,
- in config file, you need to write path with `/` and not `\` in the config file .conf, 

for example : 

```
model_config {
      train_dir: "c:/users/amd2018/Downloads/PhoenixGo/ckpt"
```

- However, in cmd.exe you need to write paths with `\` and not `/`
Also in cmd.exe command format on windows needs a space,

for example : 

`mcts_main.exe --gtp --config_path C:\Users\amd2018\Downloads\PhoenixGo\etc\mcts_1gpu_notensorrt.conf`

See point 8. below :

**8. '"ckpt/zero.ckpt-20b-v1.FP32.PLAN"' error: No such file or directory**

This fix works for all systems : Linux, Mac, Windows, only the name of the ckpt file changes
Modify your config file and write the full path of your ckpt folder :

for example : 

```
model_config {
    train_dir: "/home/amd2018/PhoenixGo/ckpt"
```
    
for example, for windows :

```
model_config {
    train_dir: "c:/users/amd2018/Downloads/PhoenixGo/ckpt"
```

**9. Most common path errors during bazel configure**

### First of all, please make sure you did the post install path exports

in the example below, we are using ubuntu 16.04 LTS is used with deb install of cuda 9.0 ,
cudnn 7.1.4, tensorrt 3.0.4, as well as bazel 0.17.2 installed with a .run, but other linux distributions with nvidia tar install are possible too

The settings below have been tested to be working and to fix most common path issues, and
are shown as an interactive help :


- a) after cuda 9.0 deb install, it is needed to do :

`export PATH=/usr/local/cuda-9.0/bin${PATH:+:${PATH}} && sudo reboot`

for ubuntu users, it is also recommended to add path in /etc/environment

`sudo nano /etc/environment`

add this part in the paths (with the `:`) `:/usr/local/cuda/bin` , then save and exit and reboot to finalize

- b) and after cudnn 7.1.4 deb install : 

check if cudnn works by compiling and runing a cudnn code sample: 

```
cp -r /usr/src/cudnn_samples_v7/ ~ && cd ~/cudnn_samples_v7/mnistCUDNN && make clean && make && ./mnistCUDNN
```
should display this : `Test passed!`

Reboot to finalize

- c) post install for cudnn and cuda

you can export path to bashrc : 
`export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/usr/local/cuda/extras/CUPTI/lib64" && source ~/.bashrc && sudo reboot`

after reboot check if cuda installation is a success : 

`cat /proc/driver/nvidia/version`

should display something like this :

```
NVRM version: NVIDIA UNIX x86_64 Kernel Module  384.130  Wed Mar 21 03:37:26 PDT 2018
GCC version:  gcc version 5.4.0 20160609 (Ubuntu 5.4.0-6ubuntu1~16.04.10)
```

check if nvcc path works :

`nvcc --version`

should display something like this : 

```
nvcc: NVIDIA (R) Cuda compiler driver
Copyright (c) 2005-2017 NVIDIA Corporation
Built on Fri_Sep__1_21:08:03_CDT_2017
Cuda compilation tools, release 9.0, V9.0.176
```


### Secondly, locate cuda and cudnn paths, and update database if not here

Run this command : `locate libcudart.so && locate libcudnn.so.7`

you need to see something like this : 

```
/usr/local/cuda-9.0/doc/man/man7/libcudart.so.7
/usr/local/cuda-9.0/targets/x86_64-linux/lib/libcudart.so
/usr/local/cuda-9.0/targets/x86_64-linux/lib/libcudart.so.9.0
/usr/local/cuda-9.0/targets/x86_64-linux/lib/libcudart.so.9.0.176
/usr/lib/x86_64-linux-gnu/libcudnn.so.7
/usr/lib/x86_64-linux-gnu/libcudnn.so.7.1.4
```
If you don't see this, run this command : 

`sudo updatedb && locate libcudart.so && locate libcudnn.so.7`

It should now display all the cuda and cudnn paths same as above.
Reboot your computer to finalize.


### Thirdly, during bazel compile, this is the paths you need to put

Press ENTER for every prompt to choose default settings, except for these : 

- CUDA : choose `y` , version `9.0`, and custom cuda path `/usr/local/cuda-9.0/`
- cudnn : choose version `7.1` and custom cudnn path `/usr/lib/x86_64-linux-gnu/`
- if you use tensorrt do `y` and press enter to keep default path

same as below :

```
Do you wish to build TensorFlow with CUDA support? [y/N]: y 
CUDA support will be enabled for TensorFlow.

Please specify the CUDA SDK version you want to use, e.g. 7.0. [Leave empty to default to CUDA 9.0]: 9.0

Please specify the location where CUDA 9.0 toolkit is installed. Refer to README.md for more details. [Default is /usr/local/cuda]: /usr/local/cuda-9.0/

Please specify the cuDNN version you want to use. [Leave empty to default to cuDNN 7.0]: 7.1

Please specify the location where cuDNN 7 library is installed. Refer to README.md for more details. [Default is /usr/local/cuda-9.0/]: /usr/lib/x86_64-linux-gnu/

Do you wish to build TensorFlow with TensorRT support? [y/N]: y
TensorRT support will be enabled for TensorFlow.

Please specify the location where TensorRT is installed. [Default is /usr/lib/x86_64-linux-gnu]:
```
these settings are just an example, but they have been tested to successfully work on ubuntu 16.04 LTS with deb install of cuda 9.0, deb install of cudnn 7.1.4, deb install of tensorrt 3.0.4, as well as .run install of bazel 0.17.2

they are provided as a general help for linux compile and run, they are not an obligatory method to use

credits : 
- [nvidia pdf instal guide for cuda 9.0](http://developer.download.nvidia.com/compute/cuda/9.0/Prod/docs/sidebar/CUDA_Installation_Guide_Linux.pdf)
- [medium.com/@zhanwenchen/](https://medium.com/@zhanwenchen/install-cuda-and-cudnn-for-tensorflow-gpu-on-ubuntu-79306e4ac04e)

