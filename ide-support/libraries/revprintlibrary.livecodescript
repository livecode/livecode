script "revprintlibrary"

on revLoadLibrary
   insert the script of me into back
end revLoadLibrary

on revUnloadLibrary
   remove the script of me from back
end revUnloadLibrary

----------------------printing handlers
local lPageNumber, pageNumber --holds lPageNumber between handlers
local lAnswerPrinter, lPageSetup
local numPages
constant kPo = 36

on revShowPrintDialog pShowPageSetup, pShowPrinter
  --pAnswerPrinter governs whether to show number of copies / etc. box
  --pAnswerPrinter covers page setup on Windows as these are the same dialog
  --pMacPrinter governs whether to show Mac printer box (MacOS only)
  put pShowPrinter into lAnswerPrinter
  put pShowPageSetup into lPageSetup
end revShowPrintDialog

on revPrintField pFieldRef
  --just takes a field reference and prints the field, complete with styles and no fuss
  --if not (there is a fld pFieldRef) then do "put the short name of" && pFieldRef && "into pFieldRef"
  revPrintText the htmlText of pFieldRef,empty,empty,the long id of pFieldRef
end revPrintField

on revPrintText pText,pHeader,pFooter,pSourceFieldRef,pHeaderFldRef,pFooterFldRef
   --pText contains the text, either html or text
   --pHeader contains header text, either html or text
   --pFooter contains header text, either html or text
   --pSourceFieldRef contains a vaild field descriptor
   local tActualPageWidth --width of page subtracting right and left printmargins
   local tPrintMargins --margins stored in array for script simplification
   local tPageWidth,tPageHeight --height and width of the page
   local tScrolling,tPageLine,tScrollList --scroll values for text height calculations
   lock messages
   --General Initialization
   if the printPageNumber is empty then
      if lAnswerPrinter is empty then put true into lAnswerPrinter
      if lPageSetUp is empty then put true into lPageSetup
      if lPageSetup then answer page setup
      if the result is "Cancel" then
         return "cancel"
      end if
      if lAnswerPrinter then answer printer
      if the result is "Cancel" then
         return "cancel"
      end if
   end if
   open printing
   if pSourceFieldRef <> "" and pHeaderFldRef = "" then put pSourceFieldRef into pHeaderFldRef
   if pSourceFieldRef <> "" and pFooterFldRef = "" then put pSourceFieldRef into pFooterFldRef
   
   put item 1 of the printmargins into tPrintMargins["left"]
   put item 2 of the printmargins into tPrintMargins["top"]
   put item 3 of the printmargins into tPrintMargins["right"]
   put item 4 of the printmargins into tPrintMargins["bottom"]
   
   put item 3 of the printPaperRectangle - item 1 of the printPaperRectangle into tPageWidth
   put item 4 of the printPaperRectangle - item 2 of the printPaperRectangle into tPageHeight
   
   put 0 into tScrolling
   put 0 into tPageLine
   put 1 into lPageNumber
   put lPageNumber into pageNumber
   put tPageWidth -tPrintMargins["right"]-tPrintMargins["left"] into tActualPageWidth
   -- reset the stack & field templates (fix wrapping bug where dontWrap
   -- of the templateField is set before calling)
   reset the templateStack
   reset the templateField
   -- prepare template for invisible stacks the width of the page
   set the backgroundcolor of the templateStack to "white"
   if pSourceFieldRef is not empty then
      put revTargetStack(pSourceFieldRef) into tTargetStack
      set the underLineLinks of the templateStack to the underLineLinks of stack tTargetStack
      set the linkColor of the templateStack to the linkColor of stack tTargetStack
      set the linkHiliteColor of the templateStack to the linkHiliteColor of stack tTargetStack
      set the linkVisitedColor of the templateStack to the linkVisitedColor of stack tTargetStack
   end if
   set the visible of the templateStack to false
   set the width of the templateStack to tActualPageWidth
   if the printerName is not "pdf" then
      set the formatForPrinting of the templateStack to false
      set the formatForPrinting of the templateStack to true
   end if
   -- prepare template for fields in these stacks
   set the width of the templateField to tActualPageWidth
   -- the following three properties are set so that text height calculations are correct
   set the borderwidth of the templateField to 0
   set the margins of the templateField to 6
   set the showborder of the templateField to true
   set the opaque of the templateField to false
   set the fixedLineHeight of the templateField to false
   set the lockLoc of the templateField to true
   -- Header and Footer initialization
   if pHeader is not empty then
      create stack "revTempHeader"
      set the defaultStack to "revTempHeader"
      set the backColor of this card to "white" -- work around 1955 unicode color issue
      revSetPrintFont pHeaderFldRef
      get revCreatePrintHeaderAndFooter(pHeader)
      -- if size of header or footer is greater then current margins will allow, then
      -- increase top or bottom margins.
      if it > tPrintMargins["top"] then put it into tPrintMargins["top"]
   end if
   if pFooter is not empty then
      create stack "revTempFooter"
      set the defaultStack to "revTempFooter"
      revSetPrintFont pFooterFldRef
      get revCreatePrintHeaderAndFooter(pFooter)
      if it>tPrintMargins["bottom"] then put it into tPrintMargins["bottom"]
   end if
   
   --Body initialization
   --set height of the templateStack to tPageHeight - tPrintMargins["bottom"] - tPrintMargins["top"]
   global gREVPrintFileName
   if gREVPrintFileName is empty or there is a stack gREVPrintFileName then put "revTempBody" into gREVPrintFileName
   create stack gREVPrintFileName
   set the defaultStack to gREVPrintFileName
   set height of this stack to tPageHeight - tPrintMargins["bottom"] - tPrintMargins["top"]
   revSetPrintFont pSourceFieldRef
   create field
   if pSourceFieldRef is not empty then
      set the tabStops of field 1 to the tabStops of pSourceFieldRef
      set the hGrid of field 1 to the hGrid of pSourceFieldRef
      set the vGrid of field 1 to the vGrid of pSourceFieldRef
      set the fixedLineHeight of field 1 to the fixedLineHeight of pSourceFieldRef -- NEW
      set the textHeight of field 1 to the effective textHeight of pSourceFieldRef -- NEW
      set the textAlign of field 1 to the effective textAlign of pSourceFieldRef
   end if
   set the rect of field 1 to rect of this card
   revPrintSetText pText
   put the pageheights of field 1 into tScrollList
   --Printing Cycle
   -- headers and footers are split up into three fields by tab items
   set the itemDel to tab
   put the number of lines of tScrollList into numPages
   repeat
      If pHeader is not empty then
         set the defaultStack to "revTempHeader"
         repeat with lLoop = 1 to 3
            put item lLoop of pHeader into field lLoop
            if field lLoop <> empty then revPrintSetText field lLoop,lLoop
         end repeat
         print this card into tPrintMargins["left"],kPo,tPageWidth-tPrintMargins["right"],kPo+the height of stack "revTempHeader"
      end if
      If pFooter is not empty then
         set the defaultStack to "revTempFooter"
         repeat with lLoop = 1 to 3
            put item lLoop of pFooter into field lLoop
            if field lLoop <> empty then revPrintSetText field lLoop,lLoop
         end repeat
         print this card into tPrintMargins["left"],tPageHeight-kPo-height of stack "revTempFooter",tPageWidth-tPrintMargins["right"],tPageHeight-kPo
      end if
      add 1 to tPageLine
      add 1 to lPageNumber
      put lPageNumber into pageNumber
      set the defaultStack to gREVPrintFileName
      set the height of fld 1 to line tPageLine of tScrollList
      set the scroll of fld 1 to tScrolling
      add line tPageLine of tScrollList to tScrolling
      print this card into tPrintMargins["left"],tPrintMargins["top"],tPageWidth-tPrintMargins["right"],tPageHeight - tPrintMargins["bottom"]
      if tPageLine is numPages then exit repeat
      print break
   end repeat
   -- clean up
   close printing
   if there is a stack "revTempHeader" then delete stack "revTempHeader"
   if there is a stack "revTempFooter" then delete stack "revTempFooter"
   delete stack gREVPrintFileName
   reset the templateStack
   reset the templateField
