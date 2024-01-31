/// Copyright 2023 Google LLC
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     https://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
#include "process_tree.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "absl/synchronization/mutex.h"
#include "gtest/gtest.h"

#include "annotations/annotator.h"
#include "process.h"

namespace process_tree {

// Peer to ProcessTree with an InsertInit method for testing.
class ProcessTreeTestPeer : public ProcessTree {
 public:
  explicit ProcessTreeTestPeer(std::vector<std::unique_ptr<Annotator>> &&annotators) : ProcessTree(std::move(annotators)) {}
  std::shared_ptr<const Process> InsertInit();
};

std::shared_ptr<const Process> ProcessTreeTestPeer::InsertInit() {
  absl::MutexLock lock(&mtx_);
  struct Pid initpid = {
      .pid = 1,
      .pidversion = 1,
  };
  auto proc = std::make_shared<Process>(
      initpid, (Cred){.uid = 0, .gid = 0},
      std::make_shared<Program>(
          (Program){.executable = "/init", .arguments = {"/init"}}),
      nullptr);
  map_.emplace(initpid, proc);
  return proc;
}

static constexpr std::string_view kAnnotatedExecutable = "/usr/bin/login";

class TestAnnotator : public Annotator {
 public:
  TestAnnotator() = default;
  ~TestAnnotator() override = default;
  void AnnotateFork(ProcessTree &tree, const Process &parent,
                    const Process &child) override;
  void AnnotateExec(ProcessTree &tree, const Process &orig_process,
                    const Process &new_process) override;
  std::optional<Annotations> Proto() const override;
};

void TestAnnotator::AnnotateFork(ProcessTree &tree, const Process &parent,
                                 const Process &child) {
  // "Base case". Propagate existing annotations down to descendants.
  if (auto annotation = tree.GetAnnotation<TestAnnotator>(parent)) {
    tree.AnnotateProcess(child, std::move(*annotation));
  }
}

void TestAnnotator::AnnotateExec(ProcessTree &tree, const Process &orig_process,
                                 const Process &new_process) {
  if (auto annotation = tree.GetAnnotation<TestAnnotator>(orig_process)) {
    tree.AnnotateProcess(new_process, std::move(*annotation));
    return;
  }

  if (new_process.program_->executable == kAnnotatedExecutable) {
    tree.AnnotateProcess(new_process, std::make_shared<TestAnnotator>());
  }
}

std::optional<Annotations> TestAnnotator::Proto() const {
  return std::nullopt;
}

class ProcessTreeTest : public testing::Test {
 protected:
  void SetUp() override {
    std::vector<std::unique_ptr<Annotator>> annotators{};
    tree_ = std::make_shared<ProcessTreeTestPeer>(std::move(annotators));
    init_proc_ = tree_->InsertInit();
  }

  std::shared_ptr<ProcessTreeTestPeer> tree_;
  std::shared_ptr<const Process> init_proc_;
};

TEST_F(ProcessTreeTest, SimpleOps) {
  uint64_t event_id = 1;
  // PID 1.1: fork() -> PID 2.2
  const struct Pid child_pid = {.pid = 2, .pidversion = 2};
  tree_->HandleFork(event_id++, *init_proc_, child_pid);

  auto child_opt = tree_->Get(child_pid);
  ASSERT_TRUE(child_opt.has_value());
  std::shared_ptr<const Process> child = *child_opt;
  EXPECT_EQ(child->pid_, child_pid);
  EXPECT_EQ(child->program_, init_proc_->program_);
  EXPECT_EQ(child->effective_cred_, init_proc_->effective_cred_);
  EXPECT_EQ(tree_->GetParent(*child), init_proc_);

  // PID 2.2: exec("/bin/bash") -> PID 2.3
  const struct Pid child_exec_pid = {.pid = 2, .pidversion = 3};
  const struct Program child_exec_prog = {.executable = "/bin/bash",
                                          .arguments = {"/bin/bash", "-i"}};
  tree_->HandleExec(event_id++, *child, child_exec_pid, child_exec_prog,
                    child->effective_cred_);

  child_opt = tree_->Get(child_exec_pid);
  ASSERT_TRUE(child_opt.has_value());
  child = *child_opt;
  EXPECT_EQ(child->pid_, child_exec_pid);
  EXPECT_EQ(*child->program_, child_exec_prog);
  EXPECT_EQ(child->effective_cred_, init_proc_->effective_cred_);
}

TEST_F(ProcessTreeTest, LoadProgram) {
  std::vector<std::unique_ptr<Annotator>> annotators{};
  annotators.emplace_back(std::make_unique<TestAnnotator>());
  tree_ = std::make_shared<ProcessTreeTestPeer>(std::move(annotators));

  uint64_t event_id = 1;
  const struct Cred cred = {.uid = 0, .gid = 0};

  // PID 1.1: fork() -> PID 2.2
  const struct Pid login_pid = {.pid = 2, .pidversion = 2};
  tree_->HandleFork(event_id++, *init_proc_, login_pid);

  // PID 2.2: exec("/usr/bin/login") -> PID 2.3
  const struct Pid login_exec_pid = {.pid = 2, .pidversion = 3};
  const struct Program login_prog = {
      .executable = std::string(kAnnotatedExecutable), .arguments = {}};
  auto login = *tree_->Get(login_pid);
  tree_->HandleExec(event_id++, *login, login_exec_pid, login_prog, cred);

  // Ensure we have an annotation on login itself...
  login = *tree_->Get(login_exec_pid);
  auto annotation = tree_->GetAnnotation<TestAnnotator>(*login);
  EXPECT_TRUE(annotation.has_value());

  // PID 2.3: fork() -> PID 3.3
  const struct Pid shell_pid = {.pid = 3, .pidversion = 3};
  tree_->HandleFork(event_id++, *login, shell_pid);
  // PID 3.3: exec("/bin/zsh") -> PID 3.4
  const struct Pid shell_exec_pid = {.pid = 3, .pidversion = 4};
  const struct Program shell_prog = {.executable = "/bin/zsh", .arguments = {}};
  auto shell = *tree_->Get(shell_pid);
  tree_->HandleExec(event_id++, *shell, shell_exec_pid, shell_prog, cred);

  // ... and also ensure we have an annotation on the descendant zsh.
  shell = *tree_->Get(shell_exec_pid);
  annotation = tree_->GetAnnotation<TestAnnotator>(*shell);
  EXPECT_TRUE(annotation.has_value());
}

TEST_F(ProcessTreeTest, Cleanup) {
  uint64_t event_id = 1;
  const struct Pid child_pid = {.pid = 2, .pidversion = 2};
  {
    tree_->HandleFork(event_id++, *init_proc_, child_pid);
    auto child = *tree_->Get(child_pid);
    tree_->HandleExit(event_id++, *child);
  }

  // We should still be able to get a handle to child...
  {
    auto child = tree_->Get(child_pid);
    EXPECT_TRUE(child.has_value());
  }

  // ... until we step far enough into the future (32 events).
  struct Pid churn_pid = {.pid = 3, .pidversion = 3};
  for (int i = 0; i < 32; i++) {
    tree_->HandleFork(event_id++, *init_proc_, churn_pid);
    churn_pid.pid++;
  }

  // Now when we try processing the next event, it should have fallen out of the
  // tree.
  tree_->HandleFork(event_id++, *init_proc_, churn_pid);
  {
    auto child = tree_->Get(child_pid);
    EXPECT_FALSE(child.has_value());
  }
}

}  // namespace process_tree
