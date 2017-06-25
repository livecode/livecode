script "InstallerInstallerUIUpdateCheckUpdateCheckingLoaderGroupBehavior"
local sLoading

on startLoading
  put true into sLoading
  doLoading
end startLoading

on doLoading
  if sLoading then
    local tPreviousColor, tCurrentColor
    put the textColor of graphic "line,0" of me into tPreviousColor
    repeat with x = 1 to the cNumber of me
      get "line",(x mod the cNumber of me)
      put the textColor of graphic it of me into tCurrentColor
      set the textColor of graphic it of me to tPreviousColor
      put tCurrentColor into tPreviousColor
    end repeat
    send "doLoading" to me in (the cDelay of me) milliseconds
  end if
end doLoading
 
on stopLoading
  put false into sLoading
end stopLoading

