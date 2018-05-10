cc_library(
    name = "go_comm",
    srcs = ["go_comm.cc"],
    hdrs = ["go_comm.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "go_state",
    srcs = ["go_state.cc"],
    hdrs = ["go_state.h"],
    deps = [":go_comm"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "errordef",
    hdrs = ["errordef.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "task_queue",
    hdrs = ["task_queue.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "wait_group",
    srcs = ["wait_group.cc"],
    hdrs = ["wait_group.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "thread_conductor",
    srcs = ["thread_conductor.cc"],
    hdrs = ["thread_conductor.h"],
    deps = [":wait_group"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "timer",
    srcs = ["timer.cc"],
    hdrs = ["timer.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "str_utils",
    srcs = ["str_utils.cc"],
    hdrs = ["str_utils.h"],
    visibility = ["//visibility:public"],
)
