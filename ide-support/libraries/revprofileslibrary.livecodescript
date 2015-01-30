script "revprofileslibrary"

on revLoadLibrary
   insert the script of me into back
end revLoadLibrary

on revUnloadLibrary
   remove the script of me from back
end revUnloadLibrary

# OK-2008-04-14 : Added variable declarations when investigating a bug

on revSetCardProfile pWhich, pWhichStack
  lock screen
  if pWhichStack is not empty then set the defaultStack to pWhichStack
  else set the defaultStack to the topStack
  
  local tList
  repeat with i = 1 to the number of controls
    put the id of control i & cr after tList
  end repeat
  delete last char of tList
  repeat for each line l in tList
    set the cREVGeneral["profile"] of control id l to pWhich
  end repeat
  set the cREVGeneral["profile"] of this cd to pWhich
  unlock screen
end revSetCardProfile

on revSetStackProfile pWhich, pWhichStack
  lock screen
  if pWhichStack is not empty then set the defaultStack to pWhichStack
  else set the defaultStack to the topStack
  
  local tList
  repeat with x = 1 to the number of cards
    put empty into tList
    repeat with i = 1 to the number of controls on cd x
      put the id of control i of cd x & cr after tList
    end repeat
    delete last char of tList
    repeat for each line l in tList
      set the cREVGeneral["profile"] of control id l of cd x to pWhich
    end repeat
    set the cREVGeneral["profile"] of cd x to pWhich
  end repeat
  set the cREVGeneral["profile"] of this stack to pWhich
  unlock screen
end revSetStackProfile

on revSetStackFileProfile pWhich, pWhichSTack
   lock screen
   if pWhichStack is not empty then set the defaultStack to pWhichStack
   else set the defaultStack to the topStack
   
   local tMainStackName
   put the mainStack of this stack into tMainStackName
   
   local tStacksList
   put tMainStackName & cr & the subStacks of stack tMainStackName into tStacksList
   if charToNum(last char of tStacksList) is 10 then delete last char of tStacksList
   repeat for each line l in tStacksList
      set the cREVGeneral["profile"] of stack l to pWhich
      
      local tList
      repeat with x = 1 to the number of cds in stack l
         set the cREVGeneral["profile"] of cd x of stack l to pWhich
         put empty into tList
         repeat with i = 1 to the number of controls on cd x of stack l
            put the id of control i of cd x of stack l & cr after tList
         end repeat
         delete last char of tList
         repeat for each line y in tList
            -- answer "preparing to set profile" && i && the name of control i of cd x of stack l && "card " & x && cr & l & cr & pWhich
            set the cREVGeneral["profile"] of control id y of cd x of stack l to pWhich
         end repeat
      end repeat
   end repeat
   unlock screen
end revSetStackFileProfile

on revNewProfile pWhichProfile, pWhichObject
   global gREVTemplateMasterName
   
   local tObjectsList
   if pWhichObject is empty then
      put (the selObj) into tObjectsList
   else
      put pWhichObject into tObjectsList
   end if
   
   repeat for each line tObjectId in tObjectsList
      # OK-2008-04-14 : The selectedObject function will return invalid references
      # to objects if in edit group mode. These references are converted into "rugged ids".
      local tObject
      put revRuggedId(tObjectId) into tObject
      
      if pWhichProfile is among the lines of the cREVGeneral["profileList"] of tObject or pWhichProfile is the cREVGeneral["masterName"] of tObject or pWhichProfile is "Master" then
         --Profile already exists
         exit revNewProfile
      end if
   end repeat
   
   local tProfile
   put pWhichProfile into tProfile
   replace space with empty in tProfile
   repeat for each line l in tObjectsList
      if the cREVGeneral["profileList"] of l is empty then
         set the cREVGeneral["profileList"] of l to pWhichProfile
         if the cREVGeneral["masterName"] of l is empty then
            if gREVTemplateMasterName is not empty then set the cREVGeneral["masterName"] of l to gREVTemplateMasterName
            else set the cREVGeneral["masterName"] of l to "Master"
         end if
      else
         set the cREVGeneral["profileList"] of l to the cREVGeneral["profileList"] of l & cr & pWhichProfile
      end if
   end repeat
