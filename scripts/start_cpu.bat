@echo off

setlocal

set config=etc\mcts_cpu.conf

echo use config file '%config%' >&2

pushd %~dp0..

echo log to %CD%\log >&2
md log 2>NUL

echo start mcts_main >&2
x64\Release\mcts_main --config_path=%config% --gtp --log_dir=log --v=1

popd