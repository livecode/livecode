script "revtablelibrary"

on revLoadLibrary
   insert the script of me into front
end revLoadLibrary

on revUnloadLibrary
   remove the script of me from front
end revUnloadLibrary

-- Table library

/*
 * Version history :
 * -----------------
 * 
 * Date      Who  Version  Comments
 * --------  ---  -------  ------------------------------------------------------------------
 * 20040203  js   2.2a1    improved table field interaction with databases
 * 20040715  js   2.3b1    fixed bug #67 ; fixed bug #1487
 * 
 */


local lObject, lxmovevalue

--on preOpenCard
  repeat with i = 1 to the number of fields
    put the long id of field i into pObject
    if (the cREVTable["cellformat"] of pObject is true) and (the cREVGeneral["table"] of pObject is true) then
      revGetTableSettings pObject
      revDisplayFormattedData pObject
    end if
  end repeat
  pass preOpenCard
end preOpenCard

on focusOut
  if lObject is empty or there is not a lObject then 
    revDeleteCellFields
    pass focusOut
  end if
  revDeleteUnwantedFields
  pass focusOut    
end focusOut

on focusIn
  if lObject is empty or there is not a lObject then 
    revDeleteCellFields
    pass focusIn
  end if
  revDeleteUnwantedFields
  pass focusIn    
end focusIn

--js::20040715::start
--    fixed bug #1487
--js::20040715::new_code
on openField
  --  try -- fix for 4555, don't access this function in standalone
  --    set the cCurrentObject of the target to revPaletteObjects()
  --  end try
  if word 1 of the target is "field" and the cREVGeneral["table"] of the target is "true" \
      and the cREVTable["celledit"] of the target is "true" and the lockText of the target is false
  then
    set the lockText of the target to true
    send "revDoFocusIn" to the target in 5 milliseconds
  else pass openField
end openField

on revDoFocusIn
  focus on the target
end revDoFocusIn
--js::20040715::end

on exitField
   local tTarget
   put the long id of the target into tTarget
   
   # OK-2009-01-29 : If we're cell editing then obtain a reference to the original table field before deleting the cell
   local tOwner
   if there is a tTarget and the cREVTable["acell"] of tTarget then
      put the cREVTable["owner"] of tTarget into tOwner
   end if
   
   if lObject is empty or there is not a lObject then 
      revDeleteCellFields
      pass exitField
   end if
   revDeleteUnwantedFields
   
   # OK-2009-01-29 : If tOwner is not empty then it is assumed that were we editing a cell and have now finished.
   # Closefield should be sent directly to the table to simulate the standard field behavior. It should be fine to pass
   # the message after this, as the target would not have been the table field anyway, so it shouldnt get it twice.
   if tOwner is not empty and there is a tOwner and not within(tOwner, the mouseLoc) then
      try
         send "exitField" to tOwner
      end try
   end if
   
   pass exitField    
end exitField

on scrollBarDrag pValue
  if (lObject is empty) or (there is not a lObject) or (the long id of the target is lObject) or (the cREVTable["acell"] of the long id of the target is true) then 
    if (the long id of the target is lObject) then
      revDeleteCellFields
    end if
    pass scrollBarDrag
  end if
  if (word 1 of the tool is "browse") and \ --(within(lObject, the clickLoc)) and \
      (the cREVTable["celledit"] of lObject is true) and \
      (within(lObject, the clickLoc))
  then
    lock screen
    revWriteCellField lObject
    revDeleteCellFields
    --revSetScrollInfo lObject
    unlock screen
  end if
  pass scrollBarDrag
end scrollBarDrag

on returnInField
  if (word 1 of the tool is "browse") and \ --(within(lObject, the clickLoc)) and \
      (the cREVTable["acell"] of the long id of the target is true)
  then
    revMoveCellDown
  end if
  pass returnInField
end returnInField

on tabKey
  if (word 1 of the tool is "browse") and \ --(within(lObject, the clickLoc)) and \
      (the cREVTable["acell"] of the long id of the target is true)
  then
    if shiftKey() is down then
      revMoveCellLeft
    else
      revMoveCellRight
    end if
  else
    pass tabKey
  end if
end tabKey

on revDeleteUnwantedFields
  if (word 1 of the tool is not "pointer") and \
      (the cREVTable["celledit"] of lObject is true) and \ --(within(lObject, the clickLoc)) and \
      not (within(lObject, the clickLoc))
  then
    lock screen
    revWriteCellField lObject
    revDeleteCellFields
    unlock screen
  else
    lock screen
    revDeleteCellFields
    unlock screen
  end if
end revDeleteUnwantedFields

on revDeleteCellFields
   if the cREVTable["acell"] of this card is not true then exit revDeleteCellFields  -- performance
   set the lockMessages to true  -- performance
   
   local tDefault
   put the defaultStack into tDefault
   
   local tDeleteList
   put empty into tDeleteList
   if lObject is not empty then
      set the defaultStack to revTargetStack(lObject)
   end if
   repeat with i = 1 to the number of fields
      put the long id of field i into tObject
      if there is not a tObject then next repeat
      if the cREVTable["acell"] of tObject is true then
         put tObject & cr after tDeleteList
      end if
   end repeat
   delete last char of tDeleteList
   repeat for each line i in tDeleteList
      delete i
   end repeat
   set the cREVTable["acell"] of this card to false
   set the defaultStack to tDefault
   set the lockMessages to false
end revDeleteCellFields


on revMoveCellRight
  if the cREVTable["maxColumnCount"] of lObject is a number \
      and the cREVTable["currentxcell"] of lObject \
      >= the cREVTable["maxColumnCount"] of lObject then
    lock screen
    repeat the cREVTable["currentxcell"] of lObject - 1
      revMoveCellLeft
    end repeat
    revMoveCellDown
    unlock screen
    exit revMoveCellRight
  end if
  lock screen
  revWriteCellField lObject
  revDeleteCellFields
  revMoveNextColumn lObject
  revCalculateCellRect lObject
  revResizeTemplates lObject
  revCreateCellField lObject
  if exists(lObject) then
    put "right" into tDirection
    put the cREVTable["currentxcell"] of lObject into txcell
    put the cREVTable["currentycell"] of lObject into tycell 
    send "revMoveCellDirection lObject,txcell,tycell,tDirection" to lObject
  end if
  unlock screen
end revMoveCellRight

on revMoveCellDown
  lock screen
  revWriteCellField lObject
  revDeleteCellFields
  revMoveNextRow lObject
  revCalculateCellRect lObject
  revResizeTemplates lObject
  revCreateCellField lObject
  if exists(lObject) then
    put "down" into tDirection
    put the cREVTable["currentxcell"] of lObject into txcell
    put the cREVTable["currentycell"] of lObject into tycell 
    send "revMoveCellDirection lObject,txcell,tycell,tDirection" to lObject
  end if
  unlock screen
end revMoveCellDown

on revMoveCellUp
  lock screen
  revWriteCellField lObject
  revDeleteCellFields
  revMovePrevRow lObject
  revCalculateCellRect lObject
  revResizeTemplates lObject
  revCreateCellField lObject
  if exists(lObject) then
    put "up" into tDirection
    put the cREVTable["currentxcell"] of lObject into txcell
    put the cREVTable["currentycell"] of lObject into tycell 
    send "revMoveCellDirection lObject,txcell,tycell,tDirection" to lObject
  end if
  unlock screen
end revMoveCellUp

on revMoveCellLeft
  lock screen
  revWriteCellField lObject
  revDeleteCellFields
  revMovePrevColumn lObject
  revCalculateCellRect lObject
  revResizeTemplates lObject
  revCreateCellField lObject
  if exists(lObject) then
    put "left" into tDirection
    put the cREVTable["currentxcell"] of lObject into txcell
    put the cREVTable["currentycell"] of lObject into tycell 
    send "revMoveCellDirection lObject,txcell,tycell,tDirection" to lObject
  end if
  unlock screen
end revMoveCellLeft

on arrowKey pWhich
  if (word 1 of the tool is "browse") and \ --(within(lObject, the clickLoc)) and \
      (the cREVTable["acell"] of the long id of the target is true)
  then
    switch pWhich
    case "right"
      revMoveCellRight
      break
    case "left"
      revMoveCellLeft
      break
    case "up"
      revMoveCellUp
      break
    case "down"
      revMoveCellDown
      break
    end switch
  else
    pass arrowKey
  end if
end arrowKey


