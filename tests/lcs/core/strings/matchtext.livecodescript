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

on TestMatchTextMultipleMatches
    local tFound
    repeat for each item tItem in "a1,b1,a2,a3,a4,a5,a0"
         get matchText(tItem, "a([0-9])", tFound)
   end repeat

   TestAssert "check that tFound is not empty", tFound is 0

   repeat for each item tItem in "c1,c2,c3,c4"
        get matchText(tItem, "a([0-9])", tFound)
   end repeat

   TestAssert "check that tFound is empty", tFound is 0
end TestMatchTextMultipleMatches