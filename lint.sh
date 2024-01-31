#!/bin/bash
set -xo pipefail

GIT_ROOT=$(git rev-parse --show-toplevel)

find $GIT_ROOT \( -name "*.m" -o -name "*.h" -o -name "*.mm" -o -name "*.cc" \) -exec clang-format --Werror {} \+

go install github.com/bazelbuild/buildtools/buildifier@latest
~/go/bin/buildifier --lint=fix -r $GIT_ROOT