--mf::20050706::start
--    fixed bug #1487
on closeField
   put the long id of the target into tTarget
   
   # OK-2009-01-29 : If we're cell editing then obtain a reference to the original table field before deleting the cell
   local tOwner
   if there is a tTarget and the cREVTable["acell"] of tTarget then
      put the cREVTable["owner"] of tTarget into tOwner
   end if
   
   if (word 1 of the tool is "browse") and (the cREVTable["acell"] of tTarget) then
      lock screen
      revWriteCellField lObject
      revDeleteCellFields
      unlock screen
   end if
   if there is a tTarget then
      if the cREVGeneral["table"] of tTarget then
         revUpdateTable tTarget
      end if
   end if
   
   # OK-2009-01-29 : If tOwner is not empty then it is assumed that were we editing a cell and have now finished.
   # Closefield should be sent directly to the table to simulate the standard field behavior. It should be fine to pass
   # the message after this, as the target would not have been the table field anyway, so it shouldnt get it twice.
   if tOwner is not empty and there is a tOwner and not within(tOwner, the mouseLoc) then
      try
         send "closeField" to tOwner
      end try
   end if
   
   pass closeField
end closeField

on revUpdateTable pTable
  put the htmlText of pTable into tHtml
  put tHtml into tHtml2
  replace "<p>" with empty in tHtml2
  replace "</p>" with empty in tHtml2
  put the text of pTable into tText
  if tText is tHtml2 then set the cREVTable["currentview"] of pTable to tText
  else set the cREVTable["currentview"] of pTable to tHtml
end revUpdateTable
--mf::20050706::end

--js::20040715::start
--    fixed bug #67
--js::20040203::start
--    improved table field interaction with databases
--js::20040715::old_code
--on revWriteCellField pObject
--  --revSetScrollInfo pObject
--  put the cREVTable["currentxcell"] of pObject into txcell
--  put the cREVTable["currentycell"] of pObject into tycell 
--  put the cREVTable["currentview"] of pObject into tNewField
--  put revGetCellName(pObject) into tFieldName 
--  
--  if there is not a field tFieldName then exit revWriteCellField
--  set the itemDelimiter to tab
--  put the text of field tFieldName into tNewText
--  put item txcell of line tycell of tNewField into tcurrentcell
--  
--  if not (tNewText = tcurrentcell) then
--    put 0 into tMaxColumns
--    put the number of lines of the cREVTable["currentview"] of pObject into tMaxRows
--    repeat with i = 1 to tMaxRows
--      if the number of items in line i of the cREVTable["currentview"] of pObject > tMaxColumns
--      then
--        put the number of items in line i of the cREVTable["currentview"] of pObject into tMaxColumns
--      end if
--    end repeat
--    if txcell > tMaxColumns then
--      put txcell into tMaxColumns
--    end if
--    if tycell > tMaxRows then
--      if the last char of tNewField is not cr then
--        put cr after tNewfield  
--      end if
--      repeat with i = 1 to (tycell - tMaxRows)
--        repeat with j = 1 to (tMaxColumns)
--          put tab after tNewfield
--        end repeat   
--        put cr after tNewfield  
--      end repeat
--      if the last char of tNewField is cr then
--        delete last char of tNewField
--      end if
--    else
--      put the number of items in line tycell of tNewField into tnumbercol
--      --answer "tnumcol" && tnumbercol
--      --answer "tmaxcol" && tMaxColumns
--      put line tycell of tNewField into tNewline
--      if the last char of tNewline is cr then
--        delete last char of tNewline
--      end if
--      repeat with i = (tnumbercol + 1) to tMaxColumns
--        put tab after tNewline
--      end repeat
--      put tNewline into line tycell of tNewField
--    end if   
--     
--    put tNewText into item txcell of line tycell tNewField
--    set the cREVTable["currentview"] of pObject to tNewField 
--    put empty into tColumnValues
--    repeat with i = 1 to tMaxColumns
--      put item i of line tycell of tNewField & cr after tColumnValues
--    end repeat
--    delete last char of tColumnValues
--    send "revUpdateDatabase pObject,txcell,tNewText,tColumnValues" to pObject
--  end if
--  revUpdateScrollInfo pObject
--  revDisplayFormattedData pObject
--  if exists(pObject) then
--    send "revUpdateCellValue pObject,txcell,tycell,tcurrentcell,tNewText" to pObject
--  end if
--end revWriteCellField
--js::20040203::new_code
--on revWriteCellField pObject
--  --revSetScrollInfo pObject
--  put the cREVTable["currentxcell"] of pObject into txcell
--  put the cREVTable["currentycell"] of pObject into tycell 
--  put the cREVTable["currentview"] of pObject into tNewField
--  put revGetCellName(pObject) into tFieldName 
--  
--  if there is not a field tFieldName then exit revWriteCellField
--  set the itemDelimiter to tab
--  put the text of field tFieldName into tNewText
--  put item txcell of line tycell of tNewField into tcurrentcell
--  
--  if not (tNewText = tcurrentcell) then
--    put 0 into tMaxColumns
--    put the number of lines of the cREVTable["currentview"] of pObject into tMaxRows
--    repeat with i = 1 to tMaxRows
--      if the number of items in line i of the cREVTable["currentview"] of pObject > tMaxColumns
--      then
--        put the number of items in line i of the cREVTable["currentview"] of pObject into tMaxColumns
--      end if
--    end repeat
--    if txcell > tMaxColumns then
--      put txcell into tMaxColumns
--    end if
--    if tycell > tMaxRows then
--      if the last char of tNewField is not cr then
--        put cr after tNewfield  
--      end if
--      repeat with i = 1 to (tycell - tMaxRows)
--        repeat with j = 1 to (tMaxColumns)
--          put tab after tNewfield
--        end repeat   
--        put cr after tNewfield  
--      end repeat
--      if the last char of tNewField is cr then
--        delete last char of tNewField
--      end if
--    else
--      put the number of items in line tycell of tNewField into tnumbercol
--      --answer "tnumcol" && tnumbercol
--      --answer "tmaxcol" && tMaxColumns
--      put line tycell of tNewField into tNewline
--      if the last char of tNewline is cr then
--        delete last char of tNewline
--      end if
--      repeat with i = (tnumbercol + 1) to tMaxColumns
--        put tab after tNewline
--      end repeat
--      put tNewline into line tycell of tNewField
--    end if   
--     
--    put tNewText into item txcell of line tycell tNewField
--    set the cREVTable["currentview"] of pObject to tNewField 
--    put empty into tColumnValues
--    repeat with i = 1 to tMaxColumns
--      put item i of line tycell of tNewField & cr after tColumnValues
--    end repeat
--    delete last char of tColumnValues
--    send "revUpdateDatabase pObject,txcell,tycell,tNewText" to pObject
--  end if
--  revUpdateScrollInfo pObject
--  revDisplayFormattedData pObject
--  if exists(pObject) then
--    send "revUpdateCellValue pObject,txcell,tycell,tcurrentcell,tNewText" to pObject
--  end if
--end revWriteCellField
--js::20040203::new_code
on revWriteCellField pObject
  --revSetScrollInfo pObject
  put the cREVTable["currentxcell"] of pObject into txcell
  put the cREVTable["currentycell"] of pObject into tycell 
  put the cREVTable["currentview"] of pObject into tCurrentView
  put revGetCellName(pObject) into tFieldName 
  
  if there is not a field tFieldName then exit revWriteCellField
  put the long ID of field tFieldName into tFieldRef
  put the text of field tFieldName into tNewText
  put the htmltext of field tFieldName into tNewHtmlText
  -- now that we have what we need, abuse the temp field a bit *chuckle*
  lock screen
  lock messages
  -- if the currentview is not yet in htmlText, make sure to convert it
  if char 1 to 3 of tCurrentView is "<p>" and char -4 to -1 of tCurrentView is "</p>"
  then set the htmltext of field tFieldName to tCurrentView
  else set the text of field tFieldName to tCurrentView
  set the itemDelimiter to tab
  put the htmltext of item txcell of line tycell of field tFieldName into tCurrentHtmlText
  if tCurrentHtmltext contains "#9;" then
    put "<p></p>" into tCurrentHtmlText
  end if
  ## NOTE :: DO NOT REMOVE THE LOCK MESSAGES OR YOU'LL GET AN ERROR AFTER SETTING THE HTMLTEXT !!!
  if not (tNewHtmlText = tCurrentHtmlText) then
    set the htmltext of item txcell of line tycell of tFieldRef to tNewHtmlText
    set the cREVTable["currentview"] of pObject to the htmlText of tFieldRef
    try -- this has to be try because there may not be a database library in a standalone
      send "revUpdateDatabase pObject,txcell,tycell,tNewText" to pObject
    catch tError
    end try
  end if
  -- and unlock things again
  unlock messages
  unlock screen
  -- move on with the other updates
  revUpdateScrollInfo pObject
  revDisplayFormattedData pObject
  if exists(pObject) then
    send "revUpdateCellValue pObject,txcell,tycell,tcurrentcell,tNewText" to pObject
  end if
