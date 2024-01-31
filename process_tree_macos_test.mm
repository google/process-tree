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
#import <Foundation/Foundation.h>
#import <XCTest/XCTest.h>

#include <bsm/libbsm.h>

#include "process_tree.h"

// We can't test the full backfill process, as retrieving information on
// processes (with task_name_for_pid) requires privileges.
// Test what we can by LoadPID'ing ourselves.

@interface LoadPIDTest : XCTestCase
@end

@implementation LoadPIDTest

- (void)testLoadPID {
  auto proc = process_tree::LoadPID(getpid()).value();

  audit_token_t self_tok;
  mach_msg_type_number_t count = TASK_AUDIT_TOKEN_COUNT;
  XCTAssertEqual(task_info(mach_task_self(), TASK_AUDIT_TOKEN, (task_info_t)&self_tok, &count),
                 KERN_SUCCESS);

  XCTAssertEqual(proc.pid_.pid, audit_token_to_pid(self_tok));
  XCTAssertEqual(proc.pid_.pidversion, audit_token_to_pidversion(self_tok));

  XCTAssertEqual(proc.effective_cred_.uid, geteuid());
  XCTAssertEqual(proc.effective_cred_.gid, getegid());

  [[[NSProcessInfo processInfo] arguments]
    enumerateObjectsUsingBlock:^(NSString *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
      XCTAssertEqualObjects(@(proc.program_->arguments[idx].c_str()), obj);
      if (idx == 0) {
        XCTAssertEqualObjects(@(proc.program_->executable.c_str()), obj);
      }
    }];
}

@end