end revPrintText


on revPrintSetText pTheText, pFldNum
  local tExpressionStart, tExpressionEnd
  if pFldNum = "" then put 1 into pFldNum
  if "<p>" is in pTheText or matchChunk(pTheText, "<(.+>.+</.+)>") is true then set the htmltext of field pFldNum to pTheText
  else put pTheText into field pFldNum
  repeat until matchChunk(field pFldNum, "<%(.[^<]*)%>", tExpressionStart, tExpressionEnd) is false
    put value(char tExpressionStart to tExpressionEnd of field pFldNum) into char tExpressionStart-2 to tExpressionEnd+2 of field pFldNum
  end repeat
end revPrintSetText

-- Need to set the font after the stack is created to prevent inheritance of
-- non-formatForPrinting fonts from Home stack

on revSetPrintFont pSourceFieldRef
  if pSourceFieldRef is not empty then
    if the effective textFont of pSourceFieldRef is among the lines of fontNames("printer") then
      set the textFont of this stack to the effective textFont of pSourceFieldRef
    else
      if "Arial" is among the lines of fontNames("Printer") then set the textFont of this stack to "Arial"
      else if "Helvetica" is among the lines of fontNames("Printer") then set the textFont of this stack to "Helvetica" 
      else set the textFont of this stack to line 1 of fontNames("printer")
    end if
    set the textSize of this stack to the effective textSize of pSourceFieldRef
    set the textStyle of this stack to the effective textStyle of pSourceFieldRef
  else
    set the textSize of this stack to 12
    if the effective textFont of this stack is not among the lines of fontNames("printer") then
      if "Arial" is among the lines of fontNames("printer") then set the textFont of this stack to "Arial"
      else if "Helvetica" is among the lines of fontNames("Printer") then set the textFont of this stack to "Helvetica"
      else set the textFont of this stack to line 1 of fontNames("printer")
    end if
  end if
end revSetPrintFont

function revCreatePrintHeaderAndFooter pTheText
  create field
  set the itemDelimiter to tab
  if item 1 of pTheText <> empty then
    revPrintSetText item 1 of pTheText,1
    put the formattedheight of field 1 into lFH1
  else put 0 into lFH1
  create field
  if item 2 of pTheText <> empty then
    revPrintSetText item 2 of pTheText,2
    put the formattedheight of field 2 into lFH2
  else put 0 into lFH2
  create field
  if item 3 of pTheText <> empty then
    revPrintSetText item 3 of pTheText,3
    put the formattedheight of field 3 into lFH3
  else put 0 into lFH3
  set the height of this stack to max(lFH1,lFH2,lFH3)
  get rect of this card
  set the rect of field 1 to it
  set the rect of field 2 to it
  set the rect of field 3 to it
  set the textalign of field 1 to left
  set the textalign of field 2 to "center"
  set the textalign of field 3 to right
  return height of this stack+kPo
end revCreatePrintHeaderAndFooter
----------------end of printing handlers

--------report manager handlers
on revUpdateViewerObjectList
  put 0 into tViewerObjects
  repeat with i = 1 to the number of controls of this card of the topStack
    put the long id of control i of this card of the topStack into tControl
    if (the cREVReport["viewername"] of tControl is not empty) and (the cREVReport["sourceobjectstackname"] of tControl is not empty) then
      put tViewerObjects + 1 into tViewerObjects
    end if 
  end repeat
  set the cREVReport["viewerobjects"] of this card of the topStack to tViewerObjects
  put empty into tViewerList
  repeat with i = 1 to tViewerObjects
    put the long id of control i of the topStack into tObject
    put the cREVReport["viewername"] of tObject into tViewerName
    put the cREVReport["sourceobjectstackname"] of tObject into tSourceStackName
    put the cREVReport["sourceobjectlongid"] of tObject into tSourceObjectlongid
    put tViewerName & "," & tSourceStackName & "," & tSourceObjectlongid & cr after tViewerList
  end repeat
  if the last char of tViewerList is cr then
    delete the last char of tViewerList
  end if
  set the cREVReport["sourceobjectlist"] of this card of the topStack to tViewerList
end revUpdateViewerObjectList




function revGetMaxNumberCards
  if the cREVGeneral["report"] of this card of the topStack is true then
    put 0 into tMaxCard
    repeat with i = 1 to the number of controls in this card of the topStack
      put the long id of control i of the topStack into tObject
      if the cREVGeneral["viewerobject"] of tObject is not true then
        next repeat
      end if
      if the cREVReport["maxcards"] of tObject > tMaxCard then
        put the cREVReport["maxcards"] of tObject into tMaxCard 
      end if
    end repeat
  end if
  return tMaxCard
end revGetMaxNumberCards