end revWriteCellField
--js::20040203::end
--js::20040715::end

--js::20040715::start
--    fixed bug #67
--js::20040715::old_code
--on revDisplayFormattedData pObject
--  put the cREVTable["currentview"] of pObject into tField
--  set the cREVTable["formattedview"] of pObject to tField
--  if the cREVTable["cellformat"] of pObject is true then
--    revDoFormatting pObject
--  end if
--  set the text of pObject to the cREVTable["formattedview"] of pObject
--end revDisplayFormattedData
--js::20040715::new_code
on revDisplayFormattedData pObject
  put the cREVTable["currentview"] of pObject into tField
  set the cREVTable["formattedview"] of pObject to tField
  if the cREVTable["cellformat"] of pObject is true then
    revDoFormatting pObject
  end if
  put the cREVTable["formattedview"] of pObject into tFormattedView
  if char 1 to 3 of tFormattedView is "<p>" and char -4 to -1 of tFormattedView is "</p>"
  then set the htmltext of pObject to tFormattedView
  else set the text of pObject to tFormattedView
end revDisplayFormattedData
--js::20040715::end

function revPercentageDisplay pPrefixList, pNumber, pGroup
  put pPrefixList into tDecimalPlaces
  if pGroup >= (the number of items in tDecimalPlaces) then
    put item (the number of items in tDecimalPlaces) of tDecimalPlaces into tDecimal
  else
    put item pGroup of tDecimalPlaces into tDecimal
  end if
  put "%0." into tFormat
  put tDecimal after tFormat
  put "f" after tFormat
  put (pNumber * 100) into pNumber
  put format(tFormat,pNumber) into tFormattedNumber
  put "%" after tFormattedNumber
  return tFormattedNumber
end revPercentageDisplay

function revGenericDisplay pPrefixList, pNumber, pGroup
  set the itemDelimiter to comma
  --  put the cREVTable["genericsymbol"] of pObject into tGenericSymbol
  put pPrefixList into tGenericSymbol
  if pGroup >= (the number of items in tGenericSymbol) then
    put item (the number of items in tGenericSymbol) of tGenericSymbol into tGeneric
  else
    put item pGroup of tGenericSymbol into tGeneric
  end if
  put tGeneric into tFormattedNumber
  put pNumber after tFormattedNumber
  return tFormattedNumber
end revGenericDisplay

function revPrefixDisplay pPrefixList, pNumber, pGroup
  set the itemDelimiter to comma
  --  put the cREVTable["genericsymbol"] of pObject into tGenericSymbol
  put pPrefixList into tGenericSymbol
  if pGroup >= (the number of items in tGenericSymbol) then
    put item (the number of items in tGenericSymbol) of tGenericSymbol into tGeneric
  else
    put item pGroup of tGenericSymbol into tGeneric
  end if
  put tGeneric into tFormattedNumber
  put pNumber after tFormattedNumber
  return tFormattedNumber
end revPrefixDisplay

function revSuffixDisplay pPrefixList, pNumber, pGroup
  set the itemDelimiter to comma
  --  put the cREVTable["genericsymbol"] of pObject into tGenericSymbol
  put pPrefixList into tGenericSymbol
  if pGroup >= (the number of items in tGenericSymbol) then
    put item (the number of items in tGenericSymbol) of tGenericSymbol into tGeneric
  else
    put item pGroup of tGenericSymbol into tGeneric
  end if
  put pNumber into tFormattedNumber
  put tGeneric after tFormattedNumber
  return tFormattedNumber
end revSuffixDisplay

function revDecimalDisplay pPrefixList, pNumber, pGroup
  set the itemDelimiter to comma
  --  put the cREVTable["decimalplaces"] of pObject into tDecimalPlaces
  put pPrefixList into tDecimalPlaces
  if pGroup >= (the number of items in tDecimalPlaces) then
    put item (the number of items in tDecimalPlaces) of tDecimalPlaces into tDecimal
  else
    put item pGroup of tDecimalPlaces into tDecimal
  end if
  put "%0." into tFormat
  put tDecimal after tFormat
  put "f" after tFormat
  put format(tFormat,pNumber) into tFormattedNumber
  return tFormattedNumber
end revDecimalDisplay

function revScientificDisplay pPrefixList, pNumber, pGroup
  set the itemDelimiter to comma
  --  put the cREVTable["scientificplaces"] of pObject into tScientificPlaces
  put pPrefixList into tScientificPlaces
  if pGroup >= (the number of items in tScientificPlaces) then
    put item (the number of items in tScientificPlaces) of tScientificPlaces into tScientific
  else
    put item pGroup of tScientificPlaces into tScientific
  end if
  put "%0." into tFormat
  put tScientific after tFormat
  put "e" after tFormat
  put format(tFormat,pNumber) into tFormattedNumber
  return tFormattedNumber
end revScientificDisplay
  
function revDateDisplay pPrefixList, pNumber, pGroup
  set the itemDelimiter to comma
  --  put the cREVTable["datetypes"] of pObject into tDateType
  put pPrefixList into tDateType
  if pGroup >= (the number of items in tDateType) then
    put item (the number of items in tDateType) of tDateType into tDate 
  else
    put item pGroup of tDateType into tDate
  end if
  switch tDate
  case "long"
    convert pNumber to long date
    break
  case "system"
    convert pNumber to system date
    break
  case "internet"
    convert pNumber to internet date
    break
  case "short"
    convert pNumber to short date
    break
  default
    convert pNumber to system date
  end switch
  
  put pNumber into tFormattedNumber
  return tFormattedNumber
end revDateDisplay
    
on revDoFormatting pObject
  put the cREVTable["formatlist"] of pObject into tFormatList
  if tFormatList is not empty then
    set the itemDelimiter to tab
    repeat with i = 1 to the number of lines in tFormatList
      if line i of tFormatList is not empty then
        put item 1 of line i of tFormatList into tCellList
        put item 2 of line i of tFormatList into tTypeList
        set the itemDelimiter to ":"
        put item 1 of tTypeList into tType
        put item 2 of tTypeList into tPrefixList
        switch tType
        case "Date"
          revDateFormat pObject,tCellList,tPrefixList
          break
        case "Prefix"
          revPrefixFormat pObject,tCellList,tPrefixList
          break
        case "Suffix"
          revSuffixFormat pObject,tCellList,tPrefixList
          break
          --        case "Generic"
          --          revGenericFormat pObject,tCellList,tPrefixList
          --          break
        case "Scientific"
          revScientificFormat pObject,tCellList,tPrefixList
          break
        case "Percentage"
          revPercentageFormat pObject,tCellList,tPrefixList
          break
        case "Decimal"
          revDecimalFormat pObject,tCellList,tPrefixList
          break
        end switch
      end if
      set the itemDelimiter to tab
    end repeat
  end if
  --  revPercentageFormat pObject
  --  revGenericFormat pObject
  --  revDecimalFormat pObject
  --  revScientificFormat pObject
  --  revDateFormat pObject
end revDoFormatting

on revDecimalFormat pObject,pCellList,pPrefixList
  --  if the cREVTable["decimalformat"] of pObject is not empty then 
  --    put the cREVTable["decimalformat"] of pObject into tRawList
  if pCellList is not empty then
    put pCellList into tRawList
    put revUnParseCellList(tRawList) into tList
    set the itemDelimiter to space
    repeat with i = 1 to the number of items in tList
      put item i of tList into tCell
      set the itemDelimiter to comma
      put item 1 of tCell into txcell
      put item 2 of tCell into tycell
      put item 3 of tCell into tgroup
      put revReadCellValue(pObject,txcell,tycell) into tValue
      if (tValue is not empty) and (tValue is a number) then
        put revDecimalDisplay(pPrefixList,tValue,tgroup) into tFormattedValue
        revWriteCellValue pObject,txcell,tycell,tFormattedValue
      end if
      set the itemDelimiter to space
    end repeat
  end if
end revDecimalFormat

