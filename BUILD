load("@build_bazel_rules_apple//apple:macos.bzl", "macos_unit_test")
load("@rules_cc//cc:defs.bzl", "cc_proto_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "process",
    hdrs = ["process.h"],
    deps = [
        "//annotations:annotator",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/synchronization",
    ],
)

objc_library(
    name = "process_tree_macos",
    srcs = [
        "process_tree_macos.mm",
    ],
    hdrs = [
        "process_tree_macos.h",
    ],
    sdk_dylibs = [
        "bsm",
    ],
    deps = [
        ":process",
    ],
)

cc_library(
    name = "process_tree",
    srcs = [
        "process_tree.cc",
    ],
    hdrs = [
        "process_tree.h",
    ],
    deps = [
        ":process",
        "//annotations:annotator",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/synchronization",
    ] + select({
        "@platforms//os:osx": [":process_tree_macos"],
        "@platforms//os:linux": [":process_tree_linux"],
        "//conditions:default": ["@platforms//:incompatible"],
    }),
)

proto_library(
    name = "process_tree_proto",
    srcs = ["process_tree.proto"],
)

cc_proto_library(
    name = "process_tree_cc_proto",
    deps = [":process_tree_proto"],
)

objc_library(
    name = "process_tree_macos_test_lib",
    srcs = [
        "process_tree_macos_test.mm",
    ],
    sdk_dylibs = [
        "bsm",
    ],
    deps = [
        ":process_tree",
    ],
)

macos_unit_test(
    name = "process_tree_macos_test",
    minimum_os_version = "11.0",
    deps = [
        ":process_tree_macos_test_lib",
    ],
)

cc_test(
    name = "process_tree_test",
    srcs = ["process_tree_test.cc"],
    deps = [
        ":process",
        ":process_tree",
        "//annotations:annotator",
        "@com_google_absl//absl/synchronization",
        "@com_google_googletest//:gtest_main",
    ],
)
