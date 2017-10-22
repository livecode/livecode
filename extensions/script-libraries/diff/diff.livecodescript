script "com.livecode.library.diff"
/*
Copyright (C) 2016-2017 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>. 
*/

on extensionInitialize
   if the target is not me then
      pass extensionInitialize
   end if
   
   insert the script of me into back
   
   if the environment contains "development" then
      set the _ideoverride of me to true
   end if
end extensionInitialize

on extensionFinalize
   if the target is not me then
      pass extensionFinalize
   end if
   
   remove the script of me from back
end extensionFinalize

/**
Title: Unified Diff Library 

Author: LiveCode

Version: 1.0.0

Description: 
Text comparison library, handling differences in unified diff format
*/

constant kDiffOperationAdd = "+"
constant kDiffOperationDelete = "-"

/**

Produce a diff representing the changes between two text strings

Description:
Compare two strings, producing a string representing the changes between them in unified-diff format.

Example:

Parameters:
pFrom (string):  The original source string. The produced diff will describe how to transform this string.

pTo (string): The updated source string. This will be compared to pFrom to produce the output diff.

pContext (integer): The number of unchanged lines to include before / after each change in the diff.

Returns:
A string representing the changes between pFrom and pTo in unified diff format.

*/

function DiffCompare pFrom, pTo, pContext
   -- default to 3 lines of context if not given
   if pContext is empty then
      put 3 into pContext
   end if
   
   put _split(pFrom, return) into pFrom
   put _split(pTo, return) into pTo
   
   local tEdits
   put diff_arrays(pFrom, pTo) into tEdits
   
   return convert_to_unified(tEdits, pFrom, pTo, pContext)
end DiffCompare

/**

Patch a string with a previously computed diff.

Description:
Apply the diff to the source string, returning an updated string with the diff changes applied.

Example:

Parameters:
pSource (string): The original string

pDiff (string): The diff representing changes to apply to pSource

Returns:
An updated version of pSource with the changes from pDiff applied.

*/

function DiffPatch pSource, pDiff
   local tEdits, tContent
   parse_unified pDiff, tEdits, tContent
   return patch_string(pSource, tEdits, tContent)
end DiffPatch

/**

Produce a diff representing the changes between two text files

Description:
Compare two files, producing a string representing the changes between them in unified-diff format.

Example:

Parameters:
pFrom (string):  The original source file. The produced diff will describe how to transform the contents of this file.

pTo (string): The updated source file. This will be compared to pFrom to produce the output diff.

pContext (integer): The number of unchanged lines to include before / after each change in the diff.

Returns:
A string representing the changes between pFrom and pTo in unified diff format.

*/

function DiffCompareFiles pFrom, pTo, pContext
   // produce file header
   local tDiff
   put "--- " & pFrom & tab & udiff_file_timestamp(pFrom) & cr & \
         "+++ " & pTo & tab & udiff_file_timestamp(pTo) & cr into tDiff
   put DiffCompare(url ("binfile:" & pFrom), url ("binfile:" & pTo), pContext) after tDiff
   return tDiff
end DiffCompareFiles

private function udiff_file_timestamp pFile
   local tDetails
   put file_details(pFile) into tDetails
   if tDetails is empty then
      return empty
   end if
   
   local tDateItems, tInternetDate, tTimestamp
   // get the modified time (in seconds) and convert to dateitems
   put item 5 of tDetails into tDateItems
   convert tDateItems from seconds to dateitems
   
   // also convert to internet format (to get timezone delta)
   put item 5 of tDetails into tInternetDate
   convert tInternetDate from seconds to internet date
   
   put item 1 of tDateItems & "-" & \
         pad(item 2 of tDateItems,2,"0") & "-" & \
         pad(item 3 of tDateItems,2,"0") && \
         pad(item 4 of tDateItems,2,"0") & ":" & \
         pad(item 5 of tDateItems,2,"0") & ":" & \
         pad(item 6 of tDateItems,2,"0") into tTimestamp
   put space & word -1 of tInternetDate after tTimestamp
   
   return tTimestamp