on revScientificFormat pObject,pCellList,pPrefixList
  --  if the cREVTable["scientificformat"] of pObject is not empty then 
  --    put the cREVTable["scientificformat"] of pObject into tRawList
  if pCellList is not empty then
    put pCellList into tRawList
    put revUnParseCellList(tRawList) into tList
    set the itemDelimiter to space
    repeat with i = 1 to the number of items in tList
      put item i of tList into tCell
      set the itemDelimiter to comma
      put item 1 of tCell into txcell
      put item 2 of tCell into tycell
      put item 3 of tCell into tgroup
      put revReadCellValue(pObject,txcell,tycell) into tValue
      if (tValue is not empty) and (tValue is a number) then
        put revScientificDisplay(pPrefixList,tValue,tgroup) into tFormattedValue
        revWriteCellValue pObject,txcell,tycell,tFormattedValue
      end if
      set the itemDelimiter to space
    end repeat
  end if
end revScientificFormat

on revDateFormat pObject,pCellList,pPrefixList
  --  if the cREVTable["dateformat"] of pObject is not empty then 
  --    put the cREVTable["dateformat"] of pObject into tRawList
  if pCellList is not empty then
    put pCellList into tRawList
    put revUnParseCellList(tRawList) into tList
    set the itemDelimiter to space
    repeat with i = 1 to the number of items in tList
      put item i of tList into tCell
      set the itemDelimiter to comma
      put item 1 of tCell into txcell
      put item 2 of tCell into tycell
      put item 3 of tCell into tgroup
      put revReadCellValue(pObject,txcell,tycell) into tValue
      if (tValue is not empty) and (tValue is a date) then
        put revDateDisplay(pPrefixList,tValue,tgroup) into tFormattedValue
        revWriteCellValue pObject,txcell,tycell,tFormattedValue
      end if
      set the itemDelimiter to space
    end repeat
  end if
end revDateFormat
    
on revPercentageFormat pObject,pCellList,pPrefixList
  --  if the cREVTable["percentageformat"] of pObject is not empty then 
  --    put the cREVTable["percentageformat"] of pObject into tRawList
  if pCellList is not empty then
    put pCellList into tRawList
    put revUnParseCellList(tRawList) into tList
    set the itemDelimiter to space
    repeat with i = 1 to the number of items in tList
      put item i of tList into tCell
      set the itemDelimiter to comma
      put item 1 of tCell into txcell
      put item 2 of tCell into tycell
      put item 3 of tCell into tgroup
      put revReadCellValue(pObject,txcell,tycell) into tValue
      if (tValue is not empty) and (tValue is a number) then
        put revPercentageDisplay(pPrefixList,tValue,tgroup) into tFormattedValue
        revWriteCellValue pObject,txcell,tycell,tFormattedValue
      end if
      set the itemDelimiter to space
    end repeat
  end if
end revPercentageFormat

on revGenericFormat pObject,pCellList,pPrefixList
  --  if the cREVTable["genericformat"] of pObject is not empty then 
  --    put the cREVTable["genericformat"] of pObject into tRawList
  if pCellList is not empty then
    put pCellList into tRawList
    put revUnParseCellList(tRawList) into tList
    set the itemDelimiter to space
    repeat with i = 1 to the number of items in tList
      put item i of tList into tCell
      set the itemDelimiter to comma
      put item 1 of tCell into txcell
      put item 2 of tCell into tycell
      put item 3 of tCell into tgroup
      put revReadCellValue(pObject,txcell,tycell) into tValue
      if tValue is not empty then
        put revGenericDisplay(pPrefixList,tValue,tgroup) into tFormattedValue
        revWriteCellValue pObject,txcell,tycell,tFormattedValue
      end if
      set the itemDelimiter to space
    end repeat
  end if
end revGenericFormat

on revPrefixFormat pObject,pCellList,pPrefixList
  --  if the cREVTable["genericformat"] of pObject is not empty then 
  --    put the cREVTable["genericformat"] of pObject into tRawList
  if pCellList is not empty then
    put pCellList into tRawList
    put revUnParseCellList(tRawList) into tList
    set the itemDelimiter to space
    repeat with i = 1 to the number of items in tList
      put item i of tList into tCell
      set the itemDelimiter to comma
      put item 1 of tCell into txcell
      put item 2 of tCell into tycell
      put item 3 of tCell into tgroup
      put revReadCellValue(pObject,txcell,tycell) into tValue
      if tValue is not empty then
        put revPrefixDisplay(pPrefixList,tValue,tgroup) into tFormattedValue
        revWriteCellValue pObject,txcell,tycell,tFormattedValue
      end if
      set the itemDelimiter to space
    end repeat
  end if
end revPrefixFormat

on revSuffixFormat pObject,pCellList,pPrefixList
  --  if the cREVTable["genericformat"] of pObject is not empty then 
  --    put the cREVTable["genericformat"] of pObject into tRawList
  if pCellList is not empty then
    put pCellList into tRawList
    put revUnParseCellList(tRawList) into tList
    set the itemDelimiter to space
    repeat with i = 1 to the number of items in tList
      put item i of tList into tCell
      set the itemDelimiter to comma
      put item 1 of tCell into txcell
      put item 2 of tCell into tycell
      put item 3 of tCell into tgroup
      put revReadCellValue(pObject,txcell,tycell) into tValue
      if tValue is not empty then
        put revSuffixDisplay(pPrefixList,tValue,tgroup) into tFormattedValue
        revWriteCellValue pObject,txcell,tycell,tFormattedValue
      end if
      set the itemDelimiter to space
    end repeat
  end if
end revSuffixFormat

function revUnParseCellList pRawList
  set the itemDelimiter to space
  put empty into tCellList
  repeat with i = 1 to the number of items in pRawList
    put item i of pRawList into tRawCell
    set the itemDelimiter to ":"
    if the number of items in tRawCell = 1 then
      put tRawCell & "," & i & " " after tCellList
    else
      put item 1 of tRawCell into topleftcell
      put item 2 of tRawCell into bottomrightcell
      set the itemDelimiter to comma
      put item 1 of topleftcell into columnstart
      put item 1 of bottomrightcell into columnfinish
      put item 2 of topleftcell into rowstart
      put item 2 of bottomrightcell into rowfinish
      repeat with j = columnstart to columnfinish
        repeat with k = rowstart to rowfinish
          put j & "," & k & "," & i & " " after tCellList
        end repeat
      end repeat
    end if
    set the itemDelimiter to space
  end repeat
  if the last char of tCellList is space then
    delete the last char of tCellList  
  end if
  return tCellList
end revUnParseCellList

function revParseCellList pObject, pColumnList
  set the itemDelimiter to comma
  put the cREVTable["viewablerows"] of pObject into maxrows
  put empty into tParseList
  repeat with i = 1 to the number of items in pColumnList
    put item i of pColumnList into tParseColumn
    set the itemDelimiter to ":"
    if the number of items in tParseColumn = 1 then
      put tParseColumn & "," & "1" & ":" & tParseColumn & "," & maxrows & " " after tParseList
    else
      put item 1 of tParseColumn into leftcolumn
      put item 2 of tParseColumn into rightcolumn
      put leftcolumn & "," & "1" & ":" & rightcolumn & "," & maxrows & " " after tParseList
    end if
    set the itemDelimiter to comma
  end repeat
  if the last char of tParseList is space then
    delete last char of tParseList  
  end if
  return tParseList
end revParseCellList

on mouseUp
  if (word 1 of the tool is "browse") and \
      (the cREVTable["celledit"] of the long id of the target is true) and \
      (the cREVGeneral["table"] of the long id of the target is true) and \
      (within(the long id of the target, the clickLoc))
  then
    lock screen
    revDeleteCellFields
    put the long id of the target into lObject
    revGetTableSettings lObject
    revCalculateCellLoc lObject
    revCalculateCellRect lObject
    revResizeTemplates lObject
    revCreateCellField lObject
    unlock screen
  end if
  pass mouseUp
end mouseUp

