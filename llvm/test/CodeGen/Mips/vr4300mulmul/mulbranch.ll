; RUN: llc -march=mips -mfix4300 -verify-machineinstrs < %s | FileCheck %s

; Function Attrs: nounwind
define dso_local i32 @my_func(i32 signext %a) local_unnamed_addr #0 {
; CHECK: mul
; CHECK-NEXT: nop
  %mul = mul nsw i32 %a, %a
  %call = tail call i32 @foo(i32 signext %mul) #2
  ret i32 %call
}

declare dso_local i32 @foo(i32 signext) local_unnamed_addr #1
