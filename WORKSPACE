workspace(name = "process_tree")

load(
    "@bazel_tools//tools/build_defs/repo:git.bzl",
    "git_repository",
)
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# Abseil LTS branch, Aug 2023
http_archive(
    name = "com_google_absl",
    strip_prefix = "abseil-cpp-20230802.0",
    urls = ["https://github.com/abseil/abseil-cpp/archive/refs/tags/20230802.0.tar.gz"],
)

# Protobuf 25.2
http_archive(
    name = "com_google_protobuf",
    strip_prefix = "protobuf-25.2",
    urls = ["https://github.com/protocolbuffers/protobuf/archive/v25.2.tar.gz"],
)

# Note: Protobuf deps must be loaded after defining the ABSL archive since
# protobuf repo would pull an in earlier version of ABSL.
load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")

protobuf_deps()

# Googletest - tag: release-1.12.1
http_archive(
    name = "com_google_googletest",
    sha256 = "ab78fa3f912d44d38b785ec011a25f26512aaedc5291f51f3807c592b506d33a",
    strip_prefix = "googletest-58d77fa8070e8cec2dc1ed015d66b454c8d78850",
    urls = ["https://github.com/google/googletest/archive/58d77fa8070e8cec2dc1ed015d66b454c8d78850.zip"],
)

http_archive(
    name = "build_bazel_rules_apple",
    sha256 = "8ac4c7997d863f3c4347ba996e831b5ec8f7af885ee8d4fe36f1c3c8f0092b2c",
    url = "https://github.com/bazelbuild/rules_apple/releases/download/2.5.0/rules_apple.2.5.0.tar.gz",
)

load("@build_bazel_rules_apple//apple:repositories.bzl", "apple_rules_dependencies")

apple_rules_dependencies()

# Hedron Bazel Compile Commands Extractor
# Allows integrating with clangd
# https://github.com/hedronvision/bazel-compile-commands-extractor
git_repository(
    name = "hedron_compile_commands",
    commit = "ac6411f8f347e5525038cb7858db4969db9e74f2",
    remote = "https://github.com/hedronvision/bazel-compile-commands-extractor.git",
    shallow_since = "1696885905 +0000",
)

load("@hedron_compile_commands//:workspace_setup.bzl", "hedron_compile_commands_setup")

hedron_compile_commands_setup()