end revNewProfile

function revUpdateProfileListings pObjects
   global gREVTemplateMasterName
   if pObjects is empty then
      put the selobj into pObjects
   end if
   
   local tProfilesList
   put empty into tProfilesList
   repeat for each line tObjectId in pObjects
      # OK-2008-04-14 : The selectedObject function will return invalid references
      # to objects if in edit group mode. These references are converted into "rugged ids".
      local tObject
      put revRuggedId(tObjectId) into tObject
      
      put the cREVGeneral["profileList"] of tObject & cr after tProfilesList
      switch
         case the cREVGeneral["masterName"] of tObject is not empty
            put the cREVGeneral["masterName"] of tObject & space & cr after tProfilesList
         break
         case gREVTemplateMasterName is not empty
            put gREVTemplateMasterName & space & cr after tProfilesList
         break
         default
            put "Master" & space & cr after tProfilesList
         break       
      end switch
   end repeat
   delete last char of tProfilesList
   sort tProfilesList
   
   local tFinalList
   put empty into tFinalList
   repeat for each line l in tProfilesList
      if l is not among the lines of tFinalList then put l & cr after tFinalList
   end repeat
   delete last char of tFinalList
   repeat for each line tObjectId in (the selObj)
      # OK-2008-04-14 : The selectedObject function will return invalid references
      # to objects if in edit group mode. These references are converted into "rugged ids".
      put revRuggedId(tObjectId) into tObject
      
      repeat with i = 1 to the number of lines in tFinalList
         if word 1 of line i of tFinalList is not among the lines of the cREVGeneral["profileList"] of tObject and word 1 of line i of tFinalList is not the cREVGeneral["masterName"] of tObject and char 1 to 3 of line i of tFinalList is not "   " then put "   " before line i of tFinalList
      end repeat
   end repeat
   sort lines of tFinalList descending
   return tFinalList
end revUpdateProfileListings

function revReturnCurrentProfile pObjects
   global gREVTemplateMasterName
   
   local tVar
   put empty into tVar
   if pObjects is empty then 
      put (the selObj) into pObjects
   end if
   
   repeat for each line tObjectId in pObjects
      # OK-2008-04-14 : The selectedObject function will return invalid references
      # to objects if in edit group mode. These references are converted into "rugged ids".
      local tObject
      put revRuggedId(tObjectId) into tObject
      
      if tVar is empty then
         put the cREVGeneral["profile"] of tObject into tVar
         if tVar is empty then
            put "Master" into tVar
         end if
         if tVar is "Master" then
            if the cREVGeneral["masterName"] of tObject is not empty then
               put the cREVGeneral["masterName"] of tObject into tVar
            else if gREVTemplateMasterName is not empty then put gREVTemplateMasterName into tVar
         end if
      else
         local tProfile
         put the cREVGeneral["profile"] of tObject into tProfile
         if tProfile is empty
         then put "Master" into tProfile
         if tProfile is "Master" and the cREVGeneral["masterName"] of tObject is not empty
         then put the cREVGeneral["masterName"] of tObject into tProfile
         if tProfile is not tVar
         then return space
      end if
   end repeat
   if tVar is empty then return "Master" else return tVar
end revReturnCurrentProfile

function revCurrentProfileTypes pObjects
   local tMasterProfiles
   put 0 into tMasterProfiles
   
   local tStandardProfiles
   put 0 into tStandardProfiles
   
   local tProfile
   repeat for each line l in pObjects
      put the cREVGeneral["profile"] of l into tProfile
      if tProfile is "Master" or tProfile is empty
      then add 1 to tMasterProfiles
      else add 1 to tStandardProfiles    
   end repeat
   
   local tNumObjects
   put the number of lines in pObjects into tNumObjects
   switch
      case tMasterProfiles is tNumObjects
         return "Master"
      break
      case tStandardProfiles is tNumObjects
         return "Standard"
      break
      default
         return "Mixed"
   end switch
end revCurrentProfileTypes
