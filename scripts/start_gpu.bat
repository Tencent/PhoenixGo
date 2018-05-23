@echo off

setlocal

set "PATH=%PATH%;C:\Program Files\NVIDIA Corporation\NVSMI"
for /f %%i in ('nvidia-smi -L ^| find /v /c ""') do set num_gpu=%%i
echo found %num_gpu% GPU(s) >&2
set config=etc\mcts_%num_gpu%gpu_notensorrt.conf

echo use config file '%config%' >&2

pushd %~dp0..

echo log to %CD%\log >&2
md log 2>NUL

echo start mcts_main >&2
x64\Release\mcts_main --config_path=%config% --gtp --log_dir=log --v=1

popd