end udiff_file_timestamp

private function pad pString, pLength, pPadChar
   local tPad, tPadLength
   put pLength - (the number of chars in pString) into tPadLength
   
   repeat tPadLength times
      put pPadChar after tPad
   end repeat
   
   return tPad & pString
end pad

private function file_details pFile
   repeat for each line tDetails in detailed_files(folder_of_path(pFile))
      if file_of_path(pFile) is item 1 of tDetails then
         return tDetails
      end if
   end repeat
end file_details

private function folder_of_path pPath
   set the itemDelimiter to "/"
   return item 1 to -2 of pPath
end folder_of_path

private function file_of_path pPath
   set the itemDelimiter to "/"
   return item -1 of pPath
end file_of_path

private function detailed_files pFolder
   local tDefault, tFiles
   
   if pFolder is empty then
      put the defaultFolder into pFolder
   end if
   
   put the defaultFolder into tDefault
   set the defaultFolder to pFolder
   
   if the result is not empty then
      return empty
   end if
   
   put the detailed files into tFiles
   
   set the defaultFolder to tDefault
   
   return tFiles
end detailed_files

--------------------------------------------------------------------------------
-- format conversion --

constant kContent = "content"
constant kAdditions = "additions"
constant kSource = "source"
constant kSourceLength = "sourceLength"
constant kContextLength = "contextLength"
constant kFirstSourceIndex = "firstSourceIndex"
constant kLastSourceIndex = "lastSourceIndex"
constant kDelta = "delta"

-- produce unified diff from edit list, containing pContextLength lines of context
private function convert_to_unified pEdits, pA, pB, pContextLength
   local tALength, tBLength
   put the number of elements in pA into tALength
   put the number of elements in pB into tBLength
   local tDiff
   local tHunk
   local tDelta = 0
   
   repeat for each line tEdit in pEdits
      local tA, tB, tOperation
      parse_edit tEdit, tA, tB, tOperation
      
      if tHunk is an array and not continue_hunk(tHunk, tA, tOperation) then
         end_hunk tHunk
         add_hunk tDiff, tHunk, tDelta
         add tHunk[kDelta] to tDelta
         put empty into tHunk
      end if
      
      if tHunk is not an array then
         begin_hunk tHunk, pA, tALength, pContextLength
      end if
      
      if tOperation is kDiffOperationAdd then
         add_addition tHunk, tA, pB[tB]
      else if tOperation is kDiffOperationDelete then
         add_deletion tHunk, tA, pA[tA]
      end if
   end repeat
   
   if tHunk is an array then
      end_hunk tHunk
      add_hunk tDiff, tHunk, tDelta
   end if
   
   return tDiff
end convert_to_unified

private command begin_hunk @pHunk, pSource, pSourceLength, pContextLength
   put empty into pHunk
   put pSource into pHunk[kSource]
   put pSourceLength into pHunk[kSourceLength]
   put pContextLength into pHunk[kContextLength]
   put 0 into pHunk[kDelta]
end begin_hunk

private command end_hunk @pHunk
   flush_additions pHunk
   
   if pHunk[kLastSourceIndex] is not empty then
      add_context_before pHunk, pHunk[kLastSourceIndex] + pHunk[kContextLength] + 1
   end if
end end_hunk

private function continue_hunk @pHunk, pIndex, pOperation
   if pOperation is kDiffOperationDelete then
      subtract 1 from pIndex
   end if
   
   return pIndex <= pHunk[kLastSourceIndex] + 2 * pHunk[kContextLength]
end continue_hunk

private command add_deletion @pHunk, pIndex, pContent
   add_context_before pHunk, pIndex
   put "-" & pContent after pHunk[kContent]
   if pHunk[kFirstSourceIndex] is empty then
      put pIndex into pHunk[kFirstSourceIndex]
   end if
   put pIndex into pHunk[kLastSourceIndex]
   subtract 1 from pHunk[kDelta]
   
   if not (pContent ends with return) then
      put return & "\ No newline at end of file" & return after pHunk[kContent]
   end if
