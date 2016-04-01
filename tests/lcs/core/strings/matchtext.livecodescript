script "CoreStringsMatchText"

-- Bug 16: Ensure that very deeply recursive regular expressions do
-- not cause a stack overflow in libpcre
constant kRecursionExpr = "x(?R)?"
constant kRecursionLength = 16384

on TestMatchTextRecursion

   -- Build a suitably long string
   local tTarget
   put "x" into tTarget
   repeat
      if length(tTarget) >= kRecursionLength then
         delete char (1+kRecursionLength) to -1 of tTarget
         exit repeat
      end if
      put tTarget after tTarget
   end repeat

   get matchText(tTarget, kRecursionExpr)

end TestMatchTextRecursion
