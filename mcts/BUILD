load("//:rules.bzl", "cc_proto_library", "tf_cc_binary")

tf_cc_binary(
    name = "mcts_main",
    srcs = ["mcts_main.cc"],
    deps = [
        ":mcts_engine",
        "@boost//:asio",
    ],
)

tf_cc_binary(
    name = "debug_tool",
    srcs = [
        "debug_tool.cc",
        "mcts_config.h",
        "mcts_config.cc",
    ],
    deps = [
        ":mcts_config_cc_proto",
        "//common:go_comm",
        "//common:go_state",
        "//common:timer",
        "//model:zero_model",
        "//model:trt_zero_model",
    ],
)

cc_library(
    name = "mcts_engine",
    srcs = [
        "mcts_engine.cc",
        "mcts_monitor.cc",
        "mcts_debugger.cc",
        "byo_yomi_timer.cc",
    ],
    hdrs = [
        "mcts_engine.h",
        "mcts_monitor.h",
        "mcts_debugger.h",
        "byo_yomi_timer.h",
    ],
    deps = [
        ":mcts_config",
        "//common:go_comm",
        "//common:go_state",
        "//common:task_queue",
        "//common:wait_group",
        "//common:thread_conductor",
        "//common:str_utils",
        "//common:timer",
        "//model:zero_model",
        "//model:trt_zero_model",
        "//dist:dist_zero_model_client",
        "//dist:async_dist_zero_model_client",
        "@com_github_google_glog//:glog",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "mcts_config",
    srcs = ["mcts_config.cc"],
    hdrs = ["mcts_config.h"],
    deps = [
        ":mcts_config_cc_proto",
        "@com_github_google_glog//:glog",
    ],
    visibility = ["//visibility:public"],
)

cc_proto_library(
    name = "mcts_config_cc_proto",
    srcs = ["mcts_config.proto"],
    deps = [
        "//model:model_config_cc_proto",
        "//dist:dist_config_cc_proto",
    ],
    visibility = ["//visibility:public"],
)
