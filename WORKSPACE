load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "com_github_google_glog",
    urls = ["https://github.com/google/glog/archive/55cc27b6eca3d7906fc1a920ca95df7717deb4e7.tar.gz"],
    sha256 = "4966f4233e4dcac53c7c7dd26054d17c045447e183bf9df7081575d2b888b95b",
    strip_prefix = "glog-55cc27b6eca3d7906fc1a920ca95df7717deb4e7",
    patches = ["//third_party/glog:glog.patch"],
)

http_archive(
    name = "org_tensorflow",
    urls = ["https://github.com/tensorflow/tensorflow/archive/v1.8.0.tar.gz"],
    sha256 = "47646952590fd213b747247e6870d89bb4a368a95ae3561513d6c76e44f92a75",
    strip_prefix = "tensorflow-1.8.0",
    patches = ["//third_party/tensorflow:tensorflow.patch"],
)

http_archive(
    name = "io_bazel_rules_closure",
    sha256 = "6691c58a2cd30a86776dd9bb34898b041e37136f2dc7e24cadaeaf599c95c657",
    strip_prefix = "rules_closure-08039ba8ca59f64248bb3b6ae016460fe9c9914f",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/rules_closure/archive/08039ba8ca59f64248bb3b6ae016460fe9c9914f.tar.gz",
        "https://github.com/bazelbuild/rules_closure/archive/08039ba8ca59f64248bb3b6ae016460fe9c9914f.tar.gz",  # 2018-01-16
    ],
)

load('@org_tensorflow//tensorflow:workspace.bzl', 'tf_workspace')
tf_workspace(path_prefix = "", tf_repo_name = "org_tensorflow")

http_archive(
    name = "com_github_nelhage_rules_boost",
    urls = ["https://github.com/nelhage/rules_boost/archive/769c22fa72177314cd6fae505bd36116d6ed1f6b.tar.gz"],
    sha256 = "b2fccaabeb6ee243c7632dd2a4d0419c78e43e1f063cb4b6a117630d060b440e",
    strip_prefix = "rules_boost-769c22fa72177314cd6fae505bd36116d6ed1f6b",
)

load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")
boost_deps()
