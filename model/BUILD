load("//:rules.bzl", "cc_proto_library")
load("@org_tensorflow//tensorflow:tensorflow.bzl", "if_tensorrt")

cc_binary(
    name = "build_tensorrt_model",
    srcs = ["build_tensorrt_model.cc"],
    deps = [
        "@com_github_google_glog//:glog",
    ] + if_tensorrt([
        "@local_config_tensorrt//:nv_infer",
        "@local_config_tensorrt//:nv_parsers",
    ]),
    copts = if_tensorrt(["-DGOOGLE_TENSORRT=1"]),
)

cc_library(
    name = "zero_model",
    srcs = [
        "zero_model.cc",
    ],
    hdrs = ["zero_model.h"],
    deps = [
        ":zero_model_base",
        ":checkpoint_utils",
        "@com_github_google_glog//:glog",
        "@org_tensorflow//tensorflow/core:tensorflow",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "trt_zero_model",
    srcs = ["trt_zero_model.cc"],
    hdrs = ["trt_zero_model.h"],
    deps = [
        ":zero_model_base",
        "@boost//:filesystem",
        "@com_github_google_glog//:glog",
    ] + if_tensorrt([
        "@local_config_tensorrt//:nv_infer",
    ]),
    copts = if_tensorrt(["-DGOOGLE_TENSORRT=1"]),
    visibility = ["//visibility:public"],
)

cc_library(
    name = "zero_model_base",
    hdrs = ["zero_model_base.h"],
    deps = [
        ":model_config_cc_proto",
        "//common:errordef",
    ],
    visibility = ["//visibility:public"],
)

cc_proto_library(
    name = "model_config_cc_proto",
    srcs = ["model_config.proto"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "checkpoint_utils",
    srcs = ["checkpoint_utils.cc"],
    hdrs = ["checkpoint_utils.h"],
    deps = [
        ":checkpoint_state_cc_proto",
        "@boost//:filesystem",
        "@com_github_google_glog//:glog",
    ],
    visibility = ["//visibility:public"],
)

cc_proto_library(
    name = "checkpoint_state_cc_proto",
    srcs = ["checkpoint_state.proto"],
)
