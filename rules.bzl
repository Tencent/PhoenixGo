load("@protobuf_archive//:protobuf.bzl", protobuf_cc_proto_library="cc_proto_library")


def cc_proto_library(name, srcs=[], deps=[], use_grpc_plugin=False, **kwargs):
    protobuf_cc_proto_library(
        name=name,
        srcs=srcs,
        deps=deps,
        use_grpc_plugin=use_grpc_plugin,
        cc_libs = ["@protobuf_archive//:protobuf"],
        protoc="@protobuf_archive//:protoc",
        default_runtime="@protobuf_archive//:protobuf",
        **kwargs
    )


def tf_cc_binary(name, srcs=[], deps=[], linkopts=[], **kwargs):
    native.cc_binary(
        name=name,
        srcs=srcs + ["@org_tensorflow//tensorflow:libtensorflow_framework.so"],
        deps=deps,
        **kwargs
    )
