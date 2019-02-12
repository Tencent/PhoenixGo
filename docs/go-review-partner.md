GoReviewPartner github page is [here](https://github.com/pnprog/goreviewpartner)

Support is still in testing, but so far these are the recommended 
settings to do use PhoenixGo with GoReviewPartner

3 config files are provided to you using optimized settings for grp 
(GoReviewPartner) :
- GPU with tensorRT (linux only) : 
[mcts_1gpu_grp.conf](/etc/mcts_1gpu_grp.conf)
- GPU without tensorRT (linux, windows) : 
[mcts_1gpu_notensorrt_grp.conf](/etc/mcts_1gpu_notensorrt_grp.conf)
- CPU-only (linux, mac, windows), much slower : 
[mcts_cpu_grp.conf](/etc/mcts_cpu_grp.conf)
 
note : it is also possible with multiple GPU, only the most common 
examples were shown here

if you want to do the changes manually, you need to :

in phoenixgo .conf config file :

- disable all time settings in config file (set to `0`)
- set time to unlimited (set timeout to 0 ms)
- use simulations per move to have fixed computation per move (playouts), 
because it is not needed to play fast since this is an analysis, 
unlimited time
- for the same reason, set `enable background search` to 0 (disable 
pondering), because this is not a live game, time settings are not 
needed and can cause conflicts
- add debug width and height with `debugger` in config file, see 
[FAQ question](/docs/FAQ.md/#a2-where-is-the-pv-analysis-)

in grp (GoReviewPartner) settings :
- it is easier to use one of the pre-made grp profile in config.ini 
(slightly modify them if needed)

- Don't forget to add paths in the config file as explained in 
[FAQ question](/docs/FAQ.md/#a5-ckptzerockpt-20b-v1fp32plan-error-no-such-file-or-directory)

- If you're on windows, you need to also pay attention to the syntax 
too, for example on windows it is `--logtostderr --v 1` not 
`--logtostderr --v=1` , see 
[FAQ question](/docs/FAQ.md/#a4-syntax-error-windows) for details and 
other differences

- and run the mcts_main (not start.sh) with the needed parameters, 
see an example 
[here](https://github.com/wonderingabout/goreviewpartner/blob/config-profiles-phoenixgo/config.ini#L100-L116)

also, see [#86](https://github.com/Tencent/PhoenixGo/issues/86) and 
[#99](https://github.com/pnprog/goreviewpartner/issues/99) for details
