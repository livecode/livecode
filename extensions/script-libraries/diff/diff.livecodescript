script "com.livecode.script-library.diff"
/*
Copyright (C) 2016 LiveCode Ltd.

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

on revLoadLibrary
   if the target is not me then
      pass revLoadLibrary
   end if
   
   insert the script of me into back
   
   if the environment contains "development" then
      set the _ideoverride of me to true
      revSBAddAvailableLibrary the short name of me, "Diff"
   end if
end revLoadLibrary

on revUnloadLibrary
   if the target is not me then
      pass revUnloadLibrary
   end if
   
   remove the script of me from back
end revUnloadLibrary

/* Text comparison library, handling differences in unified diff format */

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
   
   local tEdits
   put diff_strings(pFrom, pTo) into tEdits
   split pFrom by cr
   return convert_to_unified(tEdits, pFrom, the number of elements in pFrom, pContext)
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
   local tEdits
   put parse_unified(pDiff) into tEdits
   return patch_string(pSource, tEdits)
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

function DiffCompareFiles pFrom, pTo
   // produce file header
   local tDiff
   put "--- " & pFrom & tab & udiff_file_timestamp(pFrom) & cr & \
         "+++ " & pTo & tab & udiff_file_timestamp(pTo) & cr into tDiff
   put udiff_strings(url ("file:" & pFrom), url ("file:" & pTo)) after tDiff
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
constant kSource = "source"
constant kSourceLength = "sourceLength"
constant kContextLength = "contextLength"
constant kFirstSourceIndex = "firstSourceIndex"
constant kLastSourceIndex = "lastSourceIndex"
constant kDelta = "delta"

-- produce unified diff from edit list, containing pContextLength lines of context
private function convert_to_unified pEdits, pSource, pSourceLength, pContextLength
   local tDiff
   local tHunk
   local tDelta = 0
   
   repeat for each line tEdit in pEdits
      local tA, tB, tOperation, tContent
      parse_edit tEdit, tA, tB, tOperation, tContent
      
      if tHunk is an array and not continue_hunk(tHunk, tA, tOperation) then
         end_hunk tHunk
         add_hunk tDiff, tHunk, tDelta
         add tHunk[kDelta] to tDelta
         put empty into tHunk
      end if
      
      if tHunk is not an array then
         begin_hunk tHunk, pSource, pSourceLength, pContextLength
      end if
      
      if tOperation is kDiffOperationAdd then
         add_addition tHunk, tA, tContent
      else if tOperation is kDiffOperationDelete then
         add_deletion tHunk, tA, tContent
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
   put "-" & pContent & cr after pHunk[kContent]
   if pHunk[kFirstSourceIndex] is empty then
      put pIndex into pHunk[kFirstSourceIndex]
   end if
   put pIndex into pHunk[kLastSourceIndex]
   subtract 1 from pHunk[kDelta]
end add_deletion

private command add_addition @pHunk, pIndex, pContent
   add_context_before pHunk, pIndex + 1
   put "+" & pContent & cr after pHunk[kContent]
   if pHunk[kFirstSourceIndex] is empty then
      put pIndex into pHunk[kFirstSourceIndex]
   end if
   put pIndex into pHunk[kLastSourceIndex]
   add 1 to pHunk[kDelta]
end add_addition

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
   
   repeat with i = tStart to tEnd
      put space & pHunk[kSource][i] & cr after pHunk[kContent]
   end repeat
   
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
private function parse_unified pUniDiff
   local tEdits
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
         add 1 to tAStart
         add 1 to tBStart
         subtract 1 from tALength
         subtract 1 from tBLength
      else if tLine begins with "+" then
         // addition
         put format_edit(tAStart - 1, tBStart, kDiffOperationAdd, char 2 to -1 of tLine) & return after tEdits
         add 1 to tBStart
         subtract 1 from tBLength
      else if tLine begins with "-" then
         // deletion
         put format_edit(tAStart, tBStart - 1, kDiffOperationDelete, char 2 to -1 of tLine) & return after tEdits
         add 1 to tAStart
         subtract 1 from tALength
      end if
   end repeat
   
   return tEdits
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

--------------------------------------------------------------------------------

-- generate list of edits between two strings
private function diff_strings pA, pB
   split pA by cr
   split pB by cr
   
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
end diff_strings

-- apply edit list to string
private function patch_string pString, pEdits
   split pString by cr
   local tOutput
   put patch_arrays(pString, the number of elements in pString, pEdits) into tOutput
   
   return _combine(tOutput, cr)
   
   return tOutput
end patch_string

----------

-- Implementation of shortest-edit-script algorithm --

-- produce edits required to transform arrays pA -> pB
private function compare_arrays pA, pALength, pB, pBLength
   local tFP
   local tPath
   repeat with i = -(pALength + 1) to (pBLength + 1)
      put -1 into tFP[i]
   end repeat
   
   local tDelta
   put pBLength - pALength into tDelta
   
   local p = -1
   repeat while tFP[tDelta] < pBLength
      add 1 to p
      
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
      put pB[tNext] into tContent
   else
      put pPath[k + 1] into tPrevPath
      put kDiffOperationDelete into tOperation
      put tRight into tNext
      put pA[tNext - k] into tContent
   end if
   
   if k > 0 or tNext > 0 then
      put tPrevPath & format_edit(tNext - k, tNext, tOperation, tContent) & return into pPath[k]
   end if
   local tP
   put snake(pA, pALength, pB, pBLength, k, tNext) into pFP[k]
end compare_diagonal

private function snake @pA, pALength, @pB, pBLength, k, y
   local x
   put y - k into x
   repeat while (x < pALength) and (y < pBLength) and pA[x + 1] is pB[y + 1]
      add 1 to x
      add 1 to y
   end repeat
   
   return y
end snake

-- apply edits to source array, producing output array
private function patch_arrays @pA, pALength, pEdits
   local tOutput
   local tIndex, tAIndex
   put 1 into tIndex
   put 1 into tAIndex
   repeat for each line tEdit in pEdits
      local tA, tB, tOperation, tContent
      parse_edit tEdit, tA, tB, tOperation, tContent
      
      if tOperation is kDiffOperationAdd then
         skip_edits tOutput, tIndex, pA, tAIndex, tA
         put tContent into tOutput[tIndex]
         add 1 to tIndex
         put tA + 1 into tAIndex
      else if tOperation is kDiffOperationDelete then
         skip_edits tOutput, tIndex, pA, tAIndex, tA - 1
         put tA + 1 into tAIndex
      end if
      
   end repeat
   
   skip_edits tOutput, tIndex, pA, tAIndex, pALength
   
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
      parse_edit tEdit, tA, tB, tOperation, tContent
      
      if tOperation is kDiffOperationAdd then put kDiffOperationDelete into tOperation
      else if tOperation is kDiffOperationDelete then put kDiffOperationAdd into tOperation
      
      put format_edit(tB, tA, tOperation, tContent) & return after tEdits
   end repeat
   
   return tEdits
end reverse_edits

private function format_edit pAIndex, pBIndex, pOperation, pContent
   return pAIndex,pOperation,pBIndex,pContent
end format_edit

private command parse_edit pEdit @rAIndex, @rBIndex, @rOperation, @rContent
   put item 1 of pEdit into rAIndex
   put item 2 of pEdit into rOperation
   put item 3 of pEdit into rBIndex
   put item 4 to -1 of pEdit into rContent
end parse_edit

--------------------------------------------------------------------------------
