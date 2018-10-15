script "TestDiffLib"
on TestSetup
   local tLibFilename
   put get_library_folder() & "/diff.livecodescript" into tLibFilename
   
   start using stack tLibFilename
end TestSetup

on TestContextLines
   local tTestFolder
   put get_test_folder() & "/test_context" into tTestFolder
   
   __TestDiffFilesInFolder tTestFolder, "a", "b", "Lines of context used"
end TestContextLines

on TestLineEndings
   local tTestFolder
   put get_test_folder() & "/test_line_endings" into tTestFolder
   
   __TestDiffFilesInFolder tTestFolder, "lf", "cr", "Line endings"
   __TestDiffFilesInFolder tTestFolder, "cr", "crlf", "Line endings"
   __TestDiffFilesInFolder tTestFolder, "crlf", "lf", "Line endings"
end TestLineEndings

on TestTerminatingNewline
   local tTestFolder
   put get_test_folder() & "/test_terminating_newline" into tTestFOlder
   
   __TestDiffFilesInFolder tTestFolder, "a", "a-no-nl", "Final newline removed"
   __TestDiffFilesInFolder tTestFolder, "a", "b-no-nl", "Content change with final newline removed"
   __TestDiffFilesInFolder tTestFolder, "a-no-nl", "a-no-nl-del", "Deletion of final line without newline"
   __TestDiffFilesInFolder tTestFolder, "a-no-nl", "a-no-nl-del-no-nl", "Deletion of final line without newline, and preceding newline"
   __TestDiffFilesInFolder tTestFolder, "a-no-nl", "b-no-nl", "Change of final line with no newline in either file"
   
   __TestDiffFilesInFolder tTestFolder, "a-context", "b-context", "Inclusion of context without final newline"
end TestTerminatingNewline

// Test function assumes the following filename scheme:
// file a - <pFileA>.txt
// file b - <pFileB>.txt
// diff a -> b - <pFileA>_<pFileB>.diff
// diff b -> a - <pFileB>_<pFileA>.diff
command __TestDiffFilesInFolder pFolder, pFileA, pFileB, pDescription
   local tDefaultFolder
   
   // catch missing / misnamed folder
   TestAssert "folder exists - " & pFolder, there is a folder pFolder
   
   put the defaultFolder into tDefaultFolder
   set the defaultFolder to pFolder
   
   __TestDiffFiles (pFileA & ".txt"), (pFileB & ".txt"), (pFileA & "_" & pFileB & ".diff"), (pDescription & " - " & pFileA & " -> " & pFileB)
   __TestDiffFiles (pFileB & ".txt"), (pFileA & ".txt"), (pFileB & "_" & pFileA & ".diff"), (pDescription & " - " & pFileB & " -> " & pFileA)
   
   __TestPatchFiles (pFileA & ".txt"), (pFileB & ".txt"), (pFileA & "_" & pFileB & ".diff"), (pDescription & " - PATCH " & pFileA & " -> " & pFileB)
   __TestPatchFiles (pFileB & ".txt"), (pFileA & ".txt"), (pFileB & "_" & pFileA & ".diff"), (pDescription & " - PATCH " & pFileB & " -> " & pFileA)
   
   set the defaultFolder to tDefaultFolder
end __TestDiffFilesInFolder

command __TestDiffFiles pFileA, pFileB, pReferenceFile, pDescription
   local tDiff, tReferenceDiff
   
   // catch missing files
   TestAssert "file exists - " & pFileA, there is a file pFileA
   TestAssert "file exists - " & pFileB, there is a file pFileB
   TestAssert "file exists - " & pReferenceFile, there is a file pReferenceFile
   
   put DiffCompareFiles(pFileA, pFileB) into tDiff
   put url ("binfile:" & pReferenceFile) into tReferenceDiff
   
   // strip diff file headers
   if tDiff begins with "---" then
      delete line 1 to 2 of tDiff
   end if
   
   if tReferenceDiff begins with "---" then
      delete line 1 to 2 of tReferenceDiff
   end if
   
   TestAssert pDescription, tDiff is tReferenceDiff
end __TestDiffFiles

command __TestPatchFiles pFileA, pFileB, pDiffFile, pDescription
   local tA, tB, tDiff
   
   // catch missing files
   TestAssert "file exists - " & pFileA, there is a file pFileA
   TestAssert "file exists - " & pFileB, there is a file pFileB
   TestAssert "file exists - " & pDiffFile, there is a file pDiffFile
   
   put url ("binfile:" & pFileA) into tA
   put url ("binfile:" & pFileB) into tB
   put url ("binfile:" & pDiffFile) into tDiff
   
   put DiffPatch(tA, tDiff) into tA
   
   TestAssert pDescription, tA is tB
end __TestPatchFiles

private function get_library_folder
   return TestGetEngineRepositoryPath() & "/extensions/script-libraries/diff"
end get_library_folder

function get_test_folder
   return get_library_folder() & "/test"
end get_test_folder