function revSourceObject pViewerName, pCardNumber
  put the cREVReport["sourceobjectlist"] of this card of the topStack into tList
  repeat with i = 1 to the number of lines in tList
    if pViewerName is item 1 of line i of tList then
      put item 3 of line i of tList into tFullPath
      replace "of" with "," in tFullPath
      put item 1 of tFullPath into tObject
      if the last char of tObject is space then
        delete the last char of tObject
      end if
      if the paramCount is 2 then
        put tObject && "of card" && pCardNumber && "of stack" && quote & item 2 of line i of tList & quote into tSourceObjectName
      else
        put tObject && "of card revSourceCardNumber of stack" && quote & item 2 of line i of tList & quote into tSourceObjectName
      end if
      exit repeat
    end if
  end repeat
  return tSourceObjectName
end revSourceObject

function revGetExpressionCards
  local revSourceCardNumber, revSourceStackName, revSourceCardShortName, revSourceCardLongID
  if the cREVGeneral["report"] of this card of the topStack is true then
    put the cREVReport["viewerobjects"] of this card of the topStack into tNumber
    put empty into tExpressionList
    repeat with i = 1 to tNumber
      put empty into tExpressionLineList
      put the long id of control i of the topStack into tObject
      if the cREVGeneral["viewerobject"] of tObject is not true then
        next repeat
      end if
      put the cREVReport["maxcards"] of tObject into tMaxCard
      put the cREVReport["sourceobjectstackname"] of tObject into revSourceStackName
      repeat with j = 1 to tMaxCard
        put empty into tResult
        put the cREVReport["expression"] of this card of the topStack into tDoStatement
        put j into revSourceCardNumber
        --put the short name of card j of stack (the cREVReport["sourceobjectstackname"] of tObject) into revSourceCardShortName
        --put the long id of card j of stack (the cREVReport["sourceobjectstackname"] of tObject) into revSourceCardLongID
        --put value(tValue,tCardObject) into tResult
        do tDoStatement
        put the result into tResult
        if tResult is true then
          put j & "," after tExpressionLineList
        end if
      end repeat
      if the last char of tExpressionLineList is comma then
        delete the last char of tExpressionLineList
      end if
      set the cREVReport["expressioncards"] of tObject to tExpressionLineList
      put tExpressionLineList & cr after tExpressionList
    end repeat
    if the last char of tExpressionList is cr then
      delete the last char of tExpressionList
    end if
  end if
  return tExpressionList
end revGetExpressionCards

function revGetMarkedCards
  if the cREVGeneral["report"] of this card of the topStack is true then
    put the cREVReport["viewerobjects"] of this card of the topStack into tNumber
    put empty into tMarkedList
    repeat with i = 1 to tNumber
      put empty into tMarkedLineList
      put the long id of control i of the topStack into tObject
      if the cREVGeneral["viewerobject"] of tObject is not true then
        next repeat
      end if
      put the cREVReport["maxcards"] of tObject into tMaxCard
      repeat with j = 1 to tMaxCard
        if the mark of card j of stack (the cREVReport["sourceobjectstackname"] of tObject) is true then
          put j & "," after tMarkedLineList
        end if
      end repeat
      if the last char of tMarkedLineList is comma then
        delete the last char of tMarkedLineList
      end if
      set the cREVReport["markedcards"] of tObject to tMarkedLineList
      put tMarkedLineList & cr after tMarkedList
    end repeat
    if the last char of tMarkedList is cr then
      delete the last char of tMarkedList
    end if
  end if
  return tMarkedList
end revGetMarkedCards

function revGetUnmarkedCards
  if the cREVGeneral["report"] of this card of the topStack is true then
    put the cREVReport["viewerobjects"] of this card of the topStack into tNumber
    put empty into tUnmarkedList
    repeat with i = 1 to tNumber
      put empty into tUnmarkedLineList
      put the long id of control i of the topStack into tObject
      if the cREVGeneral["viewerobject"] of tObject is not true then
        next repeat
      end if
      put the cREVReport["maxcards"] of tObject into tMaxCard
      repeat with j = 1 to tMaxCard
        if the mark of card j of stack (the cREVReport["sourceobjectstackname"] of tObject) is false then
          put j & "," after tUnmarkedLineList
        end if
      end repeat
      if the last char of tUnmarkedLineList is comma then
        delete the last char of tUnmarkedLineList
      end if
      set the cREVReport["unmarkedcards"] of tObject to tUnmarkedLineList
      put tUnmarkedLineList & cr after tUnmarkedList
    end repeat
    if the last char of tUnmarkedList is cr then
      delete the last char of tUnmarkedList
    end if
  end if
  return tUnmarkedList
end revGetUnmarkedCards

on revPrintAllCards pCards
  put revGetMaxNumberCards() into tMaxCards
  if tMaxCards >= 1 then
    put 1 & "-" & tMaxCards into tRange
    put revReportParseRangeList(tRange) into tList
    revPrintCards tList, pCards
  end if
end revPrintAllCards

on revPrintMarkedCards pCards
  put revGetMarkedCards() into tRawList
  put revReportParseRangeList(tRawList) into tList
  if tList is not empty then
    revPrintCards tList, pCards
  end if
end revPrintMarkedCards

on revPrintUnmarkedCards pCards
  put revGetUnmarkedCards() into tRawList
  put revReportParseRangeList(tRawList) into tList
  if tList is not empty then
    revPrintCards tList, pCards
  end if
end revPrintUnmarkedCards

on revPrintRangeCards pCards
  if the cREVReport["cardrange"] of this card of the topStack is not empty then
    put the cREVReport["cardrange"] of this card of the topStack into tRawList
    put revReportParseRangeList(tRawList) into tList
    if tList is not empty then
      revPrintCards tList, pCards
    end if
  end if
end revPrintRangeCards

on revPrintExpressionCards pCards
  put revGetExpressionCards() into tRawList
  put revReportParseRangeList(tRawList) into tList
  if tList is not empty then
    revPrintCards tList, pCards
  end if
end revPrintExpressionCards

on revReturnConfirmPrint pValue
  if pValue is true then
    put true into lPrintConfirm
  else
    put false into lPrintConfirm
  end if
end revReturnConfirmPrint

function revPrintConfirmation pNumberList
  if ("revConfirmPrint" is in the script of this card of the topStack) and (the cREVGeneral["report"] of this card of the topStack is true) then
    put the cREVReport["viewerobjects"] of this card of the topStack into tNumber
    put the cREVReport["sourceobjectlist"] of this card of the topStack into tInfoList
    repeat with i = 1 to tNumber
      set the itemDelimiter to ":"
      put item i of pNumberList into pNumber
      put the long id of field i of this card of the topStack into tObject
      repeat with j = 1 to the number of lines in tInfoList
        put the cREVReport["sourceobjectlongid"] of tObject into tLongID
        set the itemDelimiter to comma
        if item 3 of line j of tInfoList is tLongID then
          put pNumber into item 4 of line j of tInfoList
        end if
      end repeat
    end repeat
    put tInfoList into gREVReportInfo
    put "revConfirmPrint()" into tFunction
    return value(tFunction,this card of the topStack)
