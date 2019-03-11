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
    urls = ["https://github.com/tensorflow/tensorflow/archive/v1.13.1.tar.gz"],
    sha256 = "7cd19978e6bc7edc2c847bce19f95515a742b34ea5e28e4389dade35348f58ed",
    strip_prefix = "tensorflow-1.13.1",
    patches = ["//third_party/tensorflow:tensorflow.patch"],
)

http_archive(
    name = "io_bazel_rules_closure",
    sha256 = "a38539c5b5c358548e75b44141b4ab637bba7c4dc02b46b1f62a96d6433f56ae",
    strip_prefix = "rules_closure-dbb96841cc0a5fb2664c37822803b06dab20c7d1",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/rules_closure/archive/dbb96841cc0a5fb2664c37822803b06dab20c7d1.tar.gz",
        "https://github.com/bazelbuild/rules_closure/archive/dbb96841cc0a5fb2664c37822803b06dab20c7d1.tar.gz",  # 2018-04-13
    ],
)

load('@org_tensorflow//tensorflow:workspace.bzl', 'tf_workspace')
tf_workspace(path_prefix = "", tf_repo_name = "org_tensorflow")

http_archive(
    name = "com_github_nelhage_rules_boost",
    urls = ["https://github.com/nelhage/rules_boost/archive/6d6fd834281cb8f8e758dd9ad76df86304bf1869.tar.gz"],
    sha256 = "9adb4899e40fc10871bab1ff2e8feee950c194eec9940490f65a2761bbe6941d",
    strip_prefix = "rules_boost-6d6fd834281cb8f8e758dd9ad76df86304bf1869",
)

load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")
boost_deps()
