script "CoreControlRepeat"
/*
Copyright (C) 2015 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

on TestRepeatForever
  local tCounter
  put 0 into tCounter
  repeat forever
    add 1 to tCounter
    if tCounter is 1000 then
      exit repeat
    end if
  end repeat
  TestAssert "repeat forever runs 1000 times", tCounter is 1000
end TestRepeatForever

on TestRepeatCount
  local tCounter
  put 1000 into tCounter
  repeat 1000 times
    subtract 1 from tCounter
  end repeat
  TestAssert "repeat 1000 times runs 1000 times", tCounter is 0
end TestRepeatCount

on TestRepeatForEachKeyInArray
  local tArray
  put 1 into tArray["foo"]
  put 2 into tArray["bar"]
  put 3 into tArray["baz"]

  local tTotal, tLastKey
  repeat for each key tKey in tArray
     add tArray[tKey] to tTotal
     put tKey into tLastKey
  end repeat
  TestAssert "repeat for each key in array", tTotal is 6
  TestAssert "repeat for each key in array preserves last value", tKey is tLastKey
  
  put "something" into tKey
  repeat for each key tKey in empty
  end repeat
  TestAssert "repeat for each key in empty preserves initial value", tKey is "something"
end TestRepeatForEachKeyInArray

on TestRepeatForEachKeyInSequence
   local tArray, tExpectedString
   repeat with i = 1 to 10000
      put i into tArray[i]
      put i & comma after tExpectedString
   end repeat
   
   local tComputedString
   repeat for each key tKey in tArray
      put tArray[tKey] & comma after tComputedString
   end repeat
   
   TestAssertBroken "repeat for each key in sequence", tComputedString is tExpectedString, "anomaly 16449"
end TestRepeatForEachKeyInSequence

on TestRepeatForEachElementInArray
  local tArray
  put 1 into tArray["foo"]
  put 2 into tArray["bar"]
  put 3 into tArray["baz"]

  local tTotal, tLastElement
  repeat for each element tElement in tArray
     add tElement to tTotal
     put tElement into tLastElement
  end repeat
  TestAssert "repeat for each element in array", tTotal is 6
  TestAssert "repeat for each element preserves last value", tElement is tLastElement
  
  put "something" into tElement
  repeat for each element tElement in empty
  end repeat
  TestAssert "repeat for each element in empty preserves initial value", tElement is "something"
end TestRepeatForEachElementInArray

on TestRepeatForEachElementInSequence
   local tArray, tExpectedString
   repeat with i = 1 to 10000
      put i into tArray[i]
      put i & comma after tExpectedString
   end repeat
   
   local tComputedString
   repeat for each element tElement in tArray
      put tElement & comma after tComputedString
   end repeat
   TestAssert "repeat for each element in sequence", tComputedString is tExpectedString
   TestAssert "repeat for each element in sequence preserves last value", tElement is 10000
end TestRepeatForEachElementInSequence

on TestRepeatForEachByte
   local tBytes, tExpectedString
   repeat with i = 0 to 255
      put numToByte(i) after tBytes
      put i & comma after tExpectedString
   end repeat
   
   local tComputedString
   repeat for each byte tByte in tBytes
      put byteToNum(tByte) & comma after tComputedString
   end repeat
   TestAssert "repeat for each byte", tComputedString is tExpectedString
   TestAssert "repeat for each byte preserves last value", tByte is the last byte of tBytes
   
   put "something" into tByte
   repeat for each byte tByte in empty
   end repeat
  TestAssert "repeat for each byte in empty preserves initial value", tByte is "something"
end TestRepeatForEachByte

on TestRepeatForEachChar
   local tString, tExpectedCharString, tExpectedCodepointString, tExpectedCodeunitString, tComputedString
   
   repeat for each word tType in "native simple combining surrogates"
      switch tType
         case "native"
            -- native boundaries are the same for char, codepoint and codeunit
            put "foo" into tString
            put "f,o,o," into tExpectedCharString
            put "f,o,o," into tExpectedCodepointString
            put "f,o,o," into tExpectedCodeunitString
            break
            
         case "simple"
            -- unicode characters which are 1-1-1 (char,unit,point)
            -- we use greek alpha, beta and gamma
            put numToCodepoint(0x0391) & numToCodepoint(0x392) & numToCodepoint(0x393) into tString
            put numToCodepoint(0x0391) , numToCodepoint(0x392) , numToCodepoint(0x393) , empty into tExpectedCharString
            put tExpectedCharString into tExpectedCodepointString
            put tExpectedCharString into tExpectedCodeunitString
            break
            
         case "combining"
            -- unicode characters which give 2-1-1 (char,unit,point)
            put "fo" & numToCodepoint(0x301) & "o" into tString
            put "f", "o" & numToCodepoint(0x301) , "o" , empty into tExpectedCharString
            put "f", "o", numToCodepoint(0x301) , "o" , empty into tExpectedCodepointString
            put tExpectedCodepointString into tExpectedCodeunitString
            break
      end switch
      
      put empty into tComputedString
      repeat for each character tChar in tString
         put tChar & comma after tComputedString
      end repeat
      TestAssert "repeat for each character -" && tType, tComputedString is tExpectedCharString
      
      put empty into tComputedString
      repeat for each codepoint tChar in tString
         put tChar & comma after tComputedString
      end repeat
      TestAssert "repeat for each codepoint -" && tType, tComputedString is tExpectedCodepointString
      
      put empty into tComputedString
      repeat for each codeunit tChar in tString
         put tChar & comma after tComputedString
      end repeat
      TestAssert "repeat for each codeunit -" && tType, tComputedString is tExpectedCodeunitString
   end repeat
   
   repeat for each character tChar in "abc"
   end repeat
   TestAssert "repeat for each character preserves last value", tChar is "c"
   
   repeat for each codepoint tChar in "abc"
   end repeat
   TestAssert "repeat for each codepoint preserves last value", tChar is "c"
   
   repeat for each codeunit tChar in "abc"
   end repeat
   TestAssert "repeat for each codeunit preserves last value", tChar is "c"
   
   put "something" into tChar
   repeat for each character tChar in empty
   end repeat
   TestAssertBroken "repeat for each character in empty preserves initial value", tChar is "something", "anomaly 16454"
   
   put "something" into tChar
   repeat for each codepoint tChar in empty
   end repeat
   TestAssertBroken "repeat for each codepoint in empty preserves initial value", tChar is "something", "anomaly 16454"
   
   put "something" into tChar
   repeat for each codeunit tChar in empty
   end repeat
   TestAssertBroken "repeat for each codeunit in empty preserves initial value", tChar is "something", "anomaly 16454"
end TestRepeatForEachChar

on TestRepeatForEachDelimitedChunk
   local tChunks, tComputedChunks
   put "foo,bar,baz,foobar,foobaz,barbaz" into tChunks
   
   put empty into tComputedChunks
   repeat for each item tChunk in tChunks
      put tChunk & comma after tComputedChunks
   end repeat
   TestAssert "repeat for each item", tComputedChunks is (tChunks & comma)
   TestAssert "repeat for each item preserves last value", tChunk is the last item of tChunks
   
   put empty into tComputedChunks
   repeat for each item tChunk in (tChunks & comma)
      put tChunk & comma after tComputedChunks
   end repeat
   TestAssert "repeat for each item trailing delimiter", tComputedChunks is (tChunks & comma)
   TestAssert "repeat for each item trailing delimiter preserves last value", tChunk is the last item of tChunks
   
   put "something" into tChunk
   repeat for each item tChunk in empty
   end repeat
   TestAssertBroken "repeat for each item in empty preserves initial value", tChunk is "something", "anomaly 16454"
   
   replace comma with return in tChunks
   
   put empty into tComputedChunks
   repeat for each line tChunk in tChunks
      put tChunk & return after tComputedChunks
   end repeat
   TestAssert "repeat for each line", tComputedChunks is (tChunks & return)
   TestAssert "repeat for each line preserves last value", tChunk is the last line of tChunks
   
   put empty into tComputedChunks
   repeat for each line tChunk in (tChunks & return)
      put tChunk & return after tComputedChunks
   end repeat
   TestAssert "repeat for each line trailing delimiter", tComputedChunks is (tChunks & return)
   TestAssert "repeat for each line trailing delimiter preserves last value", tChunk is the last line of tChunks
   
   put "something" into tChunk
   repeat for each line tChunk in empty
   end repeat
   TestAssertBroken "repeat for each line in empty preserves initial value", tChunk is "something", "anomaly 16454"
end TestRepeatForEachDelimitedChunk

on TestRepeatForEachSegment
   local tSegments, tExpectedString
   put "  the    " & quote & "quick brown" & quote & " fox" into tSegments
   put "the," & quote & "quick brown" & quote & ",fox," into tExpectedString
   
   local tComputedString
   repeat for each segment tSegment in tSegments
      put tSegment & comma after tComputedString
   end repeat
   TestAssert "repeat for each segment", tComputedString is tExpectedString
   TestAssert "repeat for each segment preserves last value", tSegment is the last segment of tSegments
   
   put "something" into tSegment
   repeat for each segment tSegment in empty
   end repeat
   TestAssertBroken "repeat for each segment in empty preserves initial value", tSegment is "something", "anomaly 16454"
end TestRepeatForEachSegment

on TestRepeatForEachTrueWord
   local tTrueWords, tExpectedString
   put "  the    " & quote & "quick brown" & quote & " fox" into tTrueWords
   put "the,quick,brown,fox," into tExpectedString
   
   local tComputedString
   repeat for each trueWord tTrueWord in tTrueWords
      put tTrueWord & comma after tComputedString
   end repeat
   TestAssert "repeat for each true word", tComputedString is tExpectedString
   TestAssert "repeat for each true word preserves last value", tTrueWord is the last trueWord of tTrueWords
   
   put "something" into tTrueWord
   repeat for each trueWord tTrueWord in empty
   end repeat
   TestAssertBroken "repeat for each true word in empty preserves initial value", tTrueWord is "something", "anomaly 16454"
end TestRepeatForEachTrueWord