end add_deletion

private command add_addition @pHunk, pIndex, pContent
   add_context_before pHunk, pIndex + 1
   // Defer adding additions to ensure they come after deletions
   put "+" & pContent after pHunk[kAdditions]
   add 1 to pHunk[kDelta]
   
   if not (pContent ends with return) then
      put return & "\ No newline at end of file" & return after pHunk[kAdditions]
   end if
end add_addition

private command flush_additions @pHunk
   if pHunk[kAdditions] is not empty then
      put pHunk[kAdditions] after pHunk[kContent]
      put empty into pHunk[kAdditions]
   end if
end flush_additions

private command add_context_before @pHunk, pIndex
   local tStart, tEnd
   if pHunk[kLastSourceIndex] is not empty then
      put pHunk[kLastSourceIndex] + 1 into tStart
   else
      put pIndex - pHunk[kContextLength] into tStart
   end if
   put pIndex - 1 into tEnd
   
   put max(1, tStart) into tStart
   put min(pHunk[kSourceLength], tEnd) into tEnd
   
   if pHunk[kLastSourceIndex] is not empty then
      put max(pHunk[kLastSourceIndex] + 1, tStart) into tStart
   end if
   
   if tStart > tEnd or tEnd < 1 then
      exit add_context_before
   end if
   
   flush_additions pHunk
   
   repeat with i = tStart to tEnd
      put space & pHunk[kSource][i] after pHunk[kContent]
   end repeat
   
   if not (pHunk[kContent] ends with return) then
      put return & "\ No newline at end of file" & return after pHunk[kContent]
   end if
   
   if pHunk[kFirstSourceIndex] is empty then
      put tStart into pHunk[kFirstSourceIndex]
   end if
   
   put tEnd into pHunk[kLastSourceIndex]
end add_context_before

private command add_hunk @pDiff, pHunk, pDelta
   local tAStart, tALength, tBStart, tBLength
   if pHunk[kFirstSourceIndex] is empty then
      put 0 into tAStart
      put 0 into tALength
   else
      put pHunk[kFirstSourceIndex] into tAStart
      put pHunk[kLastSourceIndex] + 1 - tAStart into tALength
   end if
   put tAStart + pDelta into tBStart
   put tALength + pHunk[kDelta] into tBLength
   
   local tARange, tBRange
   if tALength is 1 then
      put tAStart into tARange
   else
      put tAStart,tALength into tARange
   end if
   if tBLength is 1 then
      put tBStart into tBRange
   else
      put tBStart,tBLength into tBRange
   end if
   put "@@ -" & tARange & " +" & tBRange & " @@" & cr after pDiff
   put pHunk[kContent] after pDiff
end add_hunk

----------

