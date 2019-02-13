For your convenience, a copy of  `./mcts_main --help` output is provided to 
you below 

Date is January 2019, so it may be outdated if newer versions are released

`./mcts_main --help`

Outputs the below result :

```
mcts_main: Warning: SetUsageMessage() never called

  Flags from external/com_github_gflags_gflags/src/gflags.cc:
    -flagfile (load flags from file) type: string default: ""
    -fromenv (set flags from the environment [use 'export FLAGS_flag1=value'])
      type: string default: ""
    -tryfromenv (set flags from the environment if present) type: string
      default: ""
    -undefok (comma-separated list of flag names that it is okay to specify on
      the command line even if the program does not define a flag with that
      name.  IMPORTANT: flags in this list that have arguments MUST use the
      flag=value format) type: string default: ""

  Flags from external/com_github_gflags_gflags/src/gflags_completions.cc:
    -tab_completion_columns (Number of columns to use in output for tab
      completion) type: int32 default: 80
    -tab_completion_word (If non-empty, HandleCommandLineCompletions() will
      hijack the process and attempt to do bash-style command line flag
      completion on this value.) type: string default: ""

  Flags from external/com_github_gflags_gflags/src/gflags_reporting.cc:
    -help (show help on all flags [tip: all flags can have two dashes])
      type: bool default: false currently: true
    -helpfull (show help on all flags -- same as -help) type: bool
      default: false
    -helpmatch (show help on modules whose name contains the specified substr)
      type: string default: ""
    -helpon (show help on the modules named by this flag value) type: string
      default: ""
    -helppackage (show help on all modules in the main package) type: bool
      default: false
    -helpshort (show help on only the main module for this program) type: bool
      default: false
    -helpxml (produce an xml version of help) type: bool default: false
    -version (show version and build info and exit) type: bool default: false



  Flags from external/com_github_google_glog/src/logging.cc:
    -alsologtoemail (log messages go to these email addresses in addition to
      logfiles) type: string default: ""
    -alsologtostderr (log messages go to stderr in addition to logfiles)
      type: bool default: false
    -colorlogtostderr (color messages logged to stderr (if supported by
      terminal)) type: bool default: false
    -drop_log_memory (Drop in-memory buffers of log contents. Logs can grow
      very quickly and they are rarely read before they need to be evicted from
      memory. Instead, drop them from memory as soon as they are flushed to
      disk.) type: bool default: true
    -log_backtrace_at (Emit a backtrace when logging at file:linenum.)
      type: string default: ""
    -log_dir (If specified, logfiles are written into this directory instead of
      the default logging directory.) type: string default: ""
    -log_link (Put additional links to the log files in this directory)
      type: string default: ""
    -log_prefix (Prepend the log prefix to the start of each log line)
      type: bool default: true
    -logbuflevel (Buffer log messages logged at this level or lower (-1 means
      don't buffer; 0 means buffer INFO only; ...)) type: int32 default: 0
    -logbufsecs (Buffer log messages for at most this many seconds) type: int32
      default: 30
    -logemaillevel (Email log messages logged at this level or higher (0 means
      email all; 3 means email FATAL only; ...)) type: int32 default: 999
    -logfile_mode (Log file mode/permissions.) type: int32 default: 436
    -logmailer (Mailer used to send logging email) type: string
      default: "/bin/mail"
    -logtostderr (log messages go to stderr instead of logfiles) type: bool
      default: false
    -max_log_size (approx. maximum log file size (in MB). A value of 0 will be
      silently overridden to 1.) type: int32 default: 1800
    -minloglevel (Messages logged at a lower level than this don't actually get
      logged anywhere) type: int32 default: 0
    -stderrthreshold (log messages at or above this level are copied to stderr
      in addition to logfiles.  This flag obsoletes --alsologtostderr.)
      type: int32 default: 2
    -stop_logging_if_full_disk (Stop attempting to log to disk if the disk is
      full.) type: bool default: false

  Flags from external/com_github_google_glog/src/vlog_is_on.cc:
    -v (Show all VLOG(m) messages for m <= this. Overridable by --vmodule.)
      type: int32 default: 0
    -vmodule (per-module verbose level. Argument is a comma-separated list of
      <module name>=<log level>. <module name> is a glob pattern, matched
      against the filename base (that is, name ignoring .cc/.h./-inl.h). <log
      level> overrides any value given by --v.) type: string default: ""



  Flags from mcts/mcts_main.cc:
    -allow_ip (List of client ip allowed to connect, seperated by comma.)
      type: string default: ""
    -config_path (Path of mcts config file.) type: string default: ""
    -fork_per_request (Fork for each request or not.) type: bool default: true
    -gpu_list (List of gpus used by neural network.) type: string default: ""
    -gtp (Run as gtp server.) type: bool default: false
    -init_moves (Initialize Go board with init_moves.) type: string default: ""
    -inter_op_parallelism_threads (Number of tf's inter op threads) type: int32
      default: 0
    -intra_op_parallelism_threads (Number of tf's intra op threads) type: int32
      default: 0
    -listen_port (Listen which port.) type: int32 default: 0

```
