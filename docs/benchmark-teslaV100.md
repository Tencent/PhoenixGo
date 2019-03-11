# Benchmark setup :

## setup
- hardware : google cloud machine with Tesla V100-SXM2-16GB, 4 and 
12vcpu (skylake server or later, support avx512, google cloud platform
currently does not allow more than 12 vcpu per die, so maximum for 1 
GPU is 12 vcpu = 6 physical cores / 12 cpu threads), 
16gb system ram, 40 gb hdd
- software : ubuntu 18.04 LTS, nvidia 410, cuda 10.0, cudnn 7.4.2, 
no tensorrt, bazel 0.11.1
- engine settings: limited time 60 seconds per move, all other time 
management settings disabled in config file, all the rest is default 
settings

## methodology : 
- most moves come from the same game played using 
[gtp2ogs](https://github.com/online-go/gtp2ogs), for few moves moves, 
copy paste stderr output
- tensorRT is not used with the V100 here, because it would need to 
build our own tensor model, which was not done here, see 
[FAQ question](#a13-i-have-a-nvidia-rtx-card-turing-or-tesla-v100titan-v-volta-is-it-compatible-)
for details

## credits :
- credit for doing this tests go to 
[wonderingabout](https://github.com/wonderingabout)
- credit for providing the hardware goes to google cloud 
platform

# BATCH SIZE 4 

- batch size 4
- tensorrt : OFF
- 8 threads
- children : 64
- 400M tree size 
- unlimited sims
- 60s per move

### 4 vcpu (2 physical cores/ 4 cpu threads) :

`
stderr: 4th move(w): pp, winrate=56.125683%, N=20226, Q=0.122514, p=0.728064, v=0.114009, cost 60014.109375ms, sims=22976, height=46, avg_height=12.719517, global_step=639200
`

### 12 vcpu (6 physical cores/ 12 cpu threads) :

`
stderr: 2th move(w): pd, winrate=56.061207%, N=6544, Q=0.121224, p=0.212260, v=0.106201, cost 60021.523438ms, sims=23096, height=30, avg_height=9.461098, global_step=639200
`

# BATCH SIZE 8

- batch size 8
- tensorrt : OFF
- 16 threads
- children : 96
- 2000M tree size 
- unlimited sims
- 60s per move

### 4 vcpu (2 physical cores/ 4 cpu threads) :

`
stderr: 8th move(w): nq, winrate=56.569016%, N=27010, Q=0.131380, p=0.591054, v=0.117058, cost 60036.335938ms, sims=32152, height=59, avg_height=14.057215, global_step=639200
`

### 12 vcpu (6 physical cores/ 12 cpu threads) :

`
stderr: 4th move(w): pp, winrate=56.120705%, N=28938, Q=0.122414, p=0.715722, v=0.114932, cost 60016.148438ms, sims=32728, height=54, avg_height=13.301727, global_step=639200
`

# BATCH SIZE 16 

- batch size 16
- tensorrt : OFF
- 32 threads
- children : 128
- 2000M tree size 
- unlimited sims
- 60s per move

### 4 vcpu (2 physical cores/ 4 cpu threads) :

`
stderr: 2th move(w): dp, winrate=56.057503%, N=15696, Q=0.121150, p=0.207913, v=0.105841, cost 60048.324219ms, sims=53568, height=34, avg_height=9.826570, global_step=639200
`

### 12 vcpu (6 physical cores/ 12 cpu threads) :

`
stderr: 6th move(w): qn, winrate=56.170525%, N=29628, Q=0.123410, p=0.306212, v=0.111480, cost 60020.058594ms, sims=53968, height=71, avg_height=14.943110, global_step=639200
`

# BATCH SIZE 32 

- batch size 32
- tensorrt : OFF
- 64 threads
- children : 128
- 2000M tree size 
- unlimited sims
- 60s per move

### 4 vcpu (2 physical cores/ 4 cpu threads) :

`
stderr: 10th move(w): cp, winrate=65.475777%, N=58821, Q=0.309516, p=0.886150, v=0.196629, cost 60111.078125ms, sims=59444, height=53, avg_height=10.118464, global_step=639200
`

### 12 vcpu (6 physical cores/ 12 cpu threads) :

`
stderr: 8th move(w): qf, winrate=56.714546%, N=55717, Q=0.134291, p=0.613282, v=0.114160, cost 60048.957031ms, sims=62368, height=64, avg_height=13.618808, global_step=639200
`

# BATCH SIZE 64 

- batch size 64
- tensorrt : OFF
- 32 threads
- children : 128
- 2000M tree size 
- unlimited sims
- 60s per move

### 4 vcpu (2 physical cores/ 4 cpu threads) :

`
stderr: 12th move(w): bo, winrate=65.815079%, N=64431, Q=0.316302, p=0.884180, v=0.226788, cost 60263.683594ms, sims=64960, height=54, avg_height=12.711725, global_step=639200
`

### 12 vcpu (6 physical cores/ 12 cpu threads) :

`
stderr: 10th move(w): pc, winrate=65.360603%, N=67943, Q=0.307212, p=0.887373, v=0.175454, cost 60165.914062ms, sims=69031, height=63, avg_height=7.840148, global_step=639200
`

# BATCH SIZE 128 

- batch size 128
- tensorrt : OFF
- 256 threads
- children : 128
- 2000M tree size 
- unlimited sims
- 60s per move

### 4 vcpu (2 physical cores/ 4 cpu threads) :

`
stderr: 16th move(w): rf, winrate=65.895859%, N=66225, Q=0.317917, p=0.937327, v=0.202470, cost 60232.250000ms, sims=67328, height=44, avg_height=10.560142, global_step=639200
`

### 12 vcpu (6 physical cores/ 12 cpu threads) :

`
stderr: 12th move(w): ob, winrate=65.983253%, N=70697, Q=0.319665, p=0.920173, v=0.223816, cost 60312.035156ms, sims=71664, height=49, avg_height=6.786881, global_step=639200
`

# CONCLUSIONS for Tesla V100 : 

- all the conclusions below are without tensorRT optimization, which 
is known to bring 15-30% extra computation performance depending on 
hardware and settings :

```
-> for batch size 4 to 8 , gain = +43%
-> for batch size 8 to 16 , gain = +60%
-> for batch size 16 to 32 , gain = +14%
-> for batch size 32 to 64 , gain = +10%
-> for batch size 64 to 128 , gain = +3.7%
```

- batch sizes 8 and 16 significant great increases speed on Tesla 
V100 with 6 cores / 12 cpu threads or less
- batch sizes higher 16 to 64 bring significant small increase speed 
on Tesla V100 with 6 cores / 12 cpu threads or less, but considering 
the loss of computing accuracy, this is not an efficient choice
- therefore, the most efficient batch size seems to be 16, providing 
**an average 900 simulations per second on Tesla V100**, and a 135% 
speed increase as compared to batch size 4
- batch size higher than 64 do not bring significant speed increases on 
Tesla V100 with 6 cores / 12 cpu threads or less

- on the CPU side, as of February 2019, PhoenixGo engine does not 
significantly benefit from a number of cpu threads higher than 2 
cpu cores/ 4 cpu threads, even on Tesla V100

For comparison, you can refer to 
[gtx-1060-75w-benchmark](benchmark-gtx1060-75w.md)