--js::20040715::start
--    fixed bug #67
--js::20040715::old_code
--on revGetTableSettings pObject
--  set the cREVTable["currentxmouseloc"] of pObject to item 1 of the clickloc
--  set the cREVTable["currentymouseloc"] of pObject to item 2 of the clickloc
--  set the cREVTable["leftfieldloc"] of pObject to item 1 of the rect of pObject
--  set the cREVTable["topfieldloc"] of pObject to item 2 of the rect of pObject 
--  set the cREVTable["rightfieldloc"] of pObject to item 3 of the rect of pObject
--  set the cREVTable["bottomfieldloc"] of pObject to item 4 of the rect of pObject 
--  set the cREVTable["currenthscroll"] of pObject to the hscroll of pObject
--  set the cREVTable["currentvscroll"] of pObject to the vscroll of pObject
--  if the number of items in the tabStops of pObject is empty then
--    set the tabStops of pObject to 75
--  end if
--  set the cREVTable["numbertabstops"] of pObject to the number of items in the tabStops of pObject
--  set the cREVTable["scrollbarwidth"] of pObject to the scrollBarWidth of pObject
--  if the cREVTable["currentview"] of pObject is empty then
--    set the cREVTable["currentview"] of pObject to the text of pObject
--  end if
--  revCalculateCellSpacing pObject
--  revCalculateViewableArea pObject
--end revGetTableSettings
--js::20040715::new_code
on revGetTableSettings pObject
   set the cREVTable["currentxmouseloc"] of pObject to item 1 of the clickloc
   set the cREVTable["currentymouseloc"] of pObject to item 2 of the clickloc
   set the cREVTable["leftfieldloc"] of pObject to item 1 of the rect of pObject
   set the cREVTable["topfieldloc"] of pObject to item 2 of the rect of pObject 
   set the cREVTable["rightfieldloc"] of pObject to item 3 of the rect of pObject 
   set the cREVTable["bottomfieldloc"] of pObject to item 4 of the rect of pObject 
   set the cREVTable["currenthscroll"] of pObject to the hscroll of pObject
   set the cREVTable["currentvscroll"] of pObject to the vscroll of pObject
      
   -- fix 3231
   if the number of items in the tabStops of pObject is empty or the number of items in the tabStops of pObject is 0 then
      set the tabStops of pObject to 75
   end if
   
   set the cREVTable["numbertabstops"] of pObject to the number of items in the tabStops of pObject
   set the cREVTable["scrollbarwidth"] of pObject to the scrollBarWidth of pObject
   
   -- MD5 digest check after "or" added KM Oct 02 to fix 4079 - data updated by user
   if the cREVTable["currentview"] of pObject is empty or md5Digest(the cREVTable["currentview"] of pObject) is not md5Digest(the htmlText of pObject) then
      set the cREVTable["currentview"] of pObject to the htmltext of pObject
   end if
   
   revCalculateCellSpacing pObject
   
   revCalculateViewableArea pObject   
end revGetTableSettings
--js::20040715::end

on revMoveNextRow pObject
  put the cREVTable["currentycell"] of pObject into tycell  
  if tycell >= 1 then
    set the cREVTable["currentycell"] of pObject to (tycell + 1)
  end if
end revMoveNextRow

on revMovePrevRow pObject
  put the cREVTable["currentycell"] of pObject into tycell  
  if tycell > 1 then
    set the cREVTable["currentycell"] of pObject to (tycell - 1)
  end if
end revMovePrevRow

on revMoveNextColumn pObject
  put the cREVTable["currentxcell"] of pObject into txcell
  if txcell >= 1 then
    set the cREVTable["currentxcell"] of pObject to (txcell + 1)
  end if
end revMoveNextColumn

on revMovePrevColumn pObject
  put the cREVTable["currentxcell"] of pObject into txcell
   if txcell > 1 then
    set the cREVTable["currentxcell"] of pObject to (txcell - 1)
  end if
end revMovePrevColumn

on revCalculateCellSpacing pObject
  put the cREVTable["numbertabstops"] of pObject into tnumbertabs
   if tnumbertabs = 1 then
    put (item tnumbertabs of the tabStops of pObject) into txspacing
  else
    put (item tnumbertabs of the tabStops of pObject) - (item (tnumbertabs - 1) of the tabStops of pObject) into txspacing
  end if
  put the effective textheight of pObject into tyspacing
  set the cREVTable["cellxspacing"] of pObject to txspacing
  set the cREVTable["cellyspacing"] of pObject to tyspacing
end revCalculateCellSpacing

--js::20040715::start
--    fixed bug #67
--js::20040715::old_code
--on revCreateNextColumn pObject, pNumberofColumns
--  put the cREVTable["currentxcell"] of pObject into txcell
--  put the cREVTable["currentycell"] of pObject into tycell 
--  put the cREVTable["currentview"] of pObject into tNewField
--  put line tycell of tNewField into tNewline
--  if the last char of tNewline is cr then
--    delete the last char of tNewline
--  end if
--  put tab after tNewline
--  repeat with i = 1 to pNumberofColumns
--    put tab after tNewLine
--  end repeat
--  put tNewline into line tycell of tNewField
--  set the cREVTable["currentview"] of pObject to tNewField 
--  revDisplayFormattedData pObject
--end revCreateNextColumn
--js::20040715::new_code

-- new code implemented 25th august 2004
on revCreateNextColumn pObject, pNumberofColumns
  put the cREVTable["currentxcell"] of pObject into txcell
  put the cREVTable["currentycell"] of pObject into tycell
  put the cREVTable["currentview"] of pObject into tCurrentView
  set the itemDelimiter to tab
  -- just a rerun of the dirty trick
  lock screen
  lock messages
  create field
  put it into tFieldRef
  if char 1 to 3 of tCurrentView is "<p>" and char -4 to -1 of tCurrentView is "</p>"
  then set the htmlText of the last field to tCurrentView
  else set the text of the last field to tCurrentView
  /* put empty into item (txcell + pNumberOfColumns) of line tycell of the last field */
  
  # OK-2008-01-25 : Unneeded iterator removed and loop optimized
  --   repeat with i = 1 to pNumberofColumns
  --     put tab after line tycell of the last field
  --   end repeat
  
  local tIndent
  repeat pNumberOfColumns times
    put tab after tIndent
  end repeat
  put tIndent after line tycell of the last field
  
  put the htmlText of tFieldRef into tCurrentView
  delete the last field
  unlock messages
  unlock screen
  set the cREVTable["currentview"] of pObject to tCurrentView
  revDisplayFormattedData pObject
end revCreateNextColumn

--js::20040715::end

--js::20040715::start
--    fixed bug #67
--js::20040715::old_code
--on revCheckNextColumn pObject
--  put the cREVTable["currentxcell"] of pObject into txcell
--  put the cREVTable["currentycell"] of pObject into tycell 
--  set the itemDelimiter to tab
--  put the number of items in line tycell of the cREVTable["currentview"] of pObject into tColumns 
--  set the itemDelimiter to comma
--  put the cREVTable["currentview"] of pObject into tNewField
--  if txcell >= tColumns then
--    if the last char of tNewline is cr then
--      delete the last char of tNewline
--    end if
--    put line tycell of tNewField into tNewline
--    put tab & tab after tNewline
--    put tNewline into line tycell of tNewField
--  end if
--  set the cREVTable["currentview"] of pObject to tNewField 
--  revDisplayFormattedData pObject
--end revCheckNextColumn
--js::20040715::new_code
on revCheckNextColumn pObject
  put the cREVTable["currentxcell"] of pObject into txcell
  put the cREVTable["currentycell"] of pObject into tycell 
  put the cREVTable["currentview"] of pObject into tCurrentView 
  set the itemDelimiter to tab
  -- just a rerun of the dirty trick
  lock screen
  lock messages
  create field
  if char 1 to 3 of tCurrentView is "<p>" and char -4 to -1 of tCurrentView is "</p>"
  then set the htmlText of the last field to tCurrentView
  else set the text of the last field to tCurrentView
  put the number of items in line tycell of the last field into tColumns 
  if txcell >= tColumns then put empty into item tColumns + 2 of line tycell of the last field
  put the htmltext of the last field into tCurrentView
  delete the last field
  unlock messages
  unlock screen
  set the cREVTable["currentview"] of pObject to tCurrentView
  revDisplayFormattedData pObject
end revCheckNextColumn
--js::20040715::end