-- convert unified diff to edit list
private command parse_unified pUniDiff, @rEdits, @rContent
   local tEdits
   local tContent
   local tLastEdit
   repeat for each line tLine in pUniDiff
      // skip file headers if present
      if tLine begins with "+++" or tLine begins with "---" then
         next repeat
      end if
      
      local tAStart,tALength, tBStart, tBLength
      if tLine begins with "@@" then
         if not matchText(tLine, "@@ -([[:digit:]]+)(?:,([[:digit:]]+))? [+]([[:digit:]]+)(?:,([[:digit:]]+))? @@", tAStart, tALength, tBStart, tBLength) then
            // couldn't parse hunk header
            return empty
         end if
         if tALength is empty then
            if tAStart is 0 then
               put 0 into tALength
            else
               put 1 into tALength
            end if
         end if
         if tBLength is empty then
            if tBStart is 0 then
               put 0 into tBLength
            else
               put 1 into tBLength
            end if
         end if
      else if tLine begins with space then
         // skip context line
         put empty into tLastEdit
         
         add 1 to tAStart
         add 1 to tBStart
         subtract 1 from tALength
         subtract 1 from tBLength
      else if tLine begins with "+" then
         // addition
         put tAStart - 1,kDiffOperationAdd,tBStart into tLastEdit
         put tLastEdit & return after tEdits
         put char 2 to -1 of tLine & return into tContent[tBStart]
         
         add 1 to tBStart
         subtract 1 from tBLength
      else if tLine begins with "-" then
         // deletion
         put tAStart, kDiffOperationDelete, tBStart - 1 into tLastEdit
         put tLastEdit & return after tEdits
         
         add 1 to tAStart
         subtract 1 from tALength
      else if tLine begins with "\" then
         // no newline
         local tA, tB, tLastOp
         parse_edit tLastEdit, tA, tB, tLastOp
         // remove newline from content of last operation (if addition)
         if tLastOp is kDiffOperationAdd then
            delete the last char of tContent[tB]
         end if
      end if
   end repeat
   
   put tContent into rContent
   put tEdits into rEdits
end parse_unified

--------------------------------------------------------------------------------
-- utility commands & functions --

-- swap contents of variables A and B
private command _swap @pA, @pB
   local tTemp
   put pA into tTemp
   put pB into pA
   put tTemp into pB
end _swap 

-- numeric ordering version of combine command
private function _combine @pArray, pDelimiter
   local tExtents
   local tCombined
   local tFirst = "true"
   
   put the extents of pArray into tExtents
   repeat with i = item 1 of tExtents to item 2 of tExtents
      if tFirst then
         put pArray[i] into tCombined
         put false into tFirst
      else
         put pDelimiter & pArray[i] after tCombined
      end if
   end repeat
   
   return tCombined
end _combine

-- split function which retains delimiter chars
private function _split pString, pDelim
   local tArray
   local tIndex = 0
   set the linedelimiter to pDelim
   repeat for each line tLine in pString
      add 1 to tIndex
      put tLine & pDelim into tArray[tIndex]
   end repeat
   
   // remove delim from last line if missing from string
   if not (pString ends with pDelim) then
      delete last char of tArray[tIndex]
   end if
   
   return tArray
end _split

--------------------------------------------------------------------------------

-- generate list of edits between two strings
private function diff_strings pA, pB
   put _split(pA, return) into pA
   put _split(pB, return) into pB
   
   return diff_arrays(pA, pB)
end diff_strings

-- generate list of edits between two arrays
private function diff_arrays pA, pB
   local tALength, tBLength
   put the number of elements in pA into tALength
   put the number of elements in pB into tBLength
   
   local tSwap
   put tALength > tBLength into tSwap
   
   if tSwap then
      _swap pA, pB
      _swap tALength, tBLength
   end if
   
   local tEdits
   put compare_arrays( pA, tALength, pB, tBLength ) into tEdits
   
   if tSwap then
      put reverse_edits(tEdits) into tEdits
   end if
   
   return tEdits
end diff_arrays

-- apply edit list to string
private function patch_string pString, pEdits, pContent
   put _split(pString, return) into pString
   local tOutput
   put patch_arrays(pString, pEdits, pContent) into tOutput
   
   return _combine(tOutput, empty)
   
   return tOutput
end patch_string

----------

-- Implementation of shortest-edit-script algorithm --

-- produce edits required to transform arrays pA -> pB
private function compare_arrays pA, pALength, pB, pBLength
   local tFP
   local tPath
   
   local tDelta
   put pBLength - pALength into tDelta
   
   repeat with i = -1 to tDelta + 1
      put -1 into tFP[i]
   end repeat
   
   local p = -1
   repeat while tFP[tDelta] < pBLength
      add 1 to p
      
      put -1 into tFP[-p - 1]
      put -1 into tFP[tDelta + p + 1]
      
      local tLeft, tRIght, tNext
      repeat with k = -p to tDelta -1
         compare_diagonal tFP, tPath, pA, pALength, pB, pBLength, k
      end repeat
      
      repeat with k = tDelta + p down to tDelta
         compare_diagonal tFP, tPath, pA, pALength, pB, pBLength, k
      end repeat
      
   end repeat
   
   return tPath[tDelta]