--    put empty into lPrintConfirm
--    send "revConfirmPrint tInfoList" to this card of the topStack
--    wait until lPrintConfirm is not empty
--    if lPrintConfirm is false then
--      return false
--    else if lPrintConfirm is true then
--      return true
--    end if
  else
    return true
  end if
end revPrintConfirmation

on revPrintCards pList, pCards
  put the cREVReport["header"] of this card of the topStack into tHeader
  put the cREVReport["footer"] of this card of the topStack into tFooter
  
  if (the cREVReport["onetoone"] of this card of the topStack is true) then
    repeat for each item i in pList
      revMoveToCard i
      if revPrintConfirmation(i) is true then
        open printing
        if (tHeader is not empty) and (there is a stack tHeader) then
          print stack tHeader
        end if
        
        if revStackBiggerThanPaper() is true then
          revPrintMultiplePageCard
        else
          print this card of the topStack
        end if
        
        if (tFooter is not empty) and (there is a stack tFooter) then
          print stack tFooter
        end if
        close printing
      end if
    end repeat
  else if (the cREVReport["onetoone"] of this card of the topStack is false) then
    if the cREVReport["multiplecards"] of this card of the topStack is true then
      repeat for each item i in pList
        revMoveToCard i
        if revPrintConfirmation(i) is true then
          open printing
          if (tHeader is not empty) and (there is a stack tHeader) then
            print stack tHeader
          end if
          repeat with j = 1 to pCards
            if revStackBiggerThanPaper() is true then
              revPrintMultiplePageCard
            else
              print this card of the topStack
            end if
          end repeat
          if (tFooter is not empty) and (there is a stack tFooter) then
            print stack tFooter
          end if
          close printing
        end if
      end repeat
    else
       
      --open printing
      put 0 into tCardNumber
      repeat for each item i in pList
        revMoveToCard i
        if revPrintConfirmation(i) is true then
          if tCardNumber = 0 then
            open printing
            if (tHeader is not empty) and (there is a stack tHeader) then
              print stack tHeader
            end if
          end if
           
          if revStackBiggerThanPaper() is true then
            revPrintMultiplePageCard
          else
            print this card of the topStack 
          end if
           
          put tCardNumber + 1 into tCardNumber
          if (tCardNumber is pCards) or (i is the last item in pList) then
            if (tFooter is not empty) and (there is a stack tFooter) then
              print stack tFooter
            end if
            close printing
            put 0 into tCardNumber
          end if
        end if
      end repeat
      --close printing
       
    end if
  end if
  put empty into gREVReportInfo
end revPrintCards

function revStackBiggerThanPaper
  put revCalculateEffectivePrintArea() into tPaperSize
  put item 1 of tPaperSize into tPaperWidth
  put item 2 of tPaperSize into tPaperHeight
  put the width of this card of the topStack into tStackWidthPixel
  put the height of this card of the topStack into tStackHeightPixel
  put revPixelToPt(tStackWidthPixel) into tStackWidth
  put revPixelToPt(tStackHeightPixel) into tStackHeight
  if (tStackWidth > tPaperWidth) or (tStackHeight > tPaperHeight) then
    return true
  else
    return false
  end if
end revStackBiggerThanPaper

on revPrintMultiplePageCard
  put revCalculateNumberPages() into tRectList
  open printing
  repeat with i = 1 to the number of lines in tRectList
    set the itemDelimiter to space
    print this card of the topStack from item 1 of line i of tRectList to item 2 of line i of tRectList
    print break
  end repeat
  close printing
end revPrintMultiplePageCard


function revReportParseRangeList pRawList
  put the cREVReport["viewerobjects"] of this card of the topStack into tViewerNumber
  set the itemDelimiter to comma
  put empty into tRangeList
  put empty into tParseRangeList
  put 0 into tMaxItems
  repeat with i = 1 to the number of lines in pRawList
    put line i of pRawList into tRawLineList
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
    put cr after tRangeList
  end repeat
  if the last char of tRangeList is cr then
    delete the last char of tRangeList
  end if
   
  repeat with i = 1 to the number of lines in tRangeList
    if the number of items of line i of tRangeList > tMaxItems then
      put the number of items of line i of tRangeList into tMaxItems
    end if
  end repeat
   
  repeat with i = 1 to tMaxItems
    repeat with j = 1 to tViewerNumber
      if the number of lines in tRangeList = 1 then
        put item i of line 1 of tRangeList & ":" after tParseRangeList
      else
        if item i of line j of tRangeList is empty then
          put "0:" after tParseRangeList
        else
          put item i of line j of tRangeList & ":" after tParseRangeList
        end if
      end if
    end repeat
    if the last char of tParseRangeList is ":" then
      delete the last char of tParseRangeList
    end if
    put "," after tParseRangeList
  end repeat
  if the last char of tParseRangeList is "," then
    delete the last char of tParseRangeList
  end if
  return tParseRangeList
end revReportParseRangeList

on revMoveToCard pNumberList
  --answer pNumberList
  if the cREVGeneral["report"] of this card of the topStack is true then
    if (the cREVReport["virtualcard"] of this card of the topStack is false) or (the cREVReport["virtualcard"] of this card of the topStack is empty) then
      put the cREVReport["viewerobjects"] of this card of the topStack into tNumber
      repeat with i = 1 to tNumber
        set the itemDelimiter to ":"
        put item i of pNumberList into pNumber
        put the long id of control i of this card of the topStack into tObject
        if the cREVGeneral["viewerobject"] of tObject is not true then
          next repeat
        end if
        put the cREVReport["maxcards"] of tObject into tMaxCard
        if (pNumber <= tMaxCard) and (pNumber > 0) then
          if not revIsPageLocked(tObject) then
            put the cREVReport["currentcard"] of tObject into tOldNumber
            set the cREVReport["currentcard"] of tObject to pNumber
            --send "revChangePage tOldNumber,pNumber" to revSourceObject(the cREVReport["viewername"] of tObject,pNumber)
            --set the text of tObject to the text of control id (the cREVReport["sourceobjectshortid"] of tObject) of card (the cREVReport["currentcard"] of tObject) of stack (the cREVReport["sourceobjectstackname"] of tObject)
            revDoReportFormatting tObject
            send "revChangePage tOldNumber,pNumber" to revSourceObject(the cREVReport["viewername"] of tObject,pNumber)
          end if
        else
          set the text of tObject to empty
        end if
      end repeat
    else
      -- Do scrolling of field
    end if
  end if