--js::20040715::start
--    fixed bug #67
--js::20040715::old_code
--on revCheckNextRow pObject
--  put the cREVTable["currentxcell"] of pObject into txcell
--  put the cREVTable["currentycell"] of pObject into tycell 
--  put the number of lines of the cREVTable["currentview"] of pObject into tMaxRows 
--  put the cREVTable["currentview"] of pObject into tNewField
--  set the itemDelimiter to tab
--  put 0 into tMaxColumns
--  repeat with i = 1 to tMaxRows
--    if the number of items in line i of the cREVTable["currentview"] of pObject > tMaxColumns
--    then
--      put the number of items in line i of the cREVTable["currentview"] of pObject into tMaxColumns
--    end if
--  end repeat
--  if txcell > tMaxColumns then
--    put txcell into tMaxColumns
--  end if
--  if tycell >= tMaxRows then
--    if the last char of tNewField is not cr then
--      put cr after tNewfield
--    end if
--    repeat with i = 1 to 2  
--      repeat with j = 1 to (tMaxColumns)
--        put tab after tNewfield
--      end repeat
--      put cr after tNewField
--    end repeat
--    if the last char of tNewField is cr then
--      delete last char of tNewField   
--    end if
--  end if
--  set the cREVTable["currentview"] of pObject to tNewField 
--  revDisplayFormattedData pObject
--end revCheckNextRow
--js::20040715::new_code
on revCheckNextRow pObject
  put the cREVTable["currentxcell"] of pObject into txcell
  put the cREVTable["currentycell"] of pObject into tycell 
  put the cREVTable["currentview"] of pObject into tCurrentView 
  set the itemDelimiter to tab
  -- just a rerun of the dirty trick
  lock screen
  lock messages
  create field
  put it into tFieldRef
  if char 1 to 3 of tCurrentView is "<p>" and char -4 to -1 of tCurrentView is "</p>"
  then set the htmlText of the last field to tCurrentView
  else set the text of the last field to tCurrentView
  put the number of lines of tCurrentView into tMaxRows
  put 1 into tMaxColumns
  
  # OK-2007-01-25 : Optmization of loop
  --  repeat with i = 1 to tMaxRows
  --    if the number of items in line i of the last field > tMaxColumns then
  --      put the number of items in line i of the last field into tMaxColumns
  --    end if
  --  end repeat
  put calculateMaxColumns(the long id of the last field, tMaxRows, tab) into tMaxColumns
  
  if txcell > tMaxColumns then
    put txcell into tMaxColumns
  end if
  ##js::20040830::start
  ##js::20040830::old_code
  --if tycell >= tMaxRows then
  --put empty into item (tMaxColumns + 1) of line (tycell + 1) of tFieldRef
  --put empty into item (tMaxColumns + 1) of line (tycell + 2) of tFieldRef
  --end if
  --put word 1 to -1 of tFieldRef into tFieldRef -- seems to get blank returns (KM)
  --put the htmltext of tFieldRef into tCurrentView
  ##js::20040830::new_code
  if tycell >= tMaxRows - 1 then
    put empty into item (tMaxColumns + 1) of line (tycell + 1) of the last field
    put empty into item (tMaxColumns + 1) of line (tycell + 2) of the last field
  end if
  put the htmltext of the last field into tCurrentView
  ##js::20040830::end
  delete the last field
  unlock messages
  unlock screen
  set the cREVTable["currentview"] of pObject to tCurrentView
  revDisplayFormattedData pObject
end revCheckNextRow
--js::20040715::end

# OK-2008-01-25 : Refactored and optimized duplicated code
# Parameters
#    pField : reference to a field object
#    pMaxRows : the maximum number of lines to count down to
# Description
#   Returns the largest number of items on a line of pField up to pMaxRows
private function calculateMaxColumns pField, pMaxRows, pDelimiter
  local tRowNumber
  put 0 into tRowNumber
  
  set the itemDelimiter to pDelimiter
  
  local tMaxColumns
  repeat for each line tLine in the text of pField
    local tItemCount
    put the number of items of tLine into tItemCount
    if tItemCount > tMaxColumns then
      put tItemCount into tMaxColumns
    end if
    add 1 to tRowNumber
    if tRowNumber > pMaxRows then
      exit repeat
    end if
  end repeat
  
  return tMaxColumns
end calculateMaxColumns



on revCalculateCellLoc pObject
  revUpdateScrollInfo pObject
  put the cREVTable["currentxmouseloc"] of pObject into txmouseloc
  put the cREVTable["currentymouseloc"] of pObject into tymouseloc
  put the cREVTable["leftfieldloc"] of pObject into txfieldloc
  put the cREVTable["topfieldloc"] of pObject into tyfieldloc 
  put the cREVTable["currenthscroll"] of pObject into thscroll
  put the cREVTable["currentvscroll"] of pObject into tvscroll
  put the cREVTable["cellxspacing"] of pObject into txspacing
  put the cREVTable["cellyspacing"] of pObject into tyspacing
  put the cREVTable["numbertabstops"] of pObject into tnumbertabs
  
  put txmouseloc + thscroll - txfieldloc into tcolumnpixel
  put tymouseloc + tvscroll - tyfieldloc into trowpixel
  put 0 into txpixel
  put 0 into typixel
  put 0 into txcount
  put 0 into tycount
  
  repeat until txpixel > tcolumnpixel
    put txcount + 1 into txcount
    if txcount <= tnumbertabs then
      put item txcount of the tabStops of pObject into txpixel 
      --put txpixel + txfieldloc into trightrect
      --      if txcount = 1 then
      --        put txfieldloc into tleftrect
      --      else
      --        put txfieldloc + (item (txcount - 1) of the tabStops of pObject) into tleftrect
      --      end if
    else
      put txpixel + txspacing into txpixel
      --put txpixel + txfieldloc into trightrect
      --put txpixel + txfieldloc - txspacing into tleftrect
    end if
  end repeat
  
  
  put word 2 of the clickLine into tycount
  if tycount is empty then
    -- recursive search for line number
    repeat until typixel > trowpixel
      put tycount + 1 into tycount
      put typixel + tyspacing into typixel
      --put typixel + tyfieldloc into tbottomrect
      --put typixel + tyfieldloc - tyspacing into ttoprect
    end repeat
  end if
   
  if the cREVTable["maxColumnCount"] of pObject is a number then
    put min(the cREVTable["maxColumnCount"] of pObject,txcount) into txcount
  end if
   
  set the cREVTable["currentxcell"] of pObject to txcount
  set the cREVTable["currentycell"] of pObject to tycount  
end revCalculateCellLoc

--js::20040715::start
--    fixed bug #67
--js::20040715::old_code
--on revCalculateViewableArea pObject
--  revUpdateScrollInfo pObject
--  put the cREVTable["rightfieldloc"] of pObject into txmouseloc
--  put the cREVTable["bottomfieldloc"] of pObject into tymouseloc
--  put the cREVTable["leftfieldloc"] of pObject into txfieldloc
--  put the cREVTable["topfieldloc"] of pObject into tyfieldloc 
--  put the cREVTable["currenthscroll"] of pObject into thscroll
--  put the cREVTable["currentvscroll"] of pObject into tvscroll
--  put the cREVTable["cellxspacing"] of pObject into txspacing
--  put the cREVTable["cellyspacing"] of pObject into tyspacing
--  put the cREVTable["numbertabstops"] of pObject into tnumbertabs
--  put the number of lines of the cREVTable["currentview"] of pObject into tMaxRows
--  put txmouseloc + thscroll - txfieldloc into tcolumnpixel
--  put tymouseloc + tvscroll - tyfieldloc into trowpixel
--  put 0 into txpixel
--  put 0 into typixel
--  put 0 into txcount
--  put 0 into tycount
--  
--  repeat until txpixel > tcolumnpixel
--    put txcount + 1 into txcount
--    if txcount <= tnumbertabs then
--      put item txcount of the tabStops of pObject into txpixel 
--      --put txpixel + txfieldloc into trightrect
--      --      if txcount = 1 then
--      --        put txfieldloc into tleftrect
--      --      else
--      --        put txfieldloc + (item (txcount - 1) of the tabStops of pObject) into tleftrect
--      --      end if
--    else
--      put txpixel + txspacing into txpixel
--      --put txpixel + txfieldloc into trightrect
--      --put txpixel + txfieldloc - txspacing into tleftrect
--    end if
--  end repeat
--  
--  repeat until typixel > trowpixel
--    put tycount + 1 into tycount
--    put typixel + tyspacing into typixel
--    --put typixel + tyfieldloc into tbottomrect
--    --put typixel + tyfieldloc - tyspacing into ttoprect
--  end repeat
--   
--  put 0 into tMaxColumns
--  repeat with i = 1 to tMaxRows
--    if the number of items in line i of the cREVTable["currentview"] of pObject > tMaxColumns
--    then
--      put the number of items in line i of the cREVTable["currentview"] of pObject into tMaxColumns
--    end if
--  end repeat
--  if (tMaxRows is not empty) and (tMaxRows > tycount) then
--    put tMaxRows into tycount
--  end if
--  if (tMaxColumns is not empty) and (tMaxColumns > txcount) then
--    put tMaxColumns into txcount
--  end if
--  
--  set the cREVTable["viewablecolumns"] of pObject to txcount
--  set the cREVTable["viewablerows"] of pObject to tycount  
--end revCalculateViewableArea
--js::20040715::new_code
on revCalculateViewableArea pObject
   revUpdateScrollInfo pObject
   put the cREVTable["rightfieldloc"] of pObject into txmouseloc
   put the cREVTable["bottomfieldloc"] of pObject into tymouseloc
   put the cREVTable["leftfieldloc"] of pObject into txfieldloc
   put the cREVTable["topfieldloc"] of pObject into tyfieldloc 
   put the cREVTable["currenthscroll"] of pObject into thscroll
   put the cREVTable["currentvscroll"] of pObject into tvscroll
   put the cREVTable["cellxspacing"] of pObject into txspacing
   put the cREVTable["cellyspacing"] of pObject into tyspacing
   put the cREVTable["numbertabstops"] of pObject into tnumbertabs
   put the cREVTable["currentview"] of pObject into tcurrentview
   put the number of lines of tcurrentview into tMaxRows
   put txmouseloc + thscroll - txfieldloc into tcolumnpixel
   put tymouseloc + tvscroll - tyfieldloc into trowpixel
   put 0 into txpixel
   put 0 into typixel
   put 0 into txcount
   put 0 into tycount
   
   ## Bug 11189
   ## When last 2 values of tab stops were the same this was causing an infinte loop
   if txspacing < 1 then put 1 into txspacing
   repeat until txpixel > tcolumnpixel
      put txcount + 1 into txcount
      if txcount <= tnumbertabs then 
         if item  txcount of the tabStops of pObject > txpixel then
            put item txcount of the tabStops of pObject into txpixel 
         else
            put txpixel + txspacing into txpixel
         end if 
      else
         put txpixel + txspacing into txpixel
      end if
   end repeat

   repeat until typixel > trowpixel
      put tycount + 1 into tycount
      put typixel + tyspacing into typixel
   end repeat

   -- and the dirty trick rears its head again
   lock screen
   lock messages
   create field
   if char 1 to 3 of tcurrentview is "<p>" and char -4 to -1 of tcurrentview is "</p>"
   then set the htmltext of the last field to tcurrentview
   else set the text of the last field to tcurrentview
   put 0 into tMaxColumns
   # OK-2008-01-25 : Optimization of loop
   --  repeat with i = 1 to tMaxRows
   --    if the number of items in line i of the last field > tMaxColumns
   --    then put the number of items in line i of the last field into tMaxColumns
   --  end repeat
   put calculateMaxColumns(the long id of the last field, tMaxRows, tab) into tMaxColumns

   delete the last field
   unlock messages
   unlock screen
   -- proceed as if nothing happened
   if (tMaxRows is not empty) and (tMaxRows > tycount) then
      put tMaxRows into tycount
   end if
   if (tMaxColumns is not empty) and (tMaxColumns > txcount) then
      put tMaxColumns into txcount
   end if

   set the cREVTable["viewablecolumns"] of pObject to txcount
   set the cREVTable["viewablerows"] of pObject to tycount  