end compare_arrays

private command compare_diagonal @pFP, @pPath, @pA, pALength, @pB, pBLength, k
   local tLeft, tRight, tNext, tPrevPath, tOperation, tContent
   put pFP[k - 1] + 1 into tLeft
   put pFP[k + 1] into tRight
   if tLeft > tRight then
      put pPath[k - 1] into tPrevPath
      put kDiffOperationAdd into tOperation
      put tLeft into tNext
   else
      put pPath[k + 1] into tPrevPath
      put kDiffOperationDelete into tOperation
      put tRight into tNext
   end if
   
   if k > 0 or tNext > 0 then
      put tPrevPath & tNext-k,tOperation,tNext & cr into pPath[k]
   end if
   local tP
   --put snake(pA, pALength, pB, pBLength, k, tNext) into pFP[k]
   
   -- inline snake function
   set the caseSensitive to true
   set the formSensitive to true
   local tLimit
   put min(pALength + k, pBLength) into tLimit
   repeat while (tNext < tLimit) and pA[tNext - k + 1] is pB[tNext + 1]
      add 1 to tNext
   end repeat
   
   put tNext into pFP[k]
end compare_diagonal

private function snake @pA, pALength, @pB, pBLength, k, y
   set the caseSensitive to true
   set the formSensitive to true
   local tLimit
   put min(pALength + k, pBLength) into tLimit
   repeat while (y < tLimit) and pA[y - k + 1] is pB[y + 1]
      add 1 to y
   end repeat
   
   return y
end snake

-- apply edits to source array, producing output array
private function patch_arrays pA, pEdits, pContent
   local tOutput
   local tIndex, tAIndex
   put 1 into tIndex
   put 1 into tAIndex
   repeat for each line tEdit in pEdits
      local tA, tB, tOperation
      parse_edit tEdit, tA, tB, tOperation
      
      if tOperation is kDiffOperationAdd then
         skip_edits tOutput, tIndex, pA, tAIndex, tA
         put pContent[tB] into tOutput[tIndex]
         add 1 to tIndex
         put tA + 1 into tAIndex
      else if tOperation is kDiffOperationDelete then
         skip_edits tOutput, tIndex, pA, tAIndex, tA - 1
         put tA + 1 into tAIndex
      end if
      
   end repeat
   
   skip_edits tOutput, tIndex, pA, tAIndex, the number of elements in pA
   
   return tOutput
end patch_arrays

private command skip_edits @pOutput, @pFrontier, @pA, pStart, pEnd
   repeat with i = pStart to pEnd
      put pA[i] into pOutput[pFrontier]
      add 1 to pFrontier
   end repeat
end skip_edits

-- reverse direction of edits from A->B to B->A
private function reverse_edits pEdits
   local tEdits
   local tA, tB, tOperation, tContent
   
   repeat for each line tEdit in pEdits
      parse_edit tEdit, tA, tB, tOperation
      
      if tOperation is kDiffOperationAdd then put kDiffOperationDelete into tOperation
      else if tOperation is kDiffOperationDelete then put kDiffOperationAdd into tOperation
      
      put tB, tOperation, tA & return after tEdits
   end repeat
   
   return tEdits
end reverse_edits

private function format_edit pAIndex, pBIndex, pOperation, pContent
   return pAIndex,pOperation,pBIndex,pContent
end format_edit

private command parse_edit pEdit @rAIndex, @rBIndex, @rOperation
   put item 1 of pEdit into rAIndex
   put item 2 of pEdit into rOperation
   put item 3 of pEdit into rBIndex
end parse_edit

--------------------------------------------------------------------------------