end revMoveToCard

function revIsPageLocked tObject
  if the cREVReport["pagelock"] of tObject is true then
    return true
  else
    return false
  end if
end revIsPageLocked

on revSetPaperMargins
  put the cREVReport["paperleftmargin"] of this card of the topStack into tleft
  put the cREVReport["papertopmargin"] of this card of the topStack into ttop
  put the cREVReport["paperrightmargin"] of this card of the topStack into tright
  put the cREVReport["paperbottommargin"] of this card of the topStack into tbottom
  put tleft & "," & ttop & "," & tright & "," & tbottom into tMargins
  set the printMargins to tMargins
  put the cREVReport["paperverticalmargin"] of this card of the topStack into tvertical
  put the cREVReport["paperhorizontalmargin"] of this card of the topStack into thorizontal
  put thorizontal & "," & tvertical into tGutter
  set the printGutters to tGutter
end revSetPaperMargins

on revPrintReport
  revUpdateViewerObjectList
  if (the cREVGeneral["report"] of this card of the topStack is true) and (the cREVReport["viewerobjects"] of this card of the topStack > 0) then
    revSetPaperMargins
    put the cREVReport["printrange"] of this card of the topStack into tPrintRange
    put revCalculateNumberPrintCards() into tCards
    set the formatForPrinting of the topStack to true
    switch tPrintRange
    case "All Cards"
      revPrintAllCards tCards
      break
    case "Marked Cards"
      revPrintMarkedCards tCards
      break
    case "Unmarked Cards"
      revPrintUnmarkedCards tCards
      break
    case "A Specific Range of Cards"
      revPrintRangeCards tCards
      break
    case "Cards Fitting an Expression"
      revPrintExpressionCards tCards
      break
    end switch
  end if
end revPrintReport

on revMoveFirstCard
  if the cREVGeneral["report"] of this card of the topStack is true then
    if (the cREVReport["virtualcard"] of this card of the topStack is false) or (the cREVReport["virtualcard"] of this card of the topStack is empty) then
      put the cREVReport["viewerobjects"] of this card of the topStack into tNumber
      repeat with i = 1 to tNumber
        put the long id of control i of the topStack into tObject
        if the cREVGeneral["viewerobject"] of tObject is not true then
          next repeat
        end if
        if not revIsPageLocked(tObject) then
          if the cREVReport["currentcard"] of tObject is not empty then
            put the cREVReport["currentcard"] of tObject into tOldNumber
            set the cREVReport["currentcard"] of tObject to 1
            --send "revChangePage tOldNumber,1" to revSourceObject(the cREVReport["viewername"] of tObject,1)
            --set the text of tObject to the text of control id (the cREVReport["sourceobjectshortid"] of tObject) of card (the cREVReport["currentcard"] of tObject) of stack (the cREVReport["sourceobjectstackname"] of tObject)
            revDoReportFormatting tObject
            send "revChangePage tOldNumber,1" to revSourceObject(the cREVReport["viewername"] of tObject,1)
          end if
        end if
      end repeat
    else
      -- Do scrolling of field
    end if
  end if
end revMoveFirstCard

on revMoveNextCard
  if the cREVGeneral["report"] of this card of the topStack is true then
    if (the cREVReport["virtualcard"] of this card of the topStack is false) or (the cREVReport["virtualcard"] of this card of the topStack is empty) then
      put the cREVReport["viewerobjects"] of this card of the topStack into tNumber
      repeat with i = 1 to tNumber
        put the long id of control i of the topStack into tObject
        if the cREVGeneral["viewerobject"] of tObject is not true then
          next repeat
        end if
        put the cREVReport["currentcard"] of tObject into tCard
        put the cREVReport["maxcards"] of tObject into tMaxCard
        if tCard < tMaxCard then
          if not revIsPageLocked(tObject) then
            put tCard + 1 into tCard
            put the cREVReport["currentcard"] of tObject into tOldNumber
            set the cREVReport["currentcard"] of tObject to tCard
            --send "revChangePage tOldNumber,tCard" to revSourceObject(the cREVReport["viewername"] of tObject,tCard)
            --set the text of tObject to the text of control id (the cREVReport["sourceobjectshortid"] of tObject) of card (the cREVReport["currentcard"] of tObject) of stack (the cREVReport["sourceobjectstackname"] of tObject)
            revDoReportFormatting tObject
            send "revChangePage tOldNumber,tCard" to revSourceObject(the cREVReport["viewername"] of tObject,tCard)
          end if
        end if
      end repeat
    else
      -- Do scrolling of field
    end if
  end if
end revMoveNextCard

on revMovePreviousCard
  if the cREVGeneral["report"] of this card of the topStack is true then
    if (the cREVReport["virtualcard"] of this card of the topStack is false) or (the cREVReport["virtualcard"] of this card of the topStack is empty) then
      put the cREVReport["viewerobjects"] of this card of the topStack into tNumber
      repeat with i = 1 to tNumber
        put the long id of control i of the topStack into tObject
        if the cREVGeneral["viewerobject"] of tObject is not true then
          next repeat
        end if
        put the cREVReport["currentcard"] of tObject into tCard
        put the cREVReport["maxcards"] of tObject into tMaxCard
        if tCard > 1 then
          if not revIsPageLocked(tObject) then
            put tCard - 1 into tCard
            put the cREVReport["currentcard"] of tObject into tOldNumber
            set the cREVReport["currentcard"] of tObject to tCard
            --send "revChangePage tOldNumber,tCard" to revSourceObject(the cREVReport["viewername"] of tObject,tCard)
            --set the text of tObject to the text of control id (the cREVReport["sourceobjectshortid"] of tObject) of card (the cREVReport["currentcard"] of tObject) of stack (the cREVReport["sourceobjectstackname"] of tObject)
            revDoReportFormatting tObject
            send "revChangePage tOldNumber,tCard" to revSourceObject(the cREVReport["viewername"] of tObject,tCard)
          end if
        end if
      end repeat
    else
      --Do scrolling
    end if
  end if
end revMovePreviousCard

