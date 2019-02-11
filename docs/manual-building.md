continuation of [this](/README.md/#building---possibility-b--manual-steps-for-other-use)

##### Manual Building

Install [bazel](https://docs.bazel.build/versions/master/install.html)

Clone the PhoenixGo repository and configure the building:

```
git clone https://github.com/Tencent/PhoenixGo.git
cd PhoenixGo
./configure
```

Configure the build : during `./configure` , bazel will ask building options
and where CUDA, cuDNN, and TensorRT have been installed -> Press ENTER for default
settings and choose the building options you want, and modify paths if needed 
(see [FAQ question](/README.md/#b1-i-am-getting-errors-during-bazel-configure-bazel-building-andor-running-phoenixgo-engine) 
and see [minimalist bazel install](/docs/minimalist-bazel-install.md) if you need help)

Then build PhoenixGo with bazel:

```
bazel build //mcts:mcts_main
```

Dependencies such as Tensorflow will be downloaded automatically. The building process may take a long time (1 hour or more).

##### Downloading the trained network (ckpt)

Download and extract the trained network archive, then remove the archive to cleanup :

```
wget https://github.com/Tencent/PhoenixGo/releases/download/trained-network-20b-v1/trained-network-20b-v1.tar.gz
tar xvzf trained-network-20b-v1.tar.gz
```

##### Cleanup : 

After the building is finished, you can remove the trained network archive (.tar.gz)

And if you installed bazel with a .sh installer, you can also remove the bazel installer file to cleanup

##### After building :

After the building is a success, continue reading at [Running](README.md/#running)
