script "revrulersscriptlibrary"

on revLoadLibrary
   insert script of me into front
end revLoadLibrary

on revUnloadLibrary
   remove script of me from front
end revUnloadLibrary

local lPaused

on moveStack
  if lPaused then pass moveStack
  if char 1 to 9 of revTargetStack() is "revRulers" then
    pass moveStack
  end if
  if the mode of stack revTargetStack() is not 1 or the cantModify of stack revTargetStack() is true or the tool is "browse tool" then pass moveStack
  get the topLeft of stack revTargetStack() into it
  set the topLeft of stack "revRulersH" to item 1 of it - the cREVHSubtract of stack "revRulersH",item 2 of it - the cREVVSubtract of stack "revRulersH"
  if the platform is "MacOS" then put 2 into tAdd else put 0 into tAdd
  set the topLeft of stack "revRulersV" to item 1 of it - the cREVHSubtract of stack "revRulersV",item 2 of it + the cREVVSubtract of stack "revRulersV" + tAdd
  pass moveStack
end moveStack

on resizeStack
  revPartCalculateRulers
  pass resizeStack
end resizeStack

on iconifyStack
  put true into lPaused
  if the visible of stack "revRulersH" then hide stack "revRulersH"
  if the visible of stack "revRulersV" then hide stack "revRulersV"
  pass iconifyStack
end iconifyStack

on uniconifyStack
  if not the visible of stack "revRulersH" then
    show stack "revRulersH"
    show stack "revRulersV"
  else
    palette "revRulersV"
    palette "revRulersH"
  end if
  revPartCalculateRulers
end uniconifyStack

on openStack
  if not the visible of stack "revRulersH" then
    show stack "revRulersH"
    show stack "revRulersV"
  else
    palette "revRulersV"
    palette "revRulersH"
  end if
  revPartCalculateRulers
  pass openStack
end openStack

on newStack
  if not the visible of stack "revRulersH" then
    show stack "revRulersH"
    show stack "revRulersV"
  else
    palette "revRulersV"
    palette "revRulersH"
  end if
  revPartCalculateRulers
end newStack

on resumeStack
  lock messages
  if the mode of stack revTargetStack() is not 1 or the cantModify of stack revTargetStack() is true or the tool is "browse tool" then
    if char 1 to 9 of revTargetStack() is "revRulers" then
      unlock messages
      pass resumeStack
    end if
    put true into lPaused
    if the tool is "browse tool" then
      if the visible of stack "revRulersH" then hide stack "revRulersH"
      if the visible of stack "revRulersV" then hide stack "revRulersV"
    end if
    unlock messages
    pass resumeStack
  else
    put false into lPaused
    if not the visible of stack "revRulersH" then
      show stack "revRulersH"
      show stack "revRulersV"
    else
      palette "revRulersV"
      palette "revRulersH"
    end if
    revPartCalculateRulers
    unlock messages
    pass resumeStack
  end if
  unlock messages
end resumeStack

on closeStack
  if lPaused then pass closeStack
  send "revCheckClose" to me in 100 milliseconds
  pass closeStack
end closeStack

on revCheckClose
  if the mode of the topStack is not 1 then
    if the visible of stack "revRulersH" then hide stack "revRulersH"
    if the visible of stack "revRulersV" then hide stack "revRulersV"
  end if
end revCheckClose

on mouseMove x,y
  local tOldDefaultStack
  if lPaused then pass mouseMove
  put the defaultStack into tOldDefaultStack
  set the defaultStack to the cREVCurrentStack of stack "revRulersH"
  -- if within(this cd of this stack, the mouseloc) then - metacard bug
  if the platform is "MacOS" then put 5 into tVSubtract else put 4 into tVSubtract
  if the platform is "MacOS" then put 23 into tHSubtract else put 24 into tHSubtract
  if x > 0 and x <= the right of this cd and y > 0 and y <= the bottom of this cd then
    set the right of grc "mouse marker" of stack "revRulersH" to x+tHSubtract
    set the top of grc "mouse marker" of stack "revRulersV" to y-tVSubtract
  else
    set the right of grc "mouse marker" of stack "revRulersH" to -100
    set the top of grc "mouse marker" of stack "revRulersV" to -100
  end if
  if (the selObj) is empty then
    set the right of grc "left marker" of stack "revRulersH" to -100
    set the right of grc "right marker" of stack "revRulersH" to - 100
    set the top of grc "top marker" of stack "revRulersV" to -100
    set the top of grc "bottom marker" of stack "revRulersV" to - 100
  else
    set the right of grc "left marker" of stack "revRulersH" to the left of the selObj + tHSubtract
    set the right of grc "right marker" of stack "revRulersH" to the right of the selObj + tHSubtract
    set the top of grc "top marker" of stack "revRulersV" to the top of the selObj -  tVSubtract
    set the top of grc "bottom marker" of stack "revRulersV" to the bottom of the selObj - tVSubtract
  end if
  set the defaultStack to tOldDefaultStack
  pass mouseMove
end mouseMove

on mouseDown
  if lPaused and word 1 of the target is "card" then send "resumeStack" to btn "revRulersScript" of stack "revLibrary"
  pass mouseDown
end mouseDown

on selectedObjectChanged
  send mouseMove && the mouseLoc to me
  pass selectedObjectChanged
end selectedObjectChanged

on arrowKey
  if "revNudgeRulers" is not in the pendingMessages then send "revNudgeRulers" to me in 50 milliseconds
  pass arrowKey
end arrowKey

on revNudgeRulers
  send mouseMove && the mouseLoc to me
end revNudgeRulers