on revMoveLastCard
  if the cREVGeneral["report"] of this card of the topStack is true then
    if (the cREVReport["virtualcard"] of this card of the topStack is false) or (the cREVReport["virtualcard"] of this card of the topStack is empty) then
      put the cREVReport["viewerobjects"] of this card of the topStack into tNumber
      repeat with i = 1 to tNumber
        put the long id of control i of the topStack into tObject
        if the cREVGeneral["viewerobject"] of tObject is not true then
          next repeat
        end if
        if not revIsPageLocked(tObject) then
          if the cREVReport["currentcard"] of tObject is not empty then
            put the cREVReport["maxcards"] of tObject into tMaxCard
            put the cREVReport["currentcard"] of tObject into tOldNumber
            set the cREVReport["currentcard"] of tObject to tMaxCard
            --send "revChangePage tOldNumber,tMaxCard" to revSourceObject(the cREVReport["viewername"] of tObject,tMaxCard)
            --set the text of tObject to the text of control id (the cREVReport["sourceobjectshortid"] of tObject) of card (the cREVReport["currentcard"] of tObject) of stack (the cREVReport["sourceobjectstackname"] of tObject)
            revDoReportFormatting tObject
            send "revChangePage tOldNumber,tMaxCard" to revSourceObject(the cREVReport["viewername"] of tObject,tMaxCard)
          end if
        end if
      end repeat
    else
      --Do scrolling
    end if
  end if
end revMoveLastCard

on revDoReportFormatting pObject
  put the cREVReport["formatlist"] of pObject into tFormatList
  put the cREVReport["sourceobjectshortid"] of pObject into tShortID
  put the cREVReport["currentcard"] of pObject into tCard
  put the cREVReport["sourceobjectstackname"] of pObject into tStackName
  set the cREVReport["currentview"] of pObject to the text of control id tShortID of card tCard of stack tStackName
  set the cREVReport["originalview"] of pObject to the cREVReport["currentview"] of pObject
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
          revReportDateFormat pObject,tCellList,tPrefixList
          break
        case "Prefix"
          revReportPrefixFormat pObject,tCellList,tPrefixList
          break
        case "Suffix"
          revReportSuffixFormat pObject,tCellList,tPrefixList
          break
          --        case "Generic"
          --          revGenericFormat pObject,tCellList,tPrefixList
          --          break
        case "Scientific"
          revReportScientificFormat pObject,tCellList,tPrefixList
          break
        case "Percentage"
          revReportPercentageFormat pObject,tCellList,tPrefixList
          break
        case "Decimal"
          revReportDecimalFormat pObject,tCellList,tPrefixList
          break
        end switch
      end if
      set the itemDelimiter to tab
      set the cREVReport["currentview"] of pObject to the cREVReport["formattedview"] of pObject
    end repeat
  else
    set the cREVReport["formattedview"] of pObject to the cREVReport["currentview"] of pObject
  end if
  set the text of pObject to the cREVReport["formattedview"] of pObject
end revDoReportFormatting

on revReportDecimalFormat pObject,pCellList,pPrefixList
  put the cREVReport["sourceobjectshortid"] of pObject into tShortID
  put the cREVReport["currentcard"] of pObject into tCard
  put the cREVReport["sourceobjectstackname"] of pObject into tStackName
  put the cREVReport["currentview"] of pObject into tView
  put tView into tFormattedView
  if pCellList is not empty then
    switch pCellList
    case "To items"
      repeat with i = 1 to the number of items in tView
        put item i of tView into tValue
        if (tValue is not empty) and (tValue is a number) then
          put revDecimalDisplay(pPrefixList,tValue,1) into tFormattedValue
          replace item i of tFormattedView with tFormattedValue in tFormattedView
        end if
      end repeat
      break
    case "To words"
      repeat with i = 1 to the number of words in tView
        put word i of tView into tValue
        if (tValue is not empty) and (tValue is a number) then
          put revDecimalDisplay(pPrefixList,tValue,1) into tFormattedValue
          replace word i of tFormattedView with tFormattedValue in tFormattedView
        end if
      end repeat
      break
    case "To lines"
      repeat with i = 1 to the number of lines in tView
        put line i of tView into tValue
        if (tValue is not empty) and (tValue is a number) then
          put revDecimalDisplay(pPrefixList,tValue,1) into tFormattedValue
          replace line i of tFormattedView with tFormattedValue in tFormattedView
        end if
      end repeat
      break
    case "To whole label"
      put tView into tValue
      if (tValue is not empty) and (tValue is a number) then
        put revDecimalDisplay(pPrefixList,tValue,1) into tFormattedValue
        put tFormattedValue into tFormattedView
      end if
      break
    end switch
  end if
  set the cREVReport["formattedview"] of pObject to tFormattedView
end revReportDecimalFormat

on revReportScientificFormat pObject,pCellList,pPrefixList
  put the cREVReport["sourceobjectshortid"] of pObject into tShortID
  put the cREVReport["currentcard"] of pObject into tCard
  put the cREVReport["sourceobjectstackname"] of pObject into tStackName
  put the cREVReport["currentview"] of pObject into tView
  put tView into tFormattedView
  if pCellList is not empty then
    switch pCellList
    case "To items"
      repeat with i = 1 to the number of items in tView
        put item i of tView into tValue
        if (tValue is not empty) and (tValue is a number) then
          put revScientificDisplay(pPrefixList,tValue,1) into tFormattedValue
          replace item i of tFormattedView with tFormattedValue in tFormattedView
        end if
      end repeat
      break
    case "To words"
      repeat with i = 1 to the number of words in tView
        put word i of tView into tValue
        if (tValue is not empty) and (tValue is a number) then
          put revScientificDisplay(pPrefixList,tValue,1) into tFormattedValue
          replace word i of tFormattedView with tFormattedValue in tFormattedView
        end if
      end repeat
      break
    case "To lines"
      repeat with i = 1 to the number of lines in tView
        put line i of tView into tValue
        if (tValue is not empty) and (tValue is a number) then
          put revScientificDisplay(pPrefixList,tValue,1) into tFormattedValue
          replace line i of tFormattedView with tFormattedValue in tFormattedView
        end if
      end repeat
      break
    case "To whole label"
      put tView into tValue
      if (tValue is not empty) and (tValue is a number) then
        put revScientificDisplay(pPrefixList,tValue,1) into tFormattedValue
        put tFormattedValue into tFormattedView
      end if
      break
    end switch
  end if
  set the cREVReport["formattedview"] of pObject to tFormattedView
end revReportScientificFormat

