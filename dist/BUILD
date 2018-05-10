load("//:rules.bzl", "cc_proto_library", "tf_cc_binary")

tf_cc_binary(
    name = "dist_zero_model_server",
    srcs = [
        "dist_zero_model_server.cc",
    ],
    deps = [
        ":dist_zero_model_cc_proto",
        "//common:timer",
        "//model:zero_model",
        "//model:trt_zero_model",
        "@com_github_google_glog//:glog",
    ],
)

cc_library(
    name = "dist_zero_model_client",
    srcs = ["dist_zero_model_client.cc"],
    hdrs = ["dist_zero_model_client.h"],
    deps = [
        ":dist_config_cc_proto",
        ":dist_zero_model_cc_proto",
        ":leaky_bucket",
        "//model:zero_model_base",
        "@com_github_google_glog//:glog",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "async_dist_zero_model_client",
    srcs = ["async_dist_zero_model_client.cc"],
    hdrs = ["async_dist_zero_model_client.h"],
    deps = [
        ":dist_config_cc_proto",
        ":dist_zero_model_cc_proto",
        ":async_rpc_queue",
        ":leaky_bucket",
        "//model:zero_model_base",
        "@com_github_google_glog//:glog",
    ],
    visibility = ["//visibility:public"],
)

cc_proto_library(
    name = "dist_config_cc_proto",
    srcs = ["dist_config.proto"],
    visibility = ["//visibility:public"],
)

cc_proto_library(
    name = "dist_zero_model_cc_proto",
    srcs = ["dist_zero_model.proto"],
    deps = ["//model:model_config_cc_proto"],
    use_grpc_plugin = True,
)

cc_library(
    name = "async_rpc_queue",
    hdrs = ["async_rpc_queue.h"],
    deps = ["@grpc//:grpc++_unsecure"],
)

cc_library(
    name = "leaky_bucket",
    srcs = ["leaky_bucket.cc"],
    hdrs = ["leaky_bucket.h"],
)
