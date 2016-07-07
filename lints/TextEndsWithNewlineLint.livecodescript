script "TextEndsWithNewlineLint"
on Lint
   local tContents
   put LintGetTextContents() into tContents
   if the last char of tContents is not return then
      LintProblem "File does not end with newline"
   end if
end Lint