on revReportDateFormat pObject,pCellList,pPrefixList
  put the cREVReport["sourceobjectshortid"] of pObject into tShortID
  put the cREVReport["currentcard"] of pObject into tCard
  put the cREVReport["sourceobjectstackname"] of pObject into tStackName
  put the cREVReport["currentview"] of pObject into tView
  put tView into tFormattedView
  if pCellList is not empty then
    switch pCellList
    case "To items"
      repeat with i = 1 to the number of items in tView
        put item i of tView into tValue
        if (tValue is not empty) and (tValue is a date) then
          put revDateDisplay(pPrefixList,tValue,1) into tFormattedValue
          replace item i of tFormattedView with tFormattedValue in tFormattedView
        end if
      end repeat
      break
    case "To words"
      repeat with i = 1 to the number of words in tView
        put word i of tView into tValue
        if (tValue is not empty) and (tValue is a date) then
          put revDateDisplay(pPrefixList,tValue,1) into tFormattedValue
          replace word i of tFormattedView with tFormattedValue in tFormattedView
        end if
      end repeat
      break
    case "To lines"
      repeat with i = 1 to the number of lines in tView
        put line i of tView into tValue
        if (tValue is not empty) and (tValue is a date) then
          put revDateDisplay(pPrefixList,tValue,1) into tFormattedValue
          replace line i of tFormattedView with tFormattedValue in tFormattedView
        end if
      end repeat
      break
    case "To whole label"
      put tView into tValue
      if (tValue is not empty) and (tValue is a date) then
        put revDateDisplay(pPrefixList,tValue,1) into tFormattedValue
        put tFormattedValue into tFormattedView
      end if
      break
    end switch
  end if
  set the cREVReport["formattedview"] of pObject to tFormattedView
end revReportDateFormat

on revReportPercentageFormat pObject,pCellList,pPrefixList
  put the cREVReport["sourceobjectshortid"] of pObject into tShortID
  put the cREVReport["currentcard"] of pObject into tCard
  put the cREVReport["sourceobjectstackname"] of pObject into tStackName
  put the cREVReport["currentview"] of pObject into tView
  put tView into tFormattedView
  if pCellList is not empty then
    switch pCellList
    case "To items"
      repeat with i = 1 to the number of items in tView
        put item i of tView into tValue
        if (tValue is not empty) and (tValue is a number) then
          put revPercentageDisplay(pPrefixList,tValue,1) into tFormattedValue
          replace item i of tFormattedView with tFormattedValue in tFormattedView
        end if
      end repeat
      break
    case "To words"
      repeat with i = 1 to the number of words in tView
        put word i of tView into tValue
        if (tValue is not empty) and (tValue is a number) then
          put revPercentageDisplay(pPrefixList,tValue,1) into tFormattedValue
          replace word i of tFormattedView with tFormattedValue in tFormattedView
        end if
      end repeat
      break
    case "To lines"
      repeat with i = 1 to the number of lines in tView
        put line i of tView into tValue
        if (tValue is not empty) and (tValue is a number) then
          put revPercentageDisplay(pPrefixList,tValue,1) into tFormattedValue
          replace line i of tFormattedView with tFormattedValue in tFormattedView
        end if
      end repeat
      break
    case "To whole label"
      put tView into tValue
      if (tValue is not empty) and (tValue is a number) then
        put revPercentageDisplay(pPrefixList,tValue,1) into tFormattedValue
        put tFormattedValue into tFormattedView
      end if
      break
    end switch
  end if
  set the cREVReport["formattedview"] of pObject to tFormattedView
end revReportPercentageFormat

on revReportPrefixFormat pObject,pCellList,pPrefixList
  put the cREVReport["sourceobjectshortid"] of pObject into tShortID
  put the cREVReport["currentcard"] of pObject into tCard
  put the cREVReport["sourceobjectstackname"] of pObject into tStackName
  put the cREVReport["currentview"] of pObject into tView
  put tView into tFormattedView
  if pCellList is not empty then
    switch pCellList
    case "To items"
      repeat with i = 1 to the number of items in tView
        put item i of tView into tValue
        if (tValue is not empty) then
          put revPrefixDisplay(pPrefixList,tValue,1) into tFormattedValue
          replace item i of tFormattedView with tFormattedValue in tFormattedView
        end if
      end repeat
      break
    case "To words"
      repeat with i = 1 to the number of words in tView
        put word i of tView into tValue
        if (tValue is not empty) then
          put revPrefixDisplay(pPrefixList,tValue,1) into tFormattedValue
          replace word i of tFormattedView with tFormattedValue in tFormattedView
        end if
      end repeat
      break
    case "To lines"
      repeat with i = 1 to the number of lines in tView
        put line i of tView into tValue
        if (tValue is not empty) then
          put revPrefixDisplay(pPrefixList,tValue,1) into tFormattedValue
          replace line i of tFormattedView with tFormattedValue in tFormattedView
        end if
      end repeat
      break
    case "To whole label"
      put tView into tValue
      if (tValue is not empty) then
        put revPrefixDisplay(pPrefixList,tValue,1) into tFormattedValue
        put tFormattedValue into tFormattedView
      end if
      break
    end switch
  end if
  set the cREVReport["formattedview"] of pObject to tFormattedView
end revReportPrefixFormat

on revReportSuffixFormat pObject,pCellList,pPrefixList
  put the cREVReport["sourceobjectshortid"] of pObject into tShortID
  put the cREVReport["currentcard"] of pObject into tCard
  put the cREVReport["sourceobjectstackname"] of pObject into tStackName
  put the cREVReport["currentview"] of pObject into tView
  put tView into tFormattedView
  if pCellList is not empty then
    switch pCellList
    case "To items"
      repeat with i = 1 to the number of items in tView
        put item i of tView into tValue
        if (tValue is not empty) then
          put revSuffixDisplay(pPrefixList,tValue,1) into tFormattedValue
          replace item i of tFormattedView with tFormattedValue in tFormattedView
        end if
      end repeat
      break
    case "To words"
      repeat with i = 1 to the number of words in tView
        put word i of tView into tValue
        if (tValue is not empty) then
          put revSuffixDisplay(pPrefixList,tValue,1) into tFormattedValue
          replace word i of tFormattedView with tFormattedValue in tFormattedView
        end if
      end repeat
      break
    case "To lines"
      repeat with i = 1 to the number of lines in tView
        put line i of tView into tValue
        if (tValue is not empty) then
          put revSuffixDisplay(pPrefixList,tValue,1) into tFormattedValue
          replace line i of tFormattedView with tFormattedValue in tFormattedView
        end if
      end repeat
      break
    case "To whole label"
      put tView into tValue
      if (tValue is not empty) then
        put revSuffixDisplay(pPrefixList,tValue,1) into tFormattedValue
        put tFormattedValue into tFormattedView
      end if
      break
    end switch
  end if
  set the cREVReport["formattedview"] of pObject to tFormattedView
end revReportSuffixFormat


function revPixelToPt pPixel
  put pPixel * (105/300) * (2.8346457) into tPt
  return tPt
end revPixelToPt