end revCalculateViewableArea
--js::20040715::end

on revCalculateCellRect pObject
  revUpdateScrollInfo pObject
  put the cREVTable["leftfieldloc"] of pObject into txfieldloc
  put the cREVTable["topfieldloc"] of pObject into tyfieldloc 
  put the cREVTable["rightfieldloc"] of pObject into trightfieldloc
  put the cREVTable["scrollbarwidth"] of pObject into tscrollbarwidth
  put the cREVTable["currenthscroll"] of pObject into thscroll
  put the cREVTable["currentvscroll"] of pObject into tvscroll
  put the cREVTable["cellxspacing"] of pObject into txspacing
  put the cREVTable["cellyspacing"] of pObject into tyspacing
  put the cREVTable["currentxcell"] of pObject into txcell
  put the cREVTable["currentycell"] of pObject into tycell
  put the cREVTable["numbertabstops"] of pObject into tnumbertabs  
  --  put revReadCellValue(pObject,txcell,tycell) into tcellvalue
  put revReadCellValue(pObject,txcell,tycell,"html") into tcellvalue
  set the cREVTable["currentcellvalue"] of pObject to tcellvalue
  set the itemDelimiter to comma
  
  put (the margins of pObject) into tfudge
  put 0 into txpixel
  put 0 into typixel
  repeat with i = 1 to txcell
    if i <= tnumbertabs then
      put item i of the tabStops of pObject into txpixel 
      put txpixel + txfieldloc into trightrect
      if i = 1 then
        put txfieldloc into tleftrect
      else
        put txfieldloc + (item (i - 1) of the tabStops of pObject) into tleftrect
      end if
    else
      put txpixel + txspacing into txpixel
      put txpixel + txfieldloc into trightrect
      put trightrect - txspacing into tleftrect
    end if
  end repeat
  
  repeat with i = 1 to tycell
    put typixel + tyspacing into typixel
    put typixel + tyfieldloc into tbottomrect
    put tbottomrect - tyspacing into ttoprect
  end repeat
  
  put tleftrect - thscroll into tleftrect
  put trightrect - thscroll into trightrect
  put ttoprect - tvscroll into ttoprect
  put tbottomrect - tvscroll into tbottomrect
  
  
  if tbottomrect > (the cREVTable["bottomfieldloc"] of pObject - the cREVTable["scrollbarwidth"] of pObject - tfudge) then
    revCheckNextRow pObject
    put (tyspacing * 1) into tymove
    put tvscroll + tymove into tnewvscroll
    set the cREVTable["currentvscroll"] of pObject to tnewvscroll
    revUpdateScrollInfo pObject
    put ttoprect - tymove into ttoprect
    put tbottomrect - tymove into tbottomrect
  end if
  if ttoprect < the cREVTable["topfieldloc"] of pObject then
    put (tyspacing * 1) into tymove
    put tvscroll - tymove into tnewvscroll
    if tnewvscroll < 0 then
      set the cREVTable["currentvscroll"] of pObject to 0
      put ttoprect + tymove + tnewvscroll into ttoprect
      put tbottomrect + tymove + tnewvscroll into tbottomrect
    else
      set the cREVTable["currentvscroll"] of pObject to tnewvscroll
      put ttoprect + tymove into ttoprect
      put tbottomrect + tymove into tbottomrect
    end if
    revUpdateScrollInfo pObject
  end if
  if trightrect > (the cREVTable["rightfieldloc"] of pObject - the cREVTable["scrollbarwidth"] of pObject - tfudge) then
    revCheckNextColumn pObject
    if txcell <= tnumbertabs then
      if txcell = 1 then
        put (item txcell of the tabStops of pObject) into txmove
      else
        put (item txcell of the tabStops of pObject) - (item (txcell - 1) of the tabStops of pObject) into txmove
      end if
    else
      put (txspacing * 1) into txmove
    end if
    -- includes the case of cell fields being larger than text field width
    put thscroll + (tleftrect - txfieldloc) into tnewhscroll
    put txfieldloc into tleftrect
    put txmove + txfieldloc into trightrect
    if trightrect > trightfieldloc then
      put trightfieldloc - tscrollbarwidth into trightrect
    end if
    set the cREVTable["currenthscroll"] of pObject to tnewhscroll
    revUpdateScrollInfo pObject
    -- creates extra columns if necessary for the scrollbar
    put the hscroll of pObject into tTestHScroll
    put 1 into tCreateColumn
    repeat until tTestHScroll = tnewhscroll
      revCreateNextColumn pObject, tCreateColumn
      revUpdateScrollInfo pObject
      put the hscroll of pObject into tTestHScroll
    end repeat
  end if
  if tleftrect < the cREVTable["leftfieldloc"] of pObject then
    if txcell <= tnumbertabs then
      if txcell = 1 then
        put (item txcell of the tabStops of pObject) into txmove
      else
        put (item txcell of the tabStops of pObject) - (item (txcell - 1) of the tabStops of pObject) into txmove
      end if
    else
      put (txspacing * 1) into txmove
    end if
    -- includes the case of cell fields being larger than text field width
    put txmove into txmoveorg
    put txmove - (trightrect - txfieldloc) into txmove
    put thscroll - txmove into tnewhscroll
    put txfieldloc into tleftrect
    put txmoveorg + txfieldloc into trightrect
    if trightrect > trightfieldloc then
      put trightfieldloc - tscrollbarwidth into trightrect
    end if
    set the cREVTable["currenthscroll"] of pObject to tnewhscroll
    revUpdateScrollInfo pObject
  end if
  
  set the cREVTable["leftcellloc"] of pObject to tleftrect + tfudge - 5
  set the cREVTable["topcellloc"] of pObject to ttoprect + tfudge - 7
  set the cREVTable["rightcellloc"] of pObject to trightrect + tfudge - 5
  set the cREVTable["bottomcellloc"] of pObject to tbottomrect + tfudge
end revCalculateCellRect

