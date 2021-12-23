; RUN: llc -march=mips -mfix4300 -verify-machineinstrs < %s | FileCheck %s

; Function Attrs: mustprogress nofree norecurse nosync nounwind readnone willreturn
define dso_local i32 @fun(i32 signext %x, i32 signext %y) local_unnamed_addr #0 {
; CHECK: mul
; CHECK-NEXT: nop
; CHECK-NEXT: mul
  %mul = mul nsw i32 %x, %x
  %mul1 = mul nsw i32 %y, %y
  %add = add nuw nsw i32 %mul1, %mul
  ret i32 %add
}
