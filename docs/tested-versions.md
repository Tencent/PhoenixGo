Below is a list of other versions of cuda/cudnn/tensorrt/bazel tar/deb 
tested by independent contributors, that work with PhoenixGo AI

compatibility for tensorrt : 
- 3.0.4 requires cudnn 7.0.x and +
- 4.x requires cudnn 7.1.x and +
- 5.x requires cudnn 7.3.x and +

### tests by [wonderingabout](https://github.com/wonderingabout/) :

#### works
- bazel : 0.11.1 , 0.17.2 , 0.18.1
- ubuntu : 16.04 , 18.04 
- cuda : 9.0 deb , 10.0 deb (10.0 deb ubuntu 18.04 is with cudnn 
7.4.x deb and no tensorrt), 9.0 for windows 10 and windows server 
2016
- cudnn : 7.0.5 deb, 7.1.4 deb , 7.4.2 deb, 7.1.4 on windows 10 
with cuda 9.0
- tensorrt : 3.0.4 deb , 3.0.4 tar
- pycuda : for tensorrt tar, pycuda with pyhton 2.7 
(needs [a modification](http://0561blue.tistory.com/m/13?category=627413), 
pycuda specific issue, see [#75](https://github.com/Tencent/PhoenixGo/issues/75)

#### does NOT work : 
- cuda on windows 10 : cuda 10.0
- cudnn for windows 10 : cudnn 7.4.x for cuda 10.0 , cudnn 7.4.x for 
cuda 9.0
- tensorrt : 4.x deb , 4.x tar, 5.x deb
- bazel : 0.19.x and superior 
(needs [a modification](https://github.com/tensorflow/tensorflow/issues/23401))