function revGetCellRect pObject
  put the cREVTable["leftcellloc"] of pObject into tResult
  put "," after tResult
  put the cREVTable["topcellloc"] of pObject after tResult 
  put "," after tResult
  put the cREVTable["rightcellloc"] of pObject after tResult
  put "," after tResult
  put the cREVTable["bottomcellloc"] of pObject after tResult 
  return tResult
end revGetCellRect

function revGetCellName pObject
  put "revCell-" into tResult
  put the cREVTable["currentxcell"] of pObject after tResult
  put "," after tResult
  put the cREVTable["currentycell"] of pObject after tResult 
  return tResult
end revGetCellName

--js::20040715::start
--    fixed bug #67
--js::20040715::old_code
--function revReadCellValue pObject, pColumn, pRow
--  set the itemDelimiter to tab
--  put the cREVTable["currentview"] of pObject into tField
--  put item pColumn of line pRow of tField into tValue
--  return tValue
--end revReadCellValue
--js::20040715::new_code
function revReadCellValue pObject, pColumn, pRow, pFormat
  set the itemDelimiter to tab
  put the cREVTable["currentview"] of pObject into tCurrentView
  -- you guessed it, dirty trick upcoming
  lock screen
  lock messages
  create field
  put it into tFieldRef
  if char 1 to 3 of tcurrentview is "<p>" and char -4 to -1 of tcurrentview is "</p>"
  then set the htmltext of the last field to tcurrentview
  else set the text of the last field to tcurrentview
  if pFormat is "html" then
    put the htmltext of item pColumn of line pRow of tFieldRef into tValue
    if tValue contains "#9;" then
      put "<p></p>" into tValue
    end if
  else put item pColumn of line pRow of the text of tFieldRef into tValue--mf:20050708 bug #2225
  delete the last field
  --NOTE::end
  unlock messages
  unlock screen
  return tValue
end revReadCellValue
--js::20040715::end

--js::20040715::start
--    fixed bug #67
--js::20040715::old_code
--on revWriteCellValue pObject, pColumn, pRow, pValue
--  set the itemDelimiter to tab
--  put the cREVTable["formattedview"] of pObject into tNewField
--  put pValue into item pColumn of line pRow tNewField
--  set the cREVTable["formattedview"] of pobject to tNewField
--end revWriteCellValue
--js::20040715::new_code
on revWriteCellValue pObject, pColumn, pRow, pValue
  set the itemDelimiter to tab
  put the cREVTable["currentview"] of pObject into tCurrentView
  -- you guessed it, dirty trick upcoming
  lock screen
  lock messages
  create field
  put it into tFieldRef
  if char 1 to 3 of tcurrentview is "<p>" and char -4 to -1 of tcurrentview is "</p>"
  then set the htmltext of the last field to tcurrentview
  else set the text of the last field to tcurrentview
  if char 1 to 3 of pValue = "<p>" and char -4 to -1 of pValue is "</p>"
  then set the htmltext of item pColumn of line pRow of tFieldRef to pValue
  else put pValue into item pColumn of line pRow of the last field--mf::20050708 bug #2225
  put the htmltext of the last field into tcurrentview
  delete the last field
  unlock messages
  unlock screen
  set the cREVTable["currentview"] of pObject to tCurrentView
  set the htmlText of pObject to tCurrentView--mf::20050708 bug #2225
end revWriteCellValue
--js::20040715::end

--js::20040715::start
--    fixed bug #67
--js::20040715::old_code
--on revResizeTemplates pObject
--  put revGetCellName(pObject) into lFieldName
--  put revGetCellRect(pObject) into tRect
--  put ((item 1 of tRect) + (item 3 of tRect)) / 2 into tavlength
--  put ((item 2 of tRect) + (item 4 of tRect)) / 2 into tavheight
--  put (item 3 of tRect) - (item 1 of tRect) into tLength
--  put (item 4 of tRect) - (item 2 of tRect) into tHeight
--  set the location of the templatefield to (tavlength,tavheight)
--  set the height of the templatefield to tHeight
--  set the width of the templatefield to tLength
--  set the threeD of the templatefield to true
--  set the borderWidth of the templatefield to 1
--  set the text of the templatefield to the cREVTable["currentcellvalue"] of pObject
--  set the name of the templatefield to lFieldName
--  set the focus of the templatefield to true
--  set the cREVTable["acell"] of the templatefield to true
--  --  set the location of the templategraphic to (tavlength,tavheight)
--  --  set the height of the templategraphic to tHeight
--  --  set the width of the templategraphic to tLength
--  --  set the threeD of the templategraphic to false
--  --  set the opaque of the templategraphic to false
--end revResizeTemplates
--js::20040715::new_code
on revResizeTemplates pObject
  put revGetCellName(pObject) into lFieldName
  put revGetCellRect(pObject) into tRect
  put ((item 1 of tRect) + (item 3 of tRect)) / 2 into tavlength
  put ((item 2 of tRect) + (item 4 of tRect)) / 2 into tavheight
  put (item 3 of tRect) - (item 1 of tRect) into tLength
  put (item 4 of tRect) - (item 2 of tRect) into tHeight
  set the location of the templatefield to (tavlength,tavheight+1)  -- added 1 KM to neaten look
  set the height of the templatefield to tHeight - 8 -- added 8 KM
  set the width of the templatefield to tLength - 2 -- added 2 KM
  set the threeD of the templatefield to true
  set the borderWidth of the templatefield to 1
  set the htmltext of the templatefield to the cREVTable["currentcellvalue"] of pObject
  set the name of the templatefield to lFieldName
  
  # OK-2009-01-29 : Set metadata on the cell fields so that we know which object they are attached to.
  # These means that when we get a closeField / exitField message targeted to the cell, we can delete it,
  # and then redirect the message to the actual table field.
  set the cREVTable["owner"] of the templateField to pObject
  
  set the focus of the templatefield to true
  
  set the cREVTable["acell"] of the templatefield to true
  set the cREVTable["acell"] of this card to true -- needed to avoid cycling through controls unnecessarily
end revResizeTemplates
--js::20040715::end

on revCreateCellField pObject
  put the cREVTable["celldelay"] of pObject into tDelay
  if not revIsColumnLocked(pObject) then
    put revGetCellName(pObject) into tFieldName
    revCellDelay tDelay
    lock messages
    create field
    --create graphic tName
    select text of field tFieldName
    set the dontWrap of field tFieldName to true
    unlock messages
    reset templatefield
    --reset templategraphic
  end if
end revCreateCellField

function revIsColumnLocked pObject
  put the cREVTable["currentxcell"] of pObject into tColumn
  if the cREVTable["columnlock"] of pObject is not empty then
    put the cREVTable["columnlock"] of pObject into tLockList
    put revParseRangeList(tLockList) into tLock
    if tColumn is among the items of tLock then
      return true
    else 
      return false
    end if
  else 
    return false
  end if
end revIsColumnLocked

function revParseRangeList pRawList
  set the itemDelimiter to comma
  put empty into tRangeList
  put pRawList into tRawLineList
  repeat with j = 1 to the number of items in tRawLineList
    put item j of tRawLineList into tRawRange
    set the itemDelimiter to "-"
    if the number of items in tRawRange = 1 then
      put tRawRange & "," after tRangeList
    else
      put item 1 of tRawRange into tstart
      put item 2 of tRawRange into tfinish
      repeat with k = tstart to tfinish
        put k & "," after tRangeList
      end repeat
    end if
    set the itemDelimiter to comma
  end repeat
  if the last char of tRangeList is comma then
    delete the last char of tRangeList
  end if
  
  return tRangeList
end revParseRangeList

on revUpdateScrollInfo pObject
  lock messages
  set the hscroll of pObject to the cREVTable["currenthscroll"] of pObject
  set the vscroll of pObject to the cREVTable["currentvscroll"] of pObject
  unlock messages
end revUpdateScrollInfo

on revSetScrollInfo pObject
  lock messages
  set the cREVTable["currenthscroll"] of pObject to the hscroll of pObject
  set the cREVTable["currentvscroll"] of pObject to the vscroll of pObject
  unlock messages
end revSetScrollInfo

on revCellDelay pTime
  if (pTime is not empty) and (pTime is a number) then
    wait for pTime milliseconds
  end if
end revCellDelay

on revEmptyTable pObject
  put the long id of pObject into tObject
  if (word 1 of tObject is "field") and (the cREVGeneral["table"] of tObject is true) and (exists(tObject)) then
    set the text of tObject to empty
    set the cREVTable["currentview"] of tObject to empty
    set the cREVtable["returnedview"] of tObject to empty
  end if
end revEmptyTable
