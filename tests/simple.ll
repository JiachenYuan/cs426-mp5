; ModuleID = '<stdin>'
source_filename = "simple.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main() #0 {
  br label %1

1:                                                ; preds = %8, %0
  %.02 = phi i32 [ 0, %0 ], [ %9, %8 ]
  %.01 = phi i32 [ 0, %0 ], [ %.1, %8 ]
  %2 = icmp ult i32 %.02, 5
  br i1 %2, label %3, label %10

3:                                                ; preds = %1, %5
  %.1 = phi i32 [ %6, %5 ], [ %.01, %1 ]
  %.0 = phi i32 [ %7, %5 ], [ 0, %1 ]
  %4 = icmp ult i32 %.0, 3
  br i1 %4, label %5, label %8

5:                                                ; preds = %3
  %6 = add nsw i32 %.1, 1
  %7 = add nuw nsw i32 %.0, 1
  br label %3, !llvm.loop !6

8:                                                ; preds = %3
  %9 = add nuw nsw i32 %.02, 1
  br label %1, !llvm.loop !8

10:                                               ; preds = %1
  ret i32 %.01
}

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Ubuntu clang version 15.0.7"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
!8 = distinct !{!8, !7}
