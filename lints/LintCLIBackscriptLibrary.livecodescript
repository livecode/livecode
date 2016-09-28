script "LintCLIBackscriptLibrary"
----------------------------------------------------------------
-- Library management
----------------------------------------------------------------

on revLoadLibrary
   if the target is me then
      insert the script of me into back
   end if
end revLoadLibrary

on revUnloadLibrary
   if the target is me then
      remove the script of me from back
   end if
end revUnloadLibrary

----------------------------------------------------------------
-- Library management
----------------------------------------------------------------

on errorDialog pError
   dispatch "errorDialog" to stack "LintCLI" with pError
end errorDialog