function revPtToPixel pPt
  put pPt * (300/(105 * 2.8346457)) into tPixel
  return tPixel
end revPtToPixel

function revCalculateNumberPages
  put revCalculateEffectivePrintArea() into tPaperSize
  put item 1 of tPaperSize into tPaperWidth
  put item 2 of tPaperSize into tPaperHeight
  put revPtToPixel(tPaperWidth) into tPaperWidthPixel
  put revPtToPixel(tPaperHeight) into tPaperHeightPixel
  put the width of this card of the topStack into tStackWidthPixel
  put the height of this card of the topStack into tStackHeightPixel
  put revPixelToPt(tStackWidthPixel) into tStackWidth
  put revPixelToPt(tStackHeightPixel) into tStackHeight
  
  put tStackWidth div tPaperWidth into tHorizontalPages
  if tStackWidth mod tPaperWidth is not 0 then
    put tHorizontalPages + 1 into tHorizontalPages
  end if
  put tStackHeight div tPaperHeight into tVerticalPages
  if tStackHeight mod tPaperHeight is not 0 then
    put tVerticalPages + 1 into tVerticalPages
  end if
  put empty into tRectList
  repeat with i = 1 to tVerticalPages
    repeat with j = 1 to tHorizontalPages
      put (j-1) * tPaperWidthPixel into x1
      put (i-1) * tPaperHeightPixel into y1
      put j * tPaperWidthPixel into x2
      put i * tPaperHeightPixel into y2
      if x2 > tStackWidth then
        put tStackWidth into x2
      end if
      if y2 > tStackHeight then
        put tStackHeight into y2
      end if
      put x1 & "," & y1 & " " & x2 & "," & y2 & cr after tRectList
    end repeat
  end repeat
  if the last char of tRectList is cr then
    delete the last char of tRectList
  end if
  return tRectList
end revCalculateNumberPages

function revCalculateNumberPrintCards
  put item 1 of the printGutters into tRowGutter
  put item 2 of the printGutters into tColumnGutter
  put revCalculateEffectivePrintArea() into tPaperSize
  put item 1 of tPaperSize into tPaperWidth
  put item 2 of tPaperSize into tPaperHeight
  put the width of this card of the topStack into tStackWidthPixel
  put the height of this card of the topStack into tStackHeightPixel
  put revPixelToPt(tStackWidthPixel) into tStackWidth
  put revPixelToPt(tStackHeightPixel) into tStackHeight
  
  put 0 into tColumnPt
  put 0 into tRowPt
  put 1 into tColumnCount
  put 1 into tRowCount
  put tStackWidth into tColumnPt
  repeat until tColumnPt > tPaperWidth
    put tColumnGutter + tStackWidth + tColumnPt into tColumnPt
    put tColumnCount + 1 into tColumnCount
  end repeat
  put tStackHeight into tRowPt
  repeat until tRowPt > tPaperHeight
    put tRowGutter + tStackHeight + tRowPt into tRowPt
    put tRowCount + 1 into tRowCount
  end repeat
  if tRowCount > 1 then
    put tRowCount - 1 into tRowCount
  end if
  if tColumnCount > 1 then
    put tColumnCount - 1 into tColumnCount
  end if
  put tRowCount * tColumnCount into tResult
  return tResult
end revCalculateNumberPrintCards

function revCalculateEffectivePrintArea
  put item 1 of the printMargins into tLeft
  put item 2 of the printMargins into tTop
  put item 3 of the printMargins into tRight
  put item 4 of the printMargins into tBottom
  put item 1 of the printPaperSize into tWidth
  put item 2 of the printPaperSize into tHeight
  put 0 into tHeaderHeight
  put 0 into tFooterHeight
  put the cREVReport["header"] of this card of the topStack into tHeader
  put the cREVReport["footer"] of this card of the topStack into tFooter
  if (tHeader is not empty) and (there is a stack tHeader) then
    put the height of stack tHeader into tHeaderHeight
  end if
  if (tFooter is not empty) and (there is a stack tFooter) then
    put the height of stack tFooter into tFooterHeight
  end if

  put tHeight - tTop - tBottom - tHeaderHeight - tFooterHeight into tEffectiveHeight
  put tWidth - tLeft - tRight into tEffectiveWidth
  put tEffectiveWidth & "," & tEffectiveHeight into tResult
  return tResult
end revCalculateEffectivePrintArea


on revMenuNavigation pCardNumber
  put revReportParseRangeList(pCardNumber) into tCard
  revMoveToCard tCard
end revMenuNavigation

on revInitialReportUpdate
  put item 1 of the printMargins into tLeft
  put item 2 of the printMargins into tTop
  put item 3 of the printMargins into tRight
  put item 4 of the printMargins into tBottom
  put item 1 of the printGutters into tVertical
  put item 2 of the printGutters into tHorizontal
  
  if the cREVReport["papertopmargin"] of this card of the topStack is empty then
    set the cREVReport["papertopmargin"] of this card of the topStack to tTop
  end if
  if the cREVReport["paperbottommargin"] of this card of the topStack is empty then
    set the cREVReport["paperbottommargin"] of this card of the topStack to tBottom
  end if
  if the cREVReport["paperrightmargin"] of this card of the topStack is empty then
    set the cREVReport["paperrightmargin"] of this card of the topStack to tRight
  end if
  if the cREVReport["paperleftmargin"] of this card of the topStack is empty then
    set the cREVReport["paperleftmargin"] of this card of the topStack to tLeft
  end if
  if the cREVReport["paperhorizontalmargin"] of this card of the topStack is empty then
    set the cREVReport["paperhorizontalmargin"] of this card of the topStack to tHorizontal
  end if
  if the cREVReport["paperverticalmargin"] of this card of the topStack is empty then
    set the cREVReport["paperverticalmargin"] of this card of the topStack to tVertical
  end if
  if the cREVReport["printrange"] of this card of the topStack is empty then
    set the cREVReport["printrange"] of this card of the topStack to "All Cards"
  end if
  
  if the cREVReport["onetoone"] of this card of the topStack is empty then
    set the cREVReport["onetoone"] of this card of the topStack to true
  end if
  revUpdateViewerObjectList
end revInitialReportUpdate

function revQueryReports pStack
  if pStack is empty then put short name of the topStack into pStack
  put the number of controls of this card of stack pStack into tNumControls
  repeat with i = 1 to tNumControls
    put the cREVReport["viewername"] of control i of this card of stack pStack into tReport
    if tReport is not empty then put tReport & cr after tReports
  end repeat
  delete last char of tReports
  sort tReports
  return tReports
end revQueryReports
