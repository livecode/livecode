script "revidelibrary"

on revLoadLibrary
	insert the script of me into back
end revLoadLibrary

on revUnloadLibrary
	remove the script of me from back
end revUnloadLibrary

#############
# Internal functions
#############
# Reads all the of the extension files into an internal array which is then used by all revIDEExtension and revIDEWidget
# type functions

local sExtensionData
local sExtensionDataForDisplay

private on __loadExtensions
   local tInactiveExtensionFolder, tExtensionData
   
   put empty into sExtensionData
   put empty into sExtensionDataForDisplay
   
   revIDEStoreDefaultFolder
   
   ## Load installed extensions
   set the folder to revIDESpecialFolderPath("extensions")
   repeat for each line tFile in the files
      if char 1 of tFile is "." then next repeat
      __loadExtension tFile
   end repeat   
   
   revIDERestoreDefaultFolder
   dispatch "ideExtensionsChanged"
end __loadExtensions

private command __loadExtension pExtensionFile
   local tExtensionID, tExtensionPackageData, tIndex, tKeys
   local tExtractedExtensionFolder
   
   put char 1 to -5 of pExtensionFile into tExtensionID
   put revIDESpecialFolderPath("extensions") into tExtractedExtensionFolder
   
   ## Check if already extracted
   if there is not a folder (tExtractedExtensionFolder & "/" & pExtensionFile) then
      __extractExtension(revIDESpecialFolderPath("extensions") & "/" & pExtensionFile)
   end if
   
   # Try to load the extension
   local tExecutableModuleFile
   put revIDESpecialFolderPath("extensions") & slash & tExtensionID & slash & "module.lcm" into tExecutableModuleFile
   if there is a file tExecutableModuleFile then
      load extension tExecutableModuleFile
      if the result is not empty then
         __revIDEError("loading extension failed: " && the result)
         return empty
      end if
   end if
   
   ## Check if already loaded   
   if tExtensionID is not among the lines of the keys of sExtensionData then
      put the keys of sExtensionDataForDisplay into tKeys
      replace return with comma in tKeys
      put max(tKeys) + 1 into tIndex
      
      ## Extract the extension
      --put __extractExtension(revIDESpecialFolderPath("extensions") & "/" & pExtensionFile) into tExtensionPackageData
      put __extensionData(tExtensionID) into tExtensionPackageData
      
      local tExtenstionTypeID
      put tExtensionPackageData["name"] into tExtenstionTypeID
      put tExtensionPackageData into sExtensionData[tExtenstionTypeID]
      put tExtensionPackageData into sExtensionDataForDisplay[tIndex]
      put "active" into sExtensionData[tExtenstionTypeID]["status"]
      put "active" into sExtensionDataForDisplay[tIndex]["status"]
   end if
end __loadExtension

private function __extensionData pExtensionID
   local tExtractedExtensionFolder, tExtensionFolder, tManifest, tXMLTree, tExtensionData
   local tIsWidget, tIsLibrary, tRequiredNodes
   
   put revIDESpecialFolderPath("extensions") into tExtractedExtensionFolder
   put tExtractedExtensionFolder & "/" & pExtensionID into tExtensionFolder
   
   if there is not a folder tExtensionFolder then return empty
   
   ## Manifest
   put url ("file:" & tExtensionFolder & "/manifest.xml") into tManifest
   if tManifest is empty then return empty
   
   put revXMLCreateTree(tManifest,true,true,false) into tXMLTree
   put revXMLNodeContents(tXMLTree,"/package/name") into tExtensionData["name"]
   put revXMLNodeContents(tXMLTree,"/package/title") into tExtensionData["label"]
   put revXMLNodeContents(tXMLTree,"/package/description") into tExtensionData["description"]
   put revXMLNodeContents(tXMLTree,"/package/author") into tExtensionData["author"]
   put revXMLNodeContents(tXMLTree,"/package/version") into tExtensionData["version"]
   put revXMLNodeContents(tXMLTree,"/package/license") into tExtensionData["license"]
   
   if item 1 of tExtensionData["author"] is "xmlerr" then
      put empty into tExtensionData["author"]
   end if
   
   ## Type
   put revXMLChildNames(tXMLTree,"package",return,"widget",false) into tIsWidget 
   put revXMLChildNames(tXMLTree,"package",return,"library",false) into tIsLibrary
   
   if tIsWidget is "widget" then put "widget" into tExtensionData["type"]
   else if tIsLibrary is "library" then put "library" into tExtensionData["type"]
   else put "widget" into tExtensionData["type"]
   
   ## Requirements
   put revXMLChildNames(tXMLTree,"package",return,"requires",true) into tRequiredNodes
   repeat with x = 1 to the number of lines in tRequiredNodes
      put revXMLAttribute(tXMLTree,"package" & "/" & line x of tRequiredNodes,"name") into tExtensionData["requires"][x]["name"]
      put revXMLAttribute(tXMLTree,"package" & "/" & line x of tRequiredNodes,"version") into tExtensionData["requires"][x]["version"]
   end repeat
   
   revXMLDeleteTree tXMLTree
   
   put (tExtensionFolder & "/manifest.xml") into tExtensionData["manifest_path"]
   
   ## Icons
   if there is a file (tExtensionFolder & "/icon.png") then put (tExtensionFolder & "/icon.png") into tExtensionData["icon"]
   
   ## Guide
   if there is a file (tExtensionFolder & "/guide.md") then put (tExtensionFolder & "/guide.md") into tExtensionData["guide"]
   
   ## API
   if there is a file (tExtensionFolder & "/api.lcdoc") then put (tExtensionFolder & "/api.lcdoc") into tExtensionData["api"]
   return tExtensionData
end __extensionData

private command __extractExtension pFilename
   local tManifest, tExtensionData, tIcon, tFileRoot, tIdentifier, tXMLTree, tRequiredNodes, tType
   local tIsWidget, tIsLibrary, tItems
   
   if char -3 to -1 of pFilename is not "lce" then return __revIDEError("The file" && pFilename && "is not a valid LiveCode Extension file.")
   
   local tNewFolderName
   set the itemDel to "/"
   put char 1 to -5 of item -1 of pFilename into tNewFolderName
   revZipOpenArchive pFilename, "read"
   
   local tZipItems
   put revZipEnumerateItems(pFilename) into tZipItems
   if the last char of line 1 of tZipItems is "/" then
      put line 1 of tZipItems into tFileRoot
   else
      put empty into tFileRoot
   end if
   
   ## Extract Extension files
   local tExtensionsFolder, tUnzippedExtensionFolder, tIconFile, tLargeIconFile
   put revIDESpecialFolderPath("extensions") into tExtensionsFolder
   put tExtensionsFolder & "/" & tNewFolderName into tUnzippedExtensionFolder
   
   if there is not a folder tUnzippedExtensionFolder then
      revIDEEnsurePath tUnzippedExtensionFolder
   end if
   
   ## Manifest
   local tManifestPath
   put tUnzippedExtensionFolder & "/manifest.xml" into tManifestPath
   revZipExtractItemToFile pFilename, (tFileRoot & "manifest.xml"), tManifestPath
   
   ## Icon files
   put tUnzippedExtensionFolder & "/icon.png" into tIconFile
   revZipExtractItemToFile pFilename, (tFileRoot & "support/icon.png"), tIconFile
   
   ## Extract any other icon files
   local tUnzippedIconFile
   put revZipEnumerateItems(pFilename) into tItems
   filter tItems with "*icon*"
   repeat for each line tIcon in tItems
      put tUnzippedExtensionFolder & "/" & item -1 tIcon into tUnzippedIconFile
      revZipExtractItemToFile pFilename, tIcon, tUnzippedIconFile
   end repeat 
   
   ## Documentaion
   
   ## Guide
   local tGuidePath
   put tUnzippedExtensionFolder & "/guide.md" into tGuidePath
   revZipExtractItemToFile pFilename, (tFileRoot & "docs/guide/guide.md"), tGuidePath
   ##-- Rebuild user guide JSON
   
   ## API
   local tAPIPath
   put tUnzippedExtensionFolder & "/api.lcdoc" into tAPIPath
   revZipExtractItemToFile pFilename, (tFileRoot & "docs/api/api.lcdoc"), tAPIPath
   --   put tAPIPath into tExtensionData["api"]
   
   ## Executable Module File
   revZipExtractItemToFile pFilename, (tFileRoot & "module.lcm"), tUnzippedExtensionFolder & "/module.lcm"
   
   revZipCloseArchive pFilename
end __extractExtension

private function __trimZippedData pString
   if codepointToNum(the last char of pString) < 32 then
      delete the last char of pString
   end if
   return pString
end __trimZippedData

# Reads in all the tool data
local sToolDefinition
private on __toolDefinitionsRead
   revIDEStoreDefaultFolder
   
   put empty into sToolDefinition
   
   # Set to folder to the path where the object definition files for classic controls are stored
   local tToolDefinitionPath
   put revIDESpecialFolderPath("Tool Definitions") into tToolDefinitionPath
   set the defaultfolder to tToolDefinitionPath
   
   # Read in each file and build an array
   local tDefinitionData, tToolSection
   repeat for each line tFile in the files
      if char 1 of tFile is "." then next repeat
      set the itemdel to "."
      put item 1 of tFile into tToolSection
      put URL ("file:" & tToolDefinitionPath & slash & tFile) into tDefinitionData
      
      local tToolID, tCount
      put 1 into tCount
      set the itemdel to ","
      repeat for each line tToolDescription in tDefinitionData
         put item 1 of tToolDescription into tToolID
         put item 2 of tToolDescription into sToolDefinition[tToolSection][tCount]["label"]
         put item 3 of tToolDescription into sToolDefinition[tToolSection][tCount]["icon"]
         
         local tCount2, tPropertyName, tPropertyValue
         put 1 into tCount2
         repeat for each item tProperty in item 4 of tToolDescription
            set the itemdel to ":"
            put item 1 of tProperty into tPropertyName
            put item 2 of tProperty into tPropertyValue
            put tPropertyValue into sToolDefinition[tToolSection][tCount]["properties"][tCount2][tPropertyName]
            add 1 to tCount2
         end repeat
         
         add 1 to tCount
      end repeat
   end repeat
   
   revIDERestoreDefaultFolder
end __toolDefinitionsRead

# Reads in the basic property data for classic controls
local sClassicObjectProperties, sClassicPropertyNames
private on __objectPropertiesRead
   --put "reading object properties" after msg
   revIDEStoreDefaultFolder
   
   put empty into sClassicObjectProperties
   put empty into sClassicPropertyNames
   
   # Set to folder to the path where the object definition files for classic controls are stored
   local tObjectDefinitionsPath
   put revIDESpecialFolderPath("Object Property Definitions") into tObjectDefinitionsPath
   set the defaultfolder to tObjectDefinitionsPath
   
   # Read in each file and build an array
   local tObjectData, tObjectType
   
   repeat for each line tFile in the files
      if char 1 of tFile is "." then next repeat
      
      # Get the name of the object
      set the itemdel to "."
      put tFile into tObjectType
      if the last item of tObjectType is "txt" then delete the last item of tObjectType
      if tObjectType is empty then next repeat
      
      # Get the data from the file
      put URL ("file:" & tObjectDefinitionsPath & slash & tFile) into tObjectData
      
      # Parse the file into a structure array
      local tObjectProperties, tPropertyName, tCount
      set the itemdel to tab
      put 1 into tCount
      repeat for each line tLine in tObjectData
         if tLine is empty then next repeat
         
         put item 1 of tLine into tPropertyName
         
         if tPropertyName is "type" then
            //if tFile contains "lineGraphic" then breakpoint
            //local tTest
            //put item 2 of tLine into tTest
            put item 2 of tLine into tObjectProperties["type"]
         else
            put tCount into tObjectProperties["properties"][tPropertyName]["order"]
            put item 2 of tLine into tObjectProperties["properties"][tPropertyName]["label"]
            put item 3 of tLine into tObjectProperties["properties"][tPropertyName]["type"]
            put item 4 of tLine into tObjectProperties["properties"][tPropertyName]["group"]
            put __unescapeString(item 5 of tLine) into tObjectProperties["properties"][tPropertyName]["default"] 
            put item 6 of tLine into tObjectProperties["properties"][tPropertyName]["options"] 
            if item 7 of tLine is false then put false into tObjectProperties["properties"][tPropertyName]["user_visible"] 
            else put true into tObjectProperties["properties"][tPropertyName]["user_visible"] 
            if item 8 of tLine is true then put true into tObjectProperties["properties"][tPropertyName]["read_only"] 
            else put false into tObjectProperties["properties"][tPropertyName]["read_only"] 
            put item 9 of tLine into tObjectProperties["properties"][tPropertyName]["related"]
         end if
         add 1 to tCount
      end repeat
      
      put tObjectProperties into sClassicObjectProperties[tObjectType]
      put empty into tObjectProperties
   end repeat
   
   # Generate a list of all the property names
   put empty into sClassicPropertyNames
   repeat for each element tObjectType in sClassicObjectProperties
      repeat for each line tProperty in the keys of tObjectType["properties"]
         if tProperty is not among the lines of sClassicPropertyNames then
            put tProperty & return after sClassicPropertyNames
         end if
      end repeat
   end repeat
   delete the last char of sClassicPropertyNames
   
   revIDERestoreDefaultFolder
end __objectPropertiesRead

private function __objectPropertiesShared pObjectTypeList
   local tPropertiesArray,tPropertiesArray2, tPropertiesArray3,x
   put 1 into x
   repeat for each line tObjectType in pObjectTypeList
      if tObjectType begins with "com.livecode.interface.classic" then
         # Classic control
         if x is 1 then
            put sClassicObjectProperties[tObjectType]["properties"] into tPropertiesArray
         else
            put sClassicObjectProperties[tObjectType]["properties"] into tPropertiesArray2
            intersect tPropertiesArray with tPropertiesArray2
         end if
      else
         # Widget
         if x is 1 then
            put revIDEExtensionProperties(tObjectType) into tPropertiesArray
            put sClassicObjectProperties["com.livecode.interface.classic.widget"]["properties"] into tPropertiesArray2
            
            union tPropertiesArray with tPropertiesArray2
         else
            put revIDEExtensionProperties(tObjectType) into tPropertiesArray2
            put sClassicObjectProperties["com.livecode.interface.classic.widget"]["properties"] into tPropertiesArray3
            union tPropertiesArray2 with tPropertiesArray3
            
            intersect tPropertiesArray with tPropertiesArray2
         end if
      end if
      add 1 to x
   end repeat
   
   return tPropertiesArray
end __objectPropertiesShared

private function __objectTypes pObjectIDs
   local tObjectTypeList, tObjectType
   repeat for each line tObjectID in pObjectIDs
      put __objectType(tObjectID) into tObjectType
      if tObjectType is not among the lines of tObjectTypeList then
         put tObjectType & return after tObjectTypeList
      end if
   end repeat
   delete the last char of tObjectTypeList
   return tObjectTypeList
end __objectTypes

private function __objectType pObjectID
   switch word 1 of the name of pObjectID
      case "stack"
         return "com.livecode.interface.classic.Stack"
      case "card"
         return "com.livecode.interface.classic.Card"
      case "group"
         if the dgProps["control type"] of pObjectID is "Data Grid" then
            return "com.livecode.interface.classic.DataGrid"
         else
            return "com.livecode.interface.classic.Group"
         end if
         break
      case "field"
         if the tabStops of pObjectID is not empty then
            return "com.livecode.interface.classic.TableField"
         else if the listbehavior of pObjectID is true then
            return "com.livecode.interface.classic.ListField"
         else if the vscrollbar of pObjectID is true then
            return "com.livecode.interface.classic.TextArea"
         else
            return "com.livecode.interface.classic.Field"
         end if
         break
      case "button"
         if the menumode of pObjectID is "combobox" then return "com.livecode.interface.classic.ComboBox" 
         
         switch the style of pObjectID
            case "radiobutton"
               return "com.livecode.interface.classic.RadioButton"
               break
            case "checkbox" 
               return "com.livecode.interface.classic.Checkbox"
               break
            case "menu" 
               switch the menumode of pObjectID 
                  case "combobox" 
                     return "com.livecode.interface.classic.ComboBox" 
                     break
                  case "option"
                     return "com.livecode.interface.classic.OptionMenu" 
                     break
                  case "pulldown"
                     return "com.livecode.interface.classic.PulldownMenu" 
                     break
                  case "popup"
                     return "com.livecode.interface.classic.PopupMenu" 
                     break
                  default
                     return "com.livecode.interface.classic.TabPanel"
                     break
               end switch
            case "rectangle" 
               return "com.livecode.interface.classic.RectangleButton"
               break
            default 
               if the default of pObjectID is true then
                  return "com.livecode.interface.classic.DefaultButton"
               else
                  return "com.livecode.interface.classic.Button"
               end if
               break
         end switch
      case "scrollbar"
         switch the style of pObjectID
            case "scale"
               return "com.livecode.interface.classic.Slider"
               break
            case "progress"
               return "com.livecode.interface.classic.ProgressBar"
               break
            case "scrollbar"
               if the height of pObjectID > the width of pObjectID and the height of pObjectID < 54 then
                  return "com.livecode.interface.classic.LittleArrows"
               else
                  return "com.livecode.interface.classic.Scrollbar"
               end if
               break
         end switch
         break
      case "image"
         return "com.livecode.interface.classic.Image"
         break
      case "player"
         return "com.livecode.interface.classic.Player"
      case "widget"
         return the kind of pObjectID
         break
      case "graphic"
         switch the style of pObjectID
            case "rectangle"
               return "com.livecode.interface.classic.RectangleGraphic"
               break
            case "roundrect"
               return "com.livecode.interface.classic.RoundRectGraphic"
               break
            case "line"
               return "com.livecode.interface.classic.LineGraphic"
               break
            case "oval"
               return "com.livecode.interface.classic.OvalGraphic"
               break
            case "curve"
               return "com.livecode.interface.classic.CurveGraphic"
               break
            case "polygon"
               return "com.livecode.interface.classic.PolygonGraphic"
               break
            case "regular"
               return "com.livecode.interface.classic.RegularGraphic"
               break
         end switch
         break
   end switch
end __objectType

private function __dumpArrayRecurse pArray, pLevel, @pCounter, @pList
   local tOrderedKeys
   put the keys of pArray into tOrderedKeys
   sort lines of tOrderedKeys numeric
   repeat for each line tKey in tOrderedKeys
      
      add 1 to pCounter
      
      repeat for pLevel
         put tab after line pCounter of pList
      end repeat
      put (tKey && "= ") after line pCounter of pList
      
      if (the keys of pArray[tKey]) is empty then
         put line 1 of pArray[tKey] after line pCounter of pList
      else
         put __dumpArrayRecurse(pArray[tKey], (pLevel + 1), pCounter, pList) into pList
      end if
      
   end repeat
   
   return pList
end __dumpArrayRecurse

function __revIDEError pMessage
   local tErrorContext
   put line -2 of the executioncontexts into tErrorContext
   put "IDE ERROR:" && tErrorContext && ":" && pMessage & return after msg
   return "ERROR:" && pMessage & return & "line:" && item 3 of tErrorContext & return & "handler:" && item 2 of tErrorContext & return & "script object:" && item 1 of tErrorContext
end __revIDEError


local sControlPropertiesToRead, sCardPropertiesToRead, sStackPropertiesToRead

function __readStructureOfStackForDataView @pData, pParent, pLevel, pIndex
   local tStackIndex
   # 1) Get desired properties for the current stack
   dispatch function "__readPropertiesOfControlForDataView" to the target with sStackPropertiesToRead, pData,pParent, pLevel, pIndex
   put the result into tStackIndex
   
   --   # 2) Process any cards
   --   local tChildCardIDs
   --   put empty into tChildCardIDs
   --   repeat with x = 1 to the number of cards of the target
   --      # a) Get basic properties for the card
   --      dispatch function "__readPropertiesOfControlForDataView" to card x of the target with sCardPropertiesToRead, pData, tStackIndex, pLevel + 1
   --      local tCardIndex
   --      put the result into tCardIndex
   
   --      if tChildCardIDs is empty then 
   --         put tCardIndex into tChildCardIDs
   --      else
   --         put comma & tCardIndex after tChildCardIDs
   --      end if
   
   --      # b) Process the card container for all its controls
   
   --      dispatch function "__readStructureOfContainerForDataView" to card x of the target with pData, tCardIndex, pLevel + 1
   --      put the result into pData[tCardIndex]["children"]
   --      put false into pData[tCardIndex]["expanded"]
   --      put "container" into pData[tCardIndex]["style"]
   --      put "card" into pData[tCardIndex]["type"]
   --   end repeat
   
   --   # 3) Process any subsctacks
   --   repeat for each line tSubstack in the substacks of the target
   --      if tChildCardIDs is empty then 
   --         put the number of elements of pData + 1 into tChildCardIDs
   --      else
   --         put comma & the number of elements of pData + 1 after tChildCardIDs
   --      end if
   --      dispatch function "__readStructureOfStackForDataView" to stack tSubstack with pData,tStackIndex, pLevel + 1
   --   end repeat
   
   --   put tChildCardIDs into pData[tStackIndex]["children"]
   
   --put the number of cards of the target into pData[tStackIndex]["childCount"]
   put the number of cards of the target & the number of lines in the substacks of the target into pData[tStackIndex]["childCount"]
   put false into pData[tStackIndex]["expanded"]
   put "container" into pData[tStackIndex]["style"]
   put "stack" into pData[tStackIndex]["type"]
end __readStructureOfStackForDataView


--function __readStructureOfStackForDataView @pData, pParent, pLevel 
--   # 1) Get desired properties for the current stack
--   dispatch function "__readPropertiesOfControlForDataView" to the target with sStackPropertiesToRead, pData,pParent, pLevel
--   local tStackIndex
--   put the result into tStackIndex

--   # 2) Process any cards
--   local tChildCardIDs
--   put empty into tChildCardIDs
--   repeat with x = 1 to the number of cards of the target
--      # a) Get basic properties for the card
--      dispatch function "__readPropertiesOfControlForDataView" to card x of the target with sCardPropertiesToRead, pData, tStackIndex, pLevel + 1
--      local tCardIndex
--      put the result into tCardIndex

--      if tChildCardIDs is empty then 
--         put tCardIndex into tChildCardIDs
--      else
--         put comma & tCardIndex after tChildCardIDs
--      end if

--      # b) Process the card container for all its controls

--      dispatch function "__readStructureOfContainerForDataView" to card x of the target with pData, tCardIndex, pLevel + 1
--      put the result into pData[tCardIndex]["children"]
--      put false into pData[tCardIndex]["expanded"]
--      put "container" into pData[tCardIndex]["style"]
--      put "card" into pData[tCardIndex]["type"]
--   end repeat

--   # 3) Process any subsctacks
--   repeat for each line tSubstack in the substacks of the target
--      if tChildCardIDs is empty then 
--         put the number of elements of pData + 1 into tChildCardIDs
--      else
--         put comma & the number of elements of pData + 1 after tChildCardIDs
--      end if
--      dispatch function "__readStructureOfStackForDataView" to stack tSubstack with pData,tStackIndex, pLevel + 1
--   end repeat

--   put tChildCardIDs into pData[tStackIndex]["children"]
--   put false into pData[tStackIndex]["expanded"]
--   put "container" into pData[tStackIndex]["style"]
--   put "stack" into pData[tStackIndex]["type"]
--end __readStructureOfStackForDataView

function __readStructureOfContainerForDataView @pData, pParentIndex, pLevel
   # 1) Repeat for each control in the container
   local tChildIDs, tChildCount
      
   //repeat with x = 1 to the number of controls of the target
   --repeat for each line tControlId in the childcontrolIDs of the target
   put the number of lines in the childcontrolIDs of the target into tChildCount
   repeat with x = 1 to the number of lines in the childcontrolIDs of the target
      local tControlId
      if revIDEGetPreference("ideProjectBrowser_sortOrder") is "bottom to top" then
         put line x of the childcontrolIDs of the target into tControlId
      else
         put line (tChildCount -x) + 1 of the childcontrolIDs of the target into tControlId
      end if 
      
      # read the properties of the control
      local tIndex
      dispatch function "__readPropertiesOfControlForDataView" to control ID tControlID of the target with sControlPropertiesToRead, pData, pParentIndex, pLevel + 1
      put the result into tIndex
      if tChildIDs is empty then 
         put tIndex into tChildIDs
      else
         put comma & tIndex after tChildIDs
      end if
      
      #  if the control is a group
      if the name of control ID tControlID of the target begins with "group" then
         local tIndexStart
         put the number of elements of pData into tIndexStart
         
         dispatch function "__readStructureOfContainerForDataView" to control ID tControlID of the target with pData, tIndex, pLevel + 1
         
         put the result into pData[tIndex]["children"]
         --add the number of elements of pData - tIndexStart to x
         put false into pData[tIndex]["expanded"]
         put "group" into pData[tIndex]["style"]
      else
         put "control" into pData[tIndex]["style"]
      end if
   end repeat
   return tChildIDs
end __readStructureOfContainerForDataView

# Used internall but not private. This function is dispatched to the object and reads the properties 
# In the list. Sending it to the object allows the use of the target which reduces the lookup time
# for each of the properties
function __readPropertiesOfControlForDataView pList, @pData, pParentIndex, pLevel,pIndex
   local tIndex
   put pIndex + the number of elements of pData + 1 into tIndex
   
   put pParentIndex into pData[tIndex]["parent"]
   --put empty into pList
   repeat for each line tProp in pList
      try
         switch tProp
            case "short name"
               put the short name of the target into pData[tIndex][tProp]
               break
            case "scriptlines"
               put the number of lines of the script of the target into pData[tIndex][tProp]
               break
            case "behavior scriptlines"
               put the number of lines of the script of the behavior of the target into pData[tIndex][tProp]
               break
            case "long id"
               put the long id of the target into pData[tIndex][tProp]
               break
            case "owner"
               put the long id of the owner of the target into pData[tIndex][tProp]
               break
            case "type"
               put word 1 of the name of the target into pData[tIndex]["type"]
               break
            default
               put the tProp of the Target into pData[tIndex][tProp]
               break
         end switch
         put the short name of the target into pData[tIndex]["name"]
         put false into pData[tIndex]["expanded"]
         put pLevel into pData[tIndex]["level"]
      end try
   end repeat
   return tIndex
end __readPropertiesOfControlForDataView

function __readStructureOfStack @pData 
   # 1) Get desired properties for the current stack
   dispatch function "__readPropertiesOfControl" to the target with sStackPropertiesToRead, pData
   
   # 2) Process any cards
   repeat with x = 1 to the number of cards of the target
      # a) Get basic properties for the card
      dispatch function "__readPropertiesOfControl" to card x of the target with sCardPropertiesToRead, pData["cards"][x]
      
      # b) Process the card container for all its controls
      dispatch function "__readStructureOfContainer" to card x of the target with pData["cards"][x]["controls"]
   end repeat
   
   # 3) Process any subsctacks
   repeat for each line tSubstack in the substacks of the target
      dispatch function "__readStructureOfStack" to stack tSubstack with pData["substacks"][x]
   end repeat
end __readStructureOfStack

function __readStructureOfContainer @pData
   local tControlsProcessedInContainer
   
   # 1) Repeat for each control in the container
   repeat with x = 1 to the number of controls of the target
      # read the properties of the control
      dispatch function "__readPropertiesOfControl" to control x of the target with sControlPropertiesToRead, pData[x]
      
      --local tName
      --put the name of control x of the target into tName
      
      #  if the control is a group
      if the name of control x of the target begins with "group" then
         
         dispatch function "__readStructureOfContainer" to control x of the target with pData[x]["controls"]
         put the result into tControlsProcessedInContainer 
         add the number of elements of pData[x]["controls"] to tControlsProcessedInContainer
         add tControlsProcessedInContainer to x
      end if
   end repeat
   
   return tControlsProcessedInContainer
end __readStructureOfContainer

# Used internall but not private. This function is dispatched to the object and reads the properties 
# In the list. Sending it to the object allows the use of the target which reduces the lookup time
# for each of the properties
function __readPropertiesOfControl pList, @pData
   local tError
   
   repeat for each line tProp in pList      
      put empty into tError
      try
         switch tProp
            case "short name"
               put the short name of the target into pData[tProp]
               break
            case "scriptlines"
               put the number of lines of the script of the target into pData[tProp]
               break
            case "behavior scriptlines"
               put the number of lines of the script of the behavior of the target into pData[tProp]
               break
            case "long id"
               put the long id of the target into pData[tProp]
               break
            case "owner"
               put the long id of the owner of the target into pData[tProp]
               break
            case "type"
               put word 1 of the name of the target into pData["type"]
               break
            case "customProperties"
               put __customPropertyData(the long id of the target) into pData["customProperties"]
               break
            case "dropShadow"
            case "innerShadow"	
            case "outerGlow"
            case "innerGlow"
            case "colorOverlay"
               put the tProp of the Target into pData[tProp]
               if pData[tProp] is empty then
                  put empty into pData[tProp]["color"]
                  put empty into pData[tProp]["blendMode"]
                  put empty into pData[tProp]["opacity"]
                  put empty into pData[tProp]["filter"]
                  put empty into pData[tProp]["spread"]
                  put empty into pData[tProp]["size"]
                  put empty into pData[tProp]["distance"]
                  put empty into pData[tProp]["angle"]
               end if
               break
            default
               put the tProp of the Target into pData[tProp]
               break
         end switch
         put the short name of the target into pData["name"]
      catch tError 
      end try
      
      ## Temporary solution for multiprops such as fill
      local tPropertyName, tPropertyList, tSubPropName
      try
         if tProp contains "{" then
            set the itemDel to "{"
            put tProp into tPropertyName
            put item 2 of tProp into tPropertyList
            if char 1 of tPropertyList is "{" then delete char 1 of tPropertyList
            if char -1 of tPropertyList is "}" then delete char -1 of tPropertyList
            set the itemDel to comma
            repeat for each item tWord in tPropertyList
               set the itemDel to ":"
               put  item 2 of tWord into tSubPropName
               put tSubPropName into pData[tPropertyName][item 1 of tWord]["name"]
               put the tSubPropName of the target into pData[tPropertyName][item 1 of tWord]["value"]
               set the itemDel to comma
            end repeat
         end if
      catch tError 
      end try
      
      if tError is not empty then
         try
            do ("put" && tProp && "of" && the target && "into pData[tProp]")
         catch tError
         end try
      end if
      
      if pData[tProp] is empty then
         try
            local tEffective
            do ("put effective" && tProp && "of" && the target && "into tEffective")
            if tEffective is not empty then
               put tEffective into pData["effective" && tProp]
            end if
         catch tError
         end try
      end if
   end repeat
end __readPropertiesOfControl

private function __readControlPropertiesForDataView pList  
   local tData
   repeat for each line tProp in pList
      try
         switch tProp
            case "short name"
               put the short name of the target into tData[tProp]
               break
            case "scriptlines"
               put the number of lines of the script of the target into tData[tProp]
               break
            case "behavior scriptlines"
               put the number of lines of the script of the behavior of the target into tData[tProp]
               break
            case "long id"
               put the long id of the target into tData[tProp]
               break
            case "owner"
               put the long owner of the target into tData[tProp]
               break
            case "type"
               put word 1 of the name of the target into tData["type"]
               break
            default
               put the tProp of the Target into tData[tProp]
               break
         end switch
      end try
   end repeat
   put the short name of the target into tData["name"]
   return tData
end __readControlPropertiesForDataView

function __customPropertyData pLongID
   local tArray, tSets, tProps
   
   put the customPropertySets of pLongID into tSets
   if tSets is empty then
      put the customProperties of pLongID into tArray--["customKeys"]
   else
      set the customPropertySet of pLongID to empty
      put the customProperties of pLongID into tArray--["customKeys"]
      
      repeat for each line tSet in tSets
         set the customPropertySet of pLongID to tSet
         put the customProperties of pLongID into tArray[tSet]
      end repeat
      set the customPropertySet of pLongID to empty
   end if
   return tArray
end __customPropertyData

function __unescapeString pString
   --if pString contains "\n" then breakpoint
   replace "\n" with return in pString
   return pString
end __unescapeString

on revIDEOpenPropertyInspectorControllerStacks
   lock screen
   revIDEStoreDefaultFolder
   
   local tControllersFolder
   put revIDESpecialFolderPath("Toolset") & slash & "propertyInspectorControllers" into tControllersFolder
   set the defaultfolder to tControllersFolder
   
      repeat for each line tControllerStackFile in the files
         if char 1 of tControllerStackFile is "." then next repeat
   --      //put (tControllersFolder & slash & tControllerStackFile) & return after msg
   --      if there is a stack (char 1 to -5 of tControllerStack) then next repeat
         if there is a file (tControllersFolder & slash & tControllerStackFile) then	
		lock messages
           go invisible stack (tControllersFolder & slash & tControllerStackFile)
		unlock messages
         end if
     end repeat
   
   revIDERestoreDefaultFolder
   unlock screen
end revIDEOpenPropertyInspectorControllerStacks

on revIDEInitialiseIDELibrary
   # Load fonts
   start using font file (revIDESpecialFolderPath("fonts") & slash & "fontawesome.ttf")
   
   # Setup the default lists of properties to read from objects when reading the structure of a stack
   put "name" & return & "visible" & return & "cantselect" & return & "layer" & return & "long id" & return & "label" & return & "behavior" & return & "scriptlines" & return & "short name" & return & "owner" & return & "behavior" & return & "behavior scriptlines" & return & "type" into sControlPropertiesToRead
   put "scriptlines" & return & "short name" & return & "owner" & return & "behavior" & return & "behavior scriptlines" & return & "long id" into sCardPropertiesToRead
   put "scriptlines" & return & "short name" & return & "behavior" & return & "behavior scriptlines" & return & "long id" into sStackPropertiesToRead
   
   ## Load property inspector controller stacks
   --revIDEOpenPropertyInspectorControllerStacks
   
   ## Load Extensions
   revIDEInitialiseExtensions
   
   ## Regenerate dictionary data
   revDocsRegenerateBuiltDictionaryData

end revIDEInitialiseIDELibrary

#############
# Subscription
#############
# Stores subscriptions in the following format
# sSubscriptions[message][callback targets] = List of object ID's
local sSubscriptions
/*
Subscribes the calling object to receive the specified IDE message

pMessage (enum): The message to subscribe to:
- ideSelectedObjectsChanged: Sent when the selected object(s) changes. When a user clicks and drags to select objects, this message is NOT sent. Instead, it waits until the drag action ends.
- ideObjectSelectionStarted: Sent when a user starts to selected items in edit mode using a click and drag.

pOptional (string): If subscribing to a message that takes a parameter, specifiy the parameter as a string. For example, when subscribing for idePreferenceChanged, pass the name of the preference you wish to know about

Example:
on preOpenStack
   revIDESubscribe "ideSelectedObjectsChanged"
end preOpenStack

on ideSelectedObjectsChanged
   # Handle the notification
end ideSelectedObjectsChanged
*/
on revIDESubscribe pMessage
   # Validate input
   if pMessage is not among the items of revIDEMessages() and not pMessage begins with "idePreferenceChanged" then return __revIDEError("The message ("&pMessage&") that you are trying to subscribe to is not valid. Please check the documentation for which messages you can subscribe to.")
   
   # Get the log ID of the calling object
   local tCallingObjectID
   put the long ID of the target into tCallingObjectID
   
   if sSubscriptions[pMessage]["callback targets"] is empty then
      put tCallingObjectID into sSubscriptions[pMessage]["callback targets"]
   else if tCallingObjectID is not among the lines of sSubscriptions[pMessage]["callback targets"] then
      put return & tCallingObjectID after sSubscriptions[pMessage]["callback targets"]
   else
      return "Already subscribed"
   end if
end revIDESubscribe

/*
Unsubscribes the calling object from the given message

pMessage (enum): The message to unsubscribe from. Must be a valid message as described in [revIDESubscribe].
pObject (String): The long ID of the object you wish to unsubscribe from the message. This is an optional parameter, if not specified, the calling object will be used.
*/
on revIDEUnsubscribe pMessage, pObject
   # If not object is specified, get the long ID of the calling object
   if pObject is empty then
      put the long ID of the target into pObject
   end if
   
   # Delete the calling object from the message: sSubscriptions[pMessage]["callback targets"]
   if lineoffset(pObject, sSubscriptions[pMessage]["callback targets"]) > 0 then
      delete line lineoffset(pObject, sSubscriptions[pMessage]["callback targets"]) of sSubscriptions[pMessage]["callback targets"]
   end if
end revIDEUnsubscribe

/*
Unsubscribes the object from all messages

pObject (String): The long ID of the object you wish to unsubscribe from all message. This is an optional parameter, if not specified, the calling object will be used.
*/
on revIDEUnsubscribeAll pObject
      # If not object is specified, get the long ID of the calling object
   if pObject is empty then
      put the long ID of the target into pObject
   end if
   
   # Loop through messages and unsubscribe
   repeat for each key tMessage in sSubscriptions
      revIDEUnsubscribe tMessage, pObject
   end repeat
end revIDEUnsubscribeAll

/*
Use to find out which messages the calling object is subscribed to.

returns (string): Return delimited list of messages, 1 per line.
*/
function revIDESubscriptions 
   # Get the log ID of the calling object
   local tCallingObjectID
   put the long ID of the target into tCallingObjectID
   
   # Build a list of all the message names the calling object is subscribed to
   local tSubscriptions
   repeat for each key tMessage in sSubscriptions
      if tCallingObjectID is among the lines of sSubscriptions[tMessage]["callback targets"] then
         put tMessage & return after tSubscriptions
      end if
   end repeat
   delete the last char of tSubscriptions
   
   return tSubscriptions
end revIDESubscriptions

/*
Use to find out which objects are subscribed to a given message

pMessage (String): The name of the message.

returns (string): Return delimited list of object ID's, 1 per line.
*/
function revIDESubscribedObjects pMessage
    # Validate input
   if pMessage is not among the items of revIDEMessages() and not pMessage begins with "idePreferenceChanged" then return __revIDEError("The message ("&pMessage&") you are enquiring about is not valid. Please check the documentation for which messages you can subscribe to.")
   
   return sSubscriptions[pMessage]["callback targets"]
end revIDESubscribedObjects

/*
Use to get a list of all the messages that the IDE allows subscriptions for

related: revIDESubscribedObjects, revIDESubscriptions, revIDEUnsubscribe, revIDESubscribe

returns (string): Return delimited list of message names, 1 per line.
*/
function revIDEMessages
   return "ideExtensionStatusChanged,idePreferenceChanged,ideSelectedObjectChanged,ideObjectSelectionStarted,ideNewCard,ideNewStack,ideNewControl,ideControlDeleted,ideStackDeleted,ideCardDeleted,ideToolChanged,ideLibraryStack,ideReleaseStack,ideFindMoreWidgets,revIDENameChanged"
end revIDEMessages
 
# System function telling the IDE to send 
on revIDEMessageSend pMessage, pEffectedTarget
   --put pMessage, pEffectedTarget
   # Validate input
   --put the milliseconds && pMessage & return & line 1 to 20 of msg into msg
   
   //
   if pMessage is not among the items of revIDEMessages() and not pMessage begins with "idePreferenceChanged" and not pMessage begins with "ideInspectObjects" then return __revIDEError("The message ("&pMessage&") you have requested to be sent is not a valid IDE message. Please check the documentation for which messages you can subscribe to.")
   //if pMessage is "ideExtensionStatusChanged" then breakpoint
   # If in run mode don't send any updates
   //if the tool is not "pointer tool" then exit revIDEMessageSend
   
   repeat for each line tObjectID in sSubscriptions[pMessage]["callback targets"]
      # if the callback object doesn't exist, don't send message
      if not exists(tObjectID) then
         revIDEUnsubscribeAll tObjectID
         next repeat
      end if
      
      # If the stack isn't visible on screen, don't send a message
      local tStackID
      set the itemdel to "stack"
      put tObjectID into tStackID
      delete item 1 of tStackID
      if tStackID begins with "tack" then delete char 1 to 4 of tStackID
      put "stack" before tStackID
      if the mode of tStackID is 0 or the visible of tStackID is false then next repeat
      
      # Send the message!
      set the itemdel to ":"
      local tMessage
      put item 1 of pMessage into tMessage
      
      switch  tMessage
         case "idePreferenceChanged"
            dispatch "idePreferenceChanged" to tObjectID with item 2 of pMessage
            break
         case "ideFindMoreWidgets"
            answer tObjectID
            dispatch pMessage to tObjectID with pEffectedTarget
            break
         case "ideNewStack"
            try
               send pMessage && pEffectedTarget to tObjectID in 10 milliseconds
            catch tError
            end try
            break
         default
            dispatch pMessage to tObjectID with pEffectedTarget
            break
      end switch
      
      --put "SENT" && the milliseconds && pMessage && "sent to" && tObjectID & return & line 1 to 20 of msg into msg
   end repeat
end revIDEMessageSend

#############
# Data
#############
function revIDEGraphics
   if sClassicObjectProperties is empty then __objectPropertiesRead
   
   local tReturnData
   repeat for each key tIndex in sClassicObjectProperties
      if sClassicObjectProperties[tIndex]["type"] is "graphic" and sClassicObjectProperties[tIndex]["properties"]["tool"]["label"] is empty then
         put sClassicObjectProperties[tIndex] into tReturnData[tIndex]
      end if
   end repeat
   return tReturnData
end revIDEGraphics

function revIDEGraphicTools
   if sClassicObjectProperties is empty then __objectPropertiesRead
   
   local tReturnData
   repeat for each key tIndex in sClassicObjectProperties
      if sClassicObjectProperties[tIndex]["type"] is "graphic" and sClassicObjectProperties[tIndex]["properties"]["tool"]["label"] is not empty then
         put sClassicObjectProperties[tIndex] into tReturnData[tIndex]
      end if
   end repeat
   return tReturnData
end revIDEGraphicTools

function revIDEPaintTools
   local tReturnData
   put "tool" into tReturnData["com.livecode.tool.Select"]["type"]
   put "select tool" into tReturnData["com.livecode.tool.Select"]["tool"]
   put "Select Tool - Select a rectangle area of an image" into tReturnData["com.livecode.tool.Select"]["tooltip"]
   put 7 into tReturnData["com.livecode.tool.Select"]["order"]
   
   put "tool" into tReturnData["com.livecode.tool.Bucket"]["type"]
   put "bucket tool" into tReturnData["com.livecode.tool.Bucket"]["tool"]
   put "Bucket Tool - Fills on outline in an image" into tReturnData["com.livecode.tool.Bucket"]["tooltip"]
   put 3 into tReturnData["com.livecode.tool.Bucket"]["order"]
   
   put "tool" into tReturnData["com.livecode.tool.SprayCan"]["type"]
   put "spray tool" into tReturnData["com.livecode.tool.SprayCan"]["tool"]
   put "Spray Can Tool - Draws airbrush strokes on an image" into tReturnData["com.livecode.tool.SprayCan"]["tooltip"]
   put 4 into tReturnData["com.livecode.tool.SprayCan"]["order"]
   
   put "tool" into tReturnData["com.livecode.tool.Eraser"]["type"]
   put "eraser tool" into tReturnData["com.livecode.tool.Eraser"]["tool"]
   put "Eraser Tool - Erases an area in an image" into tReturnData["com.livecode.tool.Eraser"]["tooltip"]
   put 8 into tReturnData["com.livecode.tool.Eraser"]["order"]
   
   put "tool" into tReturnData["com.livecode.tool.Polygon"]["type"]
   put "polygon tool" into tReturnData["com.livecode.tool.Polygon"]["tool"]
   put "Polygon Tool - Click to set each point on the polygon, double click to close the polygon" into tReturnData["com.livecode.tool.Polygon"]["tooltip"]
   put 5 into tReturnData["com.livecode.tool.Polygon"]["order"]
   
   put "tool" into tReturnData["com.livecode.tool.Curve"]["type"]
   put "curve tool" into tReturnData["com.livecode.tool.Curve"]["tool"]
   put "Curve Tool - Draws a curved line on an image" into tReturnData["com.livecode.tool.Curve"]["tooltip"]
   put 6 into tReturnData["com.livecode.tool.Curve"]["order"]
   
   put "tool" into tReturnData["com.livecode.tool.Pencil"]["type"]
   put "pencil tool" into tReturnData["com.livecode.tool.Pencil"]["tool"]
   put "Pencil Tool - Draws a freehand line on an image" into tReturnData["com.livecode.tool.Pencil"]["tooltip"]
   put 1 into tReturnData["com.livecode.tool.Pencil"]["order"]
   
   put "tool" into tReturnData["com.livecode.tool.Brush"]["type"]
   put "brush tool" into tReturnData["com.livecode.tool.Brush"]["tool"]
   put "Brush Tool - Draws freehand brush strokes on an image" into tReturnData["com.livecode.tool.Brush"]["tooltip"]
   put 2 into tReturnData["com.livecode.tool.Brush"]["order"]
   
   return tReturnData
end revIDEPaintTools

function revIDEPaintToolControllers
   local tReturnData
   put "property" into tReturnData["com.livecode.globalproperty.brush"]["type"]
   put "brush" into tReturnData["com.livecode.globalproperty.brush"]["property_name"]
   put "Set the brush size to change the effect of the brush or eraser" into tReturnData["com.livecode.globalproperty.brush"]["tooltip"]
   put "1" & return & "2" & return & "3" & return & "4" into tReturnData["com.livecode.globalproperty.brush"]["options"]
   return tReturnData
end revIDEPaintToolControllers

function revIDERunEditTools
   local tReturnData
   put "tool" into tReturnData["com.livecode.tool.Browse"]["type"]
   put "browse tool" into tReturnData["com.livecode.tool.Browse"]["tool"]
   put "Browse Tool - Your stack is running and can be interacted with" into tReturnData["com.livecode.tool.Browse"]["tooltip"]
   put 1 into tReturnData["com.livecode.tool.Browse"]["order"]
   
   put "tool" into tReturnData["com.livecode.tool.Pointer"]["type"]
   put "pointer tool" into tReturnData["com.livecode.tool.Pointer"]["tool"]
   put "Pointer Tool - Select and edit objects" into tReturnData["com.livecode.tool.Pointer"]["tooltip"]
   put 2 into tReturnData["com.livecode.tool.Pointer"]["order"]
   
   return tReturnData
end revIDERunEditTools

function revIDEClassicControls 
   if sClassicObjectProperties is empty then __objectPropertiesRead
    
   local tReturnData
   repeat for each key tIndex in sClassicObjectProperties
      if sClassicObjectProperties[tIndex]["type"] is not among the items of "graphic,stack,card,group" then 
         put sClassicObjectProperties[tIndex] into tReturnData[tIndex]
      end if
   end repeat
   return tReturnData
end revIDEClassicControls

/*
For a given list of object, returns a structured array of properties, values and other meta data.
pObjects (string list): A list of ID's of the objects you wish the properties for.
related: alksdfklasdf 
example: revIDEGetPropertiesOfObjects button 1
pObjects (String List): A list of object ID's you wish the common properties for. 
Return (Array): An array keyed by the names of the properties
Description:
# jkldsafgjlkasd
This is a new section
## This is a subsection
*/
function revIDEGetPropertiesOfObjects pObjects
   # Validation that all the objects in the list exist
   repeat for each line tObject in pObjects
      if not exists(tObject) then return __revIDEError("Not all objects in the list provided exist")
   end repeat
   
   # Work out the type of the selected objects
   local tObjectTypeList
   put __objectTypes(pObjects) into  tObjectTypeList
   
   # Get this list of all the properties to read for the given type list
   local tSharedProperties
   put __objectPropertiesShared(tObjectTypeList) into tSharedProperties
   
   # Generate a list of the properties we want to retreive
   local tPropertyValues
   repeat for each line tObject in pObjects 
      dispatch function "__readPropertiesOfControl" to tObject with the keys of tSharedProperties, tPropertyValues
      
      repeat for each line tPropertyName in the keys of tPropertyValues
         if tPropertyName begins with "effective" then next repeat
         if tPropertyValues["effective" && tPropertyName] is not empty then
            put tPropertyValues["effective" && tPropertyName] into tSharedProperties[tPropertyName]["effective value"][tObject]
         end if
         put tPropertyValues[tPropertyName] into tSharedProperties[tPropertyName]["value"][tObject]
      end repeat
   end repeat
   
   //put the executioncontexts & return & the milliseconds & return & return & return after msg
   
   return tSharedProperties
end revIDEGetPropertiesOfObjects

function revIDEGetPropertyOfObject pObject, pProperty
   return the pProperty of pObject
end revIDEGetPropertyOfObject

/*
Use to get a complete list of all editable stacks in the IDE. 

returns (String): A return delimited list of stack names
*/
function revIDEEditableStacks
   local tEditableStacks
   repeat for each line tStack in the openstacks
      if the mode of stack tStack < 2 then
         if tEditableStacks is empty then 
            put tStack into tEditableStacks
         else
            put return & tStack after tEditableStacks
         end if
      end if
   end repeat
   return tEditableStacks
end revIDEEditableStacks

/*
Use to get a list of of all the global variables.

returns (list): A comma delimited list of global variable names
*/
function revIDEGlobalVariables 
   if revIDEGetPreference("cShowRevolutionStacks") is true then return the globals
   
   local tGlobalList
   repeat for each item tGlobalName in the globals
      if tGlobalName begins with "gREV" then next repeat
      if tGlobalList is empty then
         put tGlobalName into tGlobalList
      else
         put comma & tGlobalName after tGlobalList
      end if
   end repeat
   sort items of tGlobalList
   
   return tGlobalList
end revIDEGlobalVariables

function revIDEGlobalProperties
   set the itemdel to slash
   set the defaultfolder to item 1 to -3 of the filename of me
   return URL("file:globalProperties.txt")
end revIDEGlobalProperties

/*
Gets a list of the mainstacks, taking into account the Show IDE Stacks preference. 

returns : An list of main stacks, one per line
*/
function revIDEMainStacks
   local tShowIDEStacks
   put revIDEGetPreference("cShowRevolutionStacks") into tShowIDEStacks
   
   # Get list of stacks to display
   local tMainStacks
   if tShowIDEStacks then
      put the mainstacks into tMainStacks
   else
      repeat for each line tMainStack in the mainstacks
         --if tMainStack begins with "rev" or the mode of stack tMainStack is not 1 then next repeat
        if revIDEStackIsRevStack(the long id of stack tMainStack) or the mode of stack tMainStack is not 1 then next repeat
         if tMainStacks is empty then 
            put tMainStack into tMainStacks
         else
            put return & tMainStack after tMainStacks
         end if
      end repeat
   end if
   return tMainStacks
end revIDEMainStacks

function revIDEStacksForDataView pIndex
   --pExpandedStructure
   # Are IDE stacks required
   local tShowIDEStacks
   put revIDEGetPreference("cShowRevolutionStacks") into tShowIDEStacks
   
   # Get list of stacks to display
   local tMainStacks
   if tShowIDEStacks then
      put the mainstacks into tMainStacks
   else
      repeat for each line tMainStack in the mainstacks
         --if tMainStack begins with "rev" or the mode of stack tMainStack is not 1 then next repeat
         if revIDEStackIsRevStack(the long id of stack tMainStack) or the mode of stack tMainStack is not 1 then next repeat
         if tMainStacks is empty then 
            put tMainStack into tMainStacks
         else
            put return & tMainStack after tMainStacks
         end if
      end repeat
   end if
   
   # Recurse through the stacks to get their data
   local tStacksData, tCount
   repeat for each line tStack in tMainStacks
      dispatch function "__readStructureOfStackForDataView" to stack tStack with tStacksData,, 1,pIndex
   end repeat
   return tStacksData
end revIDEStacksForDataView

/*
Gets the data associated with a stack. It recurses through the cards, containers and objects getting the property data associated with each. 

returns (array): An array representing a tree structure of controls and their properties.
*/
function revIDEStacks pExpandedStackList
   # Are IDE stacks required
   local tShowIDEStacks
   put revIDEGetPreference("cShowRevolutionStacks") into tShowIDEStacks
   
   # Get list of stacks to display
   local tMainStacks
   if tShowIDEStacks then
      put the mainstacks into tMainStacks
   else
      repeat for each line tMainStack in the mainstacks
         if tMainStack begins with "rev" or the mode of stack tMainStack is not 1 then next repeat
         
         if tMainStacks is empty then 
            put tMainStack into tMainStacks
         else
            put return & tMainStack after tMainStacks
         end if
      end repeat
   end if
   
   # Recurse through the stacks to get their data
   local tStacksData, tCount
   put 1 into tCount
   repeat for each line tStack in tMainStacks
      put empty into tStacksData[tCount]
      --if tStack is among the lines of pExpandedStackList then
         dispatch function "__readStructureOfStack" to stack tStack with tStacksData[tCount]
      --else
         --dispatch function "__readPropertiesOfControl" to stack tStack with sStackPropertiesToRead, tStacksData[tCount]
      --end if
      add 1 to tCount
   end repeat
   return tStacksData
end revIDEStacks

/*
Gets the data associated with all the frontscripts. 

returns (array): A numerically keyed (for ordering) array with property:value pairs for each key.
*/
function revIDEFrontScripts
   # Get the list of frontscripts
   local tFrontScripts
   put the frontscripts into tFrontScripts
   
   # If ID elements are on not showing, filter the list for rev stacks
   local tShowIDEStacks
   put revIDEGetPreference("cShowRevolutionStacks") into tShowIDEStacks
   if tShowIDEStacks is not true then
      filter tFrontScripts without "*/rev*"
   end if
   
   # Create the list of properties to retrieve
   local tProperties
   put "short name" & return & "scriptlines" & return & "long id" into tProperties
   
   # Loop through the frontscripts getting the required properties
   # and build the return array
   local tCount, tData
   put 1 into tCount
   repeat for each line tFrontScriptObject in tFrontScripts
      dispatch function "__readPropertiesOfControl" to tFrontScriptObject with tProperties, tData[tCount]
      add 1 to tCount
   end repeat
   
   return tData
end revIDEFrontScripts

/*
Gets the data associated with all backscrips.

returns (array): A numerically keyed (for ordering) array with property:value pairs for each key.
*/
function revIDEBackScripts
   # Get the list of frontscripts
   local tBackScripts
   put the backscripts into tBackScripts
   
   # If ID elements are on not showing, filter the list for rev stacks
   local tShowIDEStacks
   put revIDEGetPreference("cShowRevolutionStacks") into tShowIDEStacks
   if tShowIDEStacks is not true then
      filter tBackScripts without "*/rev*"
   end if
   
   # Create the list of properties to retrieve
   local tProperties
   put "short name" & return & "scriptlines" & return & "long id" into tProperties
   
   # Loop through the frontscripts getting the required properties
   # and build the return array
   local tCount, tData
   put 1 into tCount
   repeat for each line tBackScriptObject in tBackScripts
      dispatch function "__readPropertiesOfControl" to tBackScriptObject with tProperties, tData[tCount]
      add 1 to tCount
   end repeat
   
   return tData
end revIDEBackScripts

/*
Gets the data associated with all the stacks in use. 

returns (array): A numerically keyed (for ordering) array with property:value pairs for each key.
*/
function revIDEStacksInUse
   # Get the list of frontscripts
   local tStacksInUse
   put the stacksinuse into tStacksInUse
   
   # Create the list of properties to retrieve
   local tProperties
   put "short name" & return & "scriptlines" into tProperties
   
   # Loop through the stacks in use getting the required properties
   # and build the return array
   local tCount, tData
   put 1 into tCount
   repeat for each line tStack in tStacksInUse
      dispatch function "__readPropertiesOfControl" to stack tStack with tProperties, tData[tCount]
      put "stack" && quote & tStack & quote into tData[tCount]["long id"]
      add 1 to tCount
   end repeat
   
   return tData
end revIDEStacksInUse

function revIDEAPIFilters
end revIDEAPIFilters

function revIDEAPIFilter pFilters
end revIDEAPIFilter

function revIDEAPI pType, pName
end revIDEAPI

function revIDELocalisedString
end revIDELocalisedString

/*
Sets a preference.

pPreferenceName (String): The name of the preference to set
pValue (Any): The value to set the preference to
tags: preferences
Example: set revIDEGetPreference("text size") to 12
Returns (Any): The value of the preference
*/
on revIDESetPreference pPreferenceName, pValue
   set the pPreferenceName of stack "revpreferences" to pValue 
   revIDEMessageSend "idePreferenceChanged:" & pPreferenceName
end revIDESetPreference

/*
Gets a preference.

pPreferenceName (String): The name of the preference
tags: preferences
Example: set the textsize of field 1 to revIDEGetPreference("text size")
Returns (Any): The value of the preference
*/
function revIDEGetPreference pPreferenceName
   return the pPreferenceName of stack "revpreferences"
end revIDEGetPreference

/*
The IDE has a repository of icon images which are used for ui elements in the IDE. The revIDEIconID function converts a "name-size" tag into a LiveCode ID suitable for direct use.

pIcon (String): The "name-size" tag for the given icon. The name is the name of the icon. The size is the size of the icon and should be one of 16, 24, 32, 64, 128
tags:icons
Example: set the icon of button 1 to revIDEIconID("help-32")
Returns (integer): The id of the image object on the revIconsNew stack.
*/
function revIDEIconID pIcon
   if not exists(image pIcon of card 1 of stack "reviconsnew") then return pIcon && " does not exist"
   return the ID of image pIcon of card 1 of stack "reviconsnew"
end revIDEIconID

function revIDEThemePath
   # Get the platform and system version
   local tPlatformName, tSystemVersion
   put revIDEPlatform() into tPlatformName
   put revIDEPlatformVersion() into tSystemVersion
   
   # Find nearest theme version for the given platform
   revIDEStoreDefaultFolder
   set the defaultfolder to revIDESpecialFolderPath("themes")
   
   local tClosestThemeNumber, tHighestThemeNumber
   repeat for each line tThemeFolder in the folders
      if char 1 of tThemeFolder is "." then next repeat
      
      local tThemeFolderStart
      put "com.livecode.theme." & tPlatformName into tThemeFolderStart
      
      if tThemeFolder begins with tThemeFolderStart then
         delete char 1 to the number of chars of tThemeFolderStart + 1 of tThemeFolder
         
         if highestVersionNumber(tSystemVersion,tThemeFolder) < 2 and highestVersionNumber(tClosestThemeNumber,tThemeFolder) is 2 then
            --if tThemeFolder <= tSystemVersion and tThemeFolder > tClosestThemeNumber then
            put tThemeFolder into tClosestThemeNumber
         end if
         
         # keep track of the highest number
         if highestVersionNumber(tHighestThemeNumber,tThemeFolder) is 2 then 
            put tThemeFolder into tHighestThemeNumber
         end if
      end if
   end repeat
   
   # Build the path to the theme
   local tThemeName
   if tClosestThemeNumber is empty then 
      if tHighestThemeNumber is not empty then
         put "com.livecode.theme." & tPlatformName & "." & tHighestThemeNumber into tThemeName
      else
         put "com.livecode.theme." & tPlatformName into tThemeName
      end if
   else 
      put "com.livecode.theme." & tPlatformName & "." & tClosestThemeNumber into tThemeName
   end if
   
   revIDERestoreDefaultFolder
   
   return revIDESpecialFolderPath("themes") & slash & tThemeName
end revIDEThemePath

function highestVersionNumber pVersionNumber1, pVersionNumber2
   set the itemdel to "."
   local tCount
   put max(the number of items of pVersionNumber1,the number of items of pVersionNumber2) into tCount
   repeat with x = 1 to tCount
      if item x of pVersionNumber1 > item x of pVersionNumber2 then return 1
      else if item x of pVersionNumber1 < item x of pVersionNumber2 then return 2
   end repeat
   return 0
end highestVersionNumber

function revIDEPlatform
   if the platform begins with "mac" then return "mac"
   else if the platform begins with "win" then return "win"
   else if the platform begins with "linux" then return "linux"
   else if the platform begins with "iphone" then return "ios"
   else if the platform begins with "android" then return "android"
   return __revIDEError("Unknown Platform")
end revIDEPlatform

function revIDEPlatformVersion
   local tSystemVersion
   put the systemVersion into tSystemVersion
  
   if tSystemVersion begins with "Windows" or tSystemVersion begins with "NT"   then return the last word of tSystemVersion
   return tSystemVersion
end revIDEPlatformVersion

function revIDEColor pTag
   if pTag is "edition_color" then return ideCoreEditionColour()
   
   switch pTag
      case "text_1"
         return "0,0,0"
         break
      case "text_2"
         return "125,125,125"
         break
      case "text_3"
         return "255,255,255"
         break
      case "dataView_rowColor"
         return "255,255,255"
         break
      case "dataView_rowAlternateColor"
         return "238,238,238"
         break
      case "dataView_hiliteColor"
         return "48,160,209"
         break
      case "dataView_alternateHiliteColor"
         return "34,154,206"
         break
      case "dataView_TextHiliteColor"
         return "255,255,255"
         break
      case "dataView_scriptBackgroundColor"
         return "34,154,206"
         break
      case "palette_background"
         if the platform is "Win32" then
            return "240,240,240"
         else
            return the effective backgroundColor of this stack
         end if
         break
      case "propertyInspector_multiValueBackground"
         return "200,206,215"
         break
   end switch 
   
   return ideColorGet(pTag)
end revIDEColor

function revIDETextSize
   return 12
end revIDETextSize

function revIDECardOfObject pLongID
   local tOffset
   repeat for each word tWord in pLongID
      if tWord is "card" then
         add 1 to tOffset
         exit repeat
      else
         add 1 to tOffset
      end if
   end repeat
   return word tOffset to -1 of pLongID
end revIDECardOfObject

function revIDEStackOfObject pLongID
   local tOffset
   repeat for each word tWord in pLongID
      if tWord is "stack" then 
         add 1 to tOffset
         exit repeat
      else
         add 1 to tOffset
      end if
   end repeat
   return word tOffset to -1 of pLongID
end revIDEStackOfObject

on revIDEMoveControl pControl, pNewCard, pLayerNumber
   local tNewControl
   lock messages
   copy pControl to pNewCard
   put the long id of it into tNewControl
   
   delete pControl
   
   try
      --revIDESetPropertyOfObject tNewControl, "layer", pLayerNumber
   catch pError
   end try
   unlock messages   
   return tNewControl
end revIDEMoveControl

function revIDEStackProperties pStackID, pLevel
   if not exists(pStackID) then return __revIDEError("You must specify an object to read from." &&  quote & pStackID & quote && "is not an object")
    
   local tStackArray
   
   dispatch function "__readPropertiesOfControl" to pStackID with sStackPropertiesToRead, tStackArray
   --put the result into tStackArray
   put "stack" into tStackArray["type"]
   put "container" into tStackArray["style"]
   put pLevel into tStackArray["level"]
   put the number of lines in the cardIDs of pStackID into tStackArray["childCount"]
   if tStackArray["childCount"] is 0 then put 1 into tStackArray["childCount"]
   return tStackArray
end revIDEStackProperties

function revIDECardPropertiesOfStack pStackID, pLevel
   local tCardIDs, tCardArray, tIndex
   local tSubStackIDs, tSubstack
   
   put the cardIDs of pStackID into tCardIDs
   repeat for each line tCard in tCardIDs
      add 1 to tIndex
      dispatch function "__readPropertiesOfControl" to card id tCard of pStackID with sCardPropertiesToRead, tCardArray[tIndex]
      put "card" into tCardArray[tIndex]["type"]
      put "container" into tCardArray[tIndex]["style"]
      put pLevel+1 into tCardArray[tIndex]["level"]
      put the number of lines in the childControlIDs of card id tCard of pStackID into tCardArray[tIndex]["childCount"]
   end repeat
   
   ## Substacks
   put the substacks of pStackID into tSubStackIDs
   repeat for each line tStackID in tSubStackIDs
      add 1 to tIndex
      
      dispatch function "__readPropertiesOfControl" to stack tStackID with sStackPropertiesToRead, tCardArray[tIndex]
      put "substack" into  tCardArray[tIndex]["type"]
      put "container" into  tCardArray[tIndex]["style"]
      put pLevel+1 into  tCardArray[tIndex]["level"]
      put the number of lines in the cardIDs of stack tStackID of pStackID into  tCardArray[tIndex]["childCount"]
      if  tCardArray[tIndex]["childCount"] is 0 then put 1 into  tCardArray[tIndex]["childCount"]
   end repeat
   
   return tCardArray
end revIDECardPropertiesOfStack

function revIDEControlPropertiesOfCard pCardID, pLevel
   local tChildControls, tControlArray, tIndex, tFullArray, tChildCount
   
   --repeat for each line tControlId in the childcontrolIDs of pCardID
   put  the childcontrolIDs of pCardID into tChildControls
   put the number of lines in tChildControls into tChildCount
   
   repeat with x = 1 to the number of lines in tChildControls
      local tControlId
      if revIDEGetPreference("ideProjectBrowser_sortOrder") is "bottom to top" then
         put line x of tChildControls into tControlId
      else
         put line (tChildCount -x) + 1 of tChildControls into tControlId
      end if 
      
      add 1 to tIndex
      put empty into tControlArray      
      dispatch function "__readPropertiesOfControl" to control id tControlID of pCardID with sControlPropertiesToRead, tControlArray
      
      if tControlArray["type"] is "group" then
         put "group" into tControlArray["style"]
         put the number of controls of control id tControlID of pCardID into tControlArray["childCount"]
      else
         put "control" into tControlArray["style"]
      end if
      put pLevel+1 into tControlArray["level"]
      put tControlArray into tFullArray[tIndex]
   end repeat
   return tFullArray
end revIDEControlPropertiesOfCard

function revIDEControlPropertiesOfGroup pGroupID, pLevel
   local tControlArray, tIndex
   
   repeat for each line tControlId in the childcontrolIDs of pGroupID
      add 1 to tIndex
      dispatch function "__readPropertiesOfControl" to control id tControlID of pGroupID with sControlPropertiesToRead, tControlArray[tIndex]
      
      if tControlArray[tIndex]["type"] is "group" then
         put "group" into tControlArray[tIndex]["style"]
         put the number of controls of control id tControlID of pGroupID into tControlArray[tIndex]["childCount"]
      else
         put "control" into tControlArray[tIndex]["style"]
      end if
      put pLevel+1 into tControlArray[tIndex]["level"]
   end repeat
   return tControlArray
end revIDEControlPropertiesOfGroup

function revIDEControlProperties pControl, pLevel
   local tData
   dispatch function "__readPropertiesOfControl" to pControl with sControlPropertiesToRead, tData
   put pLevel into tData["level"]
   put "control" into tData["style"]
   return tData
end revIDEControlProperties

function revIDEGroupProperties pControl, pLevel
   local tData
   dispatch function "__readPropertiesOfControl" to pControl with sControlPropertiesToRead, tData
   put pLevel into tData["level"]
   put "group" into tData["style"]
   return tData
end revIDEGroupProperties

function revIDECardProperties pCard, pLevel
   local tData
   dispatch function "__readPropertiesOfControl" to pCard with sCardPropertiesToRead, tData
   
   put pLevel into tData["level"]
   put "container" into tData["style"]
   put "card" into tData["type"]
   return tData
end revIDECardProperties

function revIDEStackAudioClips pStack
   local tAudioClipCount, tAudioClipArray, tScriptLines, tBehavior, tBehaviorScriptLines
   
   -- MM-2013-09-16: [[ Bug 11280 ]] as a result of the fix for bug 11068, the engine is more strict in the way it processes references.
   -- Removed unecessary "stack" keywords.
   put the num of Audioclips in pStack into tAudioClipCount
   repeat with tClip = 1 to tAudioClipCount
      put "audioclip" into tAudioClipArray[tClip]["type"]
      put the short name of Audioclip tClip of pStack into tAudioClipArray[tClip]["name"]
      put the long id of Audioclip tClip of pStack into tAudioClipArray[tClip]["long id"]
      put 3 into tAudioClipArray[tClip]["level"]
      
      // Script
      try
         put the number of lines in  the script of Audioclip tClip of pStack into tScriptLines
      catch tError
         put "pwd" into tScriptLines
      end try      
      put tScriptLines into tAudioClipArray[tClip]["scriptlines"]
      
      // Behavior
      if the behavior of Audioclip tClip of pStack is not empty then
         if there is a (the behavior of Audioclip tClip of pStack) then
            try
               put the behavior of Audioclip tClip of pStack into tBehavior
            catch pError
               put "pwd" into tBehavior
            end try
            
            if tBehavior is not "pwd" then
               put the number of lines in the script of tBehavior into tBehaviorScriptLines
            else
               put "pwd" into tBehaviorScriptLines
            end if
         end if
         put tBehavior into tAudioClipArray[tClip]["behavior"]
         put tBehaviorScriptLines into tAudioClipArray[tClip]["behavior scriptlines"]
      else
         put empty into tAudioClipArray[tClip]["behavior scriptlines"]
      end if
   end repeat
   return tAudioClipArray
end revIDEStackAudioClips

function revIDEStackVideoClips pStack
   local tVideoClipCount, tVideoClipArray, tScriptLines, tBehavior, tBehaviorScriptLines, tAudioClipArray
   
   -- MM-2013-09-16: [[ Bug 11280 ]] as a result of the fix for bug 11068, the engine is more strict in the way it processes references.
   -- Removed unecessary "stack" keywords.
   put the num of videoclips in pStack into tVideoClipCount
   repeat with tClip = 1 to tVideoClipCount
      put "videoclip" into tVideoClipArray[tClip]["type"]
      put the short name of videoclip tClip of pStack into tVideoClipArray[tClip]["name"]
      put the number of lines in  the script of videoclip tClip of pStack into tAudioClipArray[tClip]["scriptlines"]
      put the long id of videoclip tClip of pStack into tVideoClipArray[tClip]["long id"]
      put 3 into tVideoClipArray[tClip]["level"]
      
      // Script
      try
         put the number of lines in  the script of videoclip tClip of pStack into tScriptLines
      catch tError
         put "pwd" into tScriptLines
      end try      
      put tScriptLines into tVideoClipArray[tClip]["scriptlines"]
      
      // Behavior
      if the behavior of videoclip tClip of pStack is not empty then
         if there is a (the behavior of videoclip tClip of pStack) then
            try
               put the behavior of videoclip tClip of pStack into tBehavior
            catch pError
               put "pwd" into tBehavior
            end try
            
            if tBehavior is not "pwd" then
               put the number of lines in the script of tBehavior into tBehaviorScriptLines
            else
               put "pwd" into tBehaviorScriptLines
            end if
         end if
         put tBehavior into tVideoClipArray[tClip]["behavior"]
         put tBehaviorScriptLines into tVideoClipArray[tClip]["behavior scriptlines"]
      else
         put empty into tVideoClipArray[tClip]["behavior scriptlines"]
      end if
   end repeat
   return tVideoClipArray
end revIDEStackVideoClips

/*
Returns A return sepearetd list of the long ids of the selected objects
*/
function revIDESelectedObjects
   return revPaletteObjects()
end revIDESelectedObjects

#############
# Actions
#############
local sDownloadData, sDownloadStatus, sDownloadQueue
on revIDEDownloadFile pURL, pTargetDirectory, pCallbackTarget, pProgressMessageName, pCompleteMessageName
   if char 1 to 3 of pURL is not among the items of "htt,ftp" then return __revIDEError("Cannot download file" && quote & pURL & quote & ". It is not a valid URL.")
   if there is not a folder pTargetDirectory then return __revIDEError("Cannot download file. Target directory" && quote & pTargetDirectory & quote && "does not exist")
   if pURL is among the lines of sDownloadQueue then return "File already in download queue"
   
   # Store data in download queue
   set the itemdel to "/"
   put pTargetDirectory & slash & the last item of pURL into sDownloadData[pURL]["file"]
   put pCallbackTarget into sDownloadData[pURL]["callback_target"]
   put pProgressMessageName into sDownloadData[pURL]["callback_progress_message"]
   put pCompleteMessageName into sDownloadData[pURL]["callback_complete_message"]
   
   if sDownloadQueue is empty then
      put pURL into sDownloadQueue
   else
      put return & pURL after sDownloadQueue
   end if
   
   send "__downloadNextFile" to me in 0 milliseconds
end revIDEDownloadFile

on __downloadNextFile
   if sDownloadQueue is empty then exit __downloadNextFile
   
   # First update the data based on status of current download if any
   local tFirstURL
   put line 1 of sDownloadQueue into tFirstURL
   put URLStatus(tFirstURL)
   switch item 1 of URLStatus(tFirstURL)
      case "queued"
      case "contacted"
      case "requested"
      case "loading"
      case "uploading"
         # First URL download in progress
         exit __downloadNextFile
         break
      case "downloaded"
      case "uploaded"
      case "cached"
         put URL tFirstURL into URL ("binfile:" & sDownloadData[tFirstURL]["file"])
         put the result
         # First URL download complete
         if exists(sDownloadData[tFirstURL]["callback_target"]) and sDownloadData[tFirstURL]["callback_complete_message"] is not empty then
            dispatch sDownloadData[tFirstURL]["callback_complete_message"] to sDownloadData[tFirstURL]["callback_target"] with tFirstURL
         end if
         __downloadRemove tFirstURL
         break
      case "error"
      case "timeout"
         # First URL download error
         add 1 to sDownloadData[tFirstURL]["error"]
         if sDownloadData[tFirstURL]["error"] > 2 then
            __downloadRemove tFirstURL
         end if
         break
   end switch
   
   # First item might have been removed so check is not empty and fetch the first URL to act on
   if sDownloadQueue is empty then exit __downloadNextFile
   put line 1 of sDownloadQueue into tFirstURL
   
   //load url tFirstURL with message "revIDEDownloadProgressUpdate"
   libURLDownloadToFile tFirstURL, sDownloadData[tFirstURL]["file"], "revIDEDownloadFinished"
   libURLSetStatusCallback "revIDEDownloadProgressUpdate", the long ID of me
end __downloadNextFile

private on __downloadRemove pURL
   # Remove URL from queue and wipe data in sDownloadData 
   repeat with x = 1 to the number of lines of sDownloadQueue
      if line x of sDownloadQueue is pURL then
         delete line x of sDownloadQueue
         delete variable sDownloadData[pURL]
      end if
   end repeat
end __downloadRemove

function revIDEQueuedDownloads
   return sDownloadQueue
end revIDEQueuedDownloads

on revIDEDownloadProgressUpdate pURL, pStatus
   # Send callback if requests
   if exists(sDownloadData[pURL]["callback_target"]) and sDownloadData[pURL]["callback_progress_message"] is not empty then
      dispatch sDownloadData[pURL]["callback_progress_message"] to sDownloadData[pURL]["callback_target"] with pURL, pStatus
   end if
end revIDEDownloadProgressUpdate

on revIDEDownloadFinished pURL
   __downloadNextFile
end revIDEDownloadFinished

on revIDEFindMoreWidgets
   revIDEMessageSend "ideFindMoreWidgets"
end revIDEFindMoreWidgets

on revIDECreateObject pObjectTypeID, pTarget, pLoc  
   if not exists(pTarget) then return __revIDEError("Cannot create object. Target does not exist: " && pTarget)
   if sClassicObjectProperties is empty then __objectPropertiesRead
   if not pObjectTypeID begins with "com.livecode" then __revIDEError("There us notobject type ID: " && pTarget)
   # Set the default stack to the target we're creating the object on
   set the defaultstack to revIDEStackOfObject(pTarget)
   
   # Create the object
   lock screen
   lock messages
   
   # Get the engine control type
   if pObjectTypeID begins with "com.livecode.interface.classic" then
      local tObjectType
      put sClassicObjectProperties[pObjectTypeID]["type"] into tObjectType
      
      if tObjectType is empty then return __revIDEError("Invalid classic control typoe ID: " && pTarget)
      
      # A classic control
      local tCreatedControlID
      do "create" && tObjectType
      put the long ID of the last control into tCreatedControlID
      
      # Set all the properties to their default
      repeat for each line tProperty in the keys of sClassicObjectProperties[pObjectTypeID]["properties"]
         if sClassicObjectProperties[pObjectTypeID]["properties"][tProperty]["default"] is "no_default" then next repeat
         if sClassicObjectProperties[pObjectTypeID]["properties"][tProperty]["read_only"] is true then next repeat
         revIDESetPropertyOfObject tCreatedControlID, tProperty, sClassicObjectProperties[pObjectTypeID]["properties"][tProperty]["default"]
      end repeat
   else
      # Not a classic control
      try
        create widget as pObjectTypeID
         put the long id of the last control into tCreatedControlID
      catch tError
         return false
      end try
   end if
   
   set the loc of tCreatedControlID to pLoc
   
   unlock messages
   unlock screen
   
   ## Prevent passing of ideNewControl message for objects created on IDE stacks
   if revIDEStackIsRevStack(tCreatedControlID) is false then
      revIDEMessageSend "ideNewControl", tCreatedControlID
   end if
   
   return tCreatedControlID
end revIDECreateObject

on revIDECloneObject pLongID
   local tTarget, tLoc, tControlType
   local tCreatedControlID
   
   put the loc of pLongID into tLoc
   add 20 to item 1 of tLoc
   add 20 to item 2 of tLoc
   
   put revIDECardOfObject(pLongID) into tTarget
   if not exists(tTarget) then return __revIDEError("Cannot create object. Target does not exist:" && tTarget)
   
   --   # Set the default stack to the target we're creating the object on
   --   set the defaultstack to the owner of tTarget
   
   --   # Create the object
   --   lock screen
   -- lock messages
   --do "clone" && pLongID
   clone pLongID
   put the long ID of the last control into tCreatedControlID
   set the loc of tCreatedControlID to tLoc
   
   --   unlock messages
   --   unlock screen
   --   return tCreatedControlID
end revIDECloneObject

on revIDECloneObjectWithDefaults pLongID
   local tTarget, tLoc, tControlType
   
   put the loc of pLongID into tLoc
   add 20 to item 1 of tLoc
   add 20 to item 2 of tLoc
   
   put revIDECardOfObject(pLongID) into tTarget
   if not exists(tTarget) then return __revIDEError("Cannot create object. Target does not exist:" && tTarget)
   
   local tOriginalObjectType
   put __objectType(pLongID) into  tOriginalObjectType
   # Get the engine control type
   local tObjectType
   put sClassicObjectProperties[tOriginalObjectType]["type"] into tObjectType
   if tObjectType is empty then return __revIDEError("Cannot create object. The engine type of classic control" && pLongID && "could not be determined. Please check the object definition file and ensure that a type has been defined.")
   
   # Set the default stack to the target we're creating the object on
   set the defaultstack to the owner of tTarget
   
   # Create the object
   lock screen
   --lock messages
   
   local tCreatedControlID
   do "create" && tObjectType
   put the long ID of the last control into tCreatedControlID
   
   # Set all the properties to their default
   repeat for each line tProperty in the keys of sClassicObjectProperties[tOriginalObjectType]["properties"]
      if sClassicObjectProperties[tOriginalObjectType]["properties"][tProperty]["default"] is "no_default" then next repeat
      if sClassicObjectProperties[tOriginalObjectType]["properties"][tProperty]["read_only"] is true then next repeat
      revIDESetPropertyOfObject tCreatedControlID, tProperty, sClassicObjectProperties[tOriginalObjectType]["properties"][tProperty]["default"]
   end repeat
   set the loc of tCreatedControlID to tLoc
   
   --unlock messages
   unlock screen
end revIDECloneObjectWithDefaults

on revIDEDeleteObjects pObjectIDs
   local tGroupID
   repeat for each line tControlID in pObjectIDs
      delete tControlID
   end repeat
end revIDEDeleteObjects

on revIDEGroupObjects pObjectIDs
   local tName, tLayer, tGroupID,tList
   put the long name of (line 1 of pObjectIDs) into tName
   put 1000000 into tLayer
   
   lock screen
   
   repeat for each line tControl in pObjectIDs
      if the layer of tControl < tLayer then put the layer of tControl into tLayer
      put tControl & " and " after tList
   end repeat
   
   delete char -5 to -1 of tList
   put "group " before tList
   
   do tList
   
   put the long id of the owner of (tName)  into tGroupID
   set the layer of tGroupID to tLayer
   return tGroupID
end revIDEGroupObjects


/*
Sets the tool.

pTool (enum): One of the following
pointer - Sets the tool to "pointer" mode causing the application to run as if a standalone
browse - Sets the tool to "browse" mode allowing you to select object
*/
on revIDESetTool pTool
   do "choose" && pTool
   revIDEMessageSend "ideToolChanged"
end revIDESetTool

/*
Sets the property of an object to the value specified

pObject (String): The long ID of the target object
pProperty (String): The name of the property to set
pValue (Any): The value to set the property to

Example: revIDESetPropertyOfObject the long ID of button 1, "label", "Go Next Card"
*/
on revIDESetPropertyOfObject pObjects, pProperty, pValue
   if pProperty is empty then return __revIDEError("You must specify a property name")
   
   lock screen
   repeat for each line tObject in pObjects
      if not exists(tObject) then return __revIDEError("Cannot set property '"&tObject&"' because object does not exist")
      
      try 
         if pProperty contains "[" then
            ## An array property
            do ("set" && pProperty && "of" && tObject && "to" && pValue)
         else
            set pProperty of tObject to pValue
         end if
      catch tError         
         if the number of elements of pValue > 0 then put revFormatArrayForDisplay(pValue) into pValue
         return __revIDEError(tError & return & "Property:" & pProperty & return & "Value:" & pValue & return & "Object:" & tObject)
      end try
   end repeat
   unlock screen
end revIDESetPropertyOfObject

command revIDEInspectObjects pObjects
   send "revIDEMessageSend ideInspectObjects","pObjects" to stack "revIDELibrary" in 0 milliseconds
end revIDEInspectObjects

/*
Generates API documentation for a giver object and launched it in the system browser.

Example: revIDELoadAPIForScript the long id of button 1
*/
on revIDELoadAPIForScript pObjectID
   revDocsPreviewObjectScriptInDocsViewer pObjectID
end revIDELoadAPIForScript

--on revIDERelayerControl pControl, pLayerNumber
--   revIDESetPropertyOfObject pControl, "layer", pLayerNumber
--end revIDERelayerControl

on revIDERelayerControl pControl, pLayerNumber, pCard
   local tObjectID, tName
   --put pControl && "of" && pCard into tObjectID
   put pControl into tObjectID
   put the name of tObjectID && "of" && pCard into tName
   if not exists(tObjectID) then return __revIDEError(tObjectID && "is not a control")
   
   revIDESetPropertyOfObject pControl, "layer", pLayerNumber
   return the long id of tName
end revIDERelayerControl

on revIDEMoveControl pControl, pNewCard, pLayerNumber
   local tNewControl
   lock messages
   copy pControl to pNewCard
   put the long id of it into tNewControl
   
   delete pControl
   
   revIDESetPropertyOfObject tNewControl, "layer", pLayerNumber
   unlock messages   
end revIDEMoveControl

on revIDEMoveCard pCard, pNewStack
   local tNewCard
   lock messages
   copy pCard to pNewStack
   put the long id of it into tNewCard
   
   delete pCard
   unlock messages
   return tNewCard
end revIDEMoveCard

on revIDEMoveSubstack pSubstack, pNewStack
   local tNewSubstack
   lock messages
   set the mainstack of pSubstack to pNewStack
   unlock messages
end revIDEMoveSubstack

on  revIDERenumberCard pCardID,pNewNumber
   revIDESetPropertyOfObject pCardID, "number", pNewNumber
end revIDERenumberCard

command revIDESelectObjects pObjects
   try
      replace return with " AND " in pObjects
      do "select" && pObjects
   catch tError
      
   end try
end revIDESelectObjects

command revIDERefreshProjectBrowser
   dispatch "updateProjectView" to stack "revIDEProjectBrowser"
end revIDERefreshProjectBrowser

on revIDEAlignControls pControls, pPosition
   local tInitialControl, tInitialValue
      
   put line 1 of pControls into tInitialControl
   
   if pPosition is "left" then
      put the left of tInitialControl into tInitialValue
   else if pPosition is "top" then
      put the top of tInitialControl into tInitialValue
   else if pPosition is "right" then
      put the right of tInitialControl into tInitialValue
   else if pPosition is "bottom" then
      put the bottom of tInitialControl into tInitialValue
   end if
   
   repeat with x = 2 to the number of lines in pControls
      revIDESetPropertyOfObject line x of pControls, pPosition, tInitialValue
   end repeat
end revIDEAlignControls

on revIDECenterControls pControls, pPosition   
   local tInitialControl,tInitialLoc, tPositionValue, tNewLoc, tObject, tObjectLoc
   
   put line 1 of pControls into tInitialControl
   put value("the loc of me", tInitialControl) into tInitialLoc 
   
   if pPosition is "vertical" then
      put item 1 of tInitialLoc into tPositionValue 
      
      repeat with x = 2 to the number of lines in pControls
         put line x of pControls into tObject
         put value("the loc of me", tObject) into tObjectLoc
         put tPositionValue, item 2 of tObjectLoc into tNewLoc
         revIDESetPropertyOfObject tObject, "location", tNewLoc
      end repeat
      
   else if pPosition is "horizontal" then
      put item 2 of tInitialLoc into tPositionValue 
      
      repeat with x = 2 to the number of lines in pControls
         put line x of pControls into tObject
         put value("the loc of me", tObject) into tObjectLoc 
         put item 1 of tObjectLoc, tPositionValue into tNewLoc
         revIDESetPropertyOfObject tObject , "location", tNewLoc
      end repeat      
   end if
end revIDECenterControls

on revIDEEqualizeControls pControls, pPosition
   local tInitialControl, tInitialValue, tObject
   local tInitialWidth, tInitialHeight
   
   put line 1 of pControls into tInitialControl
   
   if pPosition is "width" then
      put the width of tInitialControl into tInitialValue
      repeat with x = 2 to the number of lines in pControls
         put line x of pControls into tObject
         revIDESetPropertyOfObject tObject, "width", tInitialValue
      end repeat
   else if pPosition is "height" then
      put the height of tInitialControl into tInitialValue      
      repeat with x = 2 to the number of lines in pControls
         put line x of pControls into tObject
         revIDESetPropertyOfObject tObject, "height", tInitialValue
      end repeat
      
   else if pPosition is "rect" then
      put the width of tInitialControl into tInitialWidth 
      put the height of tInitialControl into tInitialHeight
      repeat with x = 2 to the number of lines in pControls
         put line x of pControls into tObject
         revIDESetPropertyOfObject tObject, "width", tInitialWidth
         revIDESetPropertyOfObject tObject, "height", tInitialHeight
      end repeat
   end if
end revIDEEqualizeControls

command revIDEDistributeControls pControls, pPosition, pDistribution
   local tStack, tLeft, tRight, tTop,tBottom, tTotalWidth, tTotalHeight, tSpacing
   local tAvailableWidth, tAvailableHeight
   local tOrderedObjectList
   
   put word -2 to -1 of line 1 of pControls into tStack
   put the width of tStack into tLeft
   put the height of tStack into tTop
   put 0 into tRight
   put 0 into tBottom
   repeat with x = 1 to the number of lines in pControls
      get line x of pControls
      if the left of it < tLeft then put the left of it into tLeft
      if the top of it < tTop then put the top of it into tTop
      if the right of it > tRight then put the right of it into tRight
      if the bottom of it > tBottom then put the bottom of it into tBottom
      add the width of it to tTotalWidth
      add the height of it to tTotalHeight
      
      put it & comma & the left of it & comma & the top of it & return after tOrderedObjectList
   end repeat
   
   if pControls is "horizontal" then
      put "Across Card" into pDistribution
      switch pDistribution
         case "First To Last selected"
            sort lines of tOrderedObjectList by item 2 of each
            put tRight-tLeft into tAvailableWidth
            put ((tAvailableWidth-tTotalWidth)/(the number of lines in pControls)) div 1 into tSpacing
            break
         case "Edge To Edge"
            sort lines of tOrderedObjectList by item 2 of each
            put 0 into tSpacing
            break
         case "Across Card"
            sort lines of tOrderedObjectList by item 2 of each
            put the width of tStack into tAvailableWidth
            put ((tAvailableWidth-tTotalWidth)/(the number of lines in pControls+1)) div 1 into tSpacing
            put tSpacing into tLeft
            break
      end switch
      repeat with x = 1 to the number of lines in tOrderedObjectList
         get item 1 of line x of pControls
         revIDESetPropertyOfObject item 1 of line x of pControls, "left", tLeft
         --set the left of it to tLeft
         add the width of it + tSpacing to tLeft
      end repeat      
   else if pControls is "vertical" then
      switch pDistribution
         put "Across Card" into pDistribution
      case "First To Last selected"
         sort lines of tOrderedObjectList by item 3 of each
         put tBottom-tTop into tAvailableHeight
         put ((tAvailableHeight-tTotalHeight)/number of lines in tOrderedObjectList) div 1 into tSpacing
         break
      case "Edge To Edge"
         sort lines of tOrderedObjectList by item 3 of each
         put 0 into tSpacing
         break
      case "Across Card"
         sort lines of tOrderedObjectList by item 3 of each
         put the height of tStack into tAvailableHeight
         put ((tAvailableHeight-tTotalHeight)/(the number of lines in tOrderedObjectList+1)) div 1 into tSpacing
         put tSpacing into tTop
         break
   end switch
   repeat with x = 1 to the number of lines in tOrderedObjectList
      get item 1 of line x of tOrderedObjectList
      revIDESetPropertyOfObject item 1 of line x of tOrderedObjectList, "top", tTop
      --set the top of it to tTop
      add the height of it + tSpacing to tTop
   end repeat   
end if
end revIDEDistributeControls

#############
# Utility
#############
/*
Similar in functionality to specialFolderPath, it returns paths the folders with the IDE structure.

pKey (enum): The key for the folder you wish the retrieve.
- IDE: Returns the path the base IDE folder
- Object Property Definitions: Returns the path to the folder containing the object property definition files

returns (string): the folder on the local file system where the IDE is stored.

Example: 
local tIDEFolder
put revIDESpecialFolderPath("IDE") into tIDEFolder
*/
function revIDESpecialFolderPath pKey
   local tPath
   set the itemdel to "/"
   switch pKey
      case "IDE"
         return item 1 to -3 of the filename of stack "home"
         break
      case "Toolset"
         return item 1 to -3 of the filename of stack "home" & "/Toolset"
         break
      case "Toolchain"
         return item 1 to -3 of the filename of stack "home" & "/Toolchain"
         break
      case "Object Property Definitions"
         return item 1 to -3 of the filename of stack "home" & "/Toolset/resources/supporting_files/property_definitions"
         break
      case "Tool Definitions"
         return item 1 to -3 of the filename of stack "home" & "/Toolset/resources/supporting_files/tool_definitions"
         break
      case "fonts"
         return item 1 to -3 of the filename of stack "home" & "/Toolset/resources/supporting_files/fonts"
         break
      case "themes"
         return item 1 to -3 of the filename of stack "home" & "/Toolset/resources/supporting_files/themes"
         break
      case "extensions"
         put  item 1 to -3 of the filename of stack "home" & "/extensions" into tPath
         revIDEEnsurePath(tPath)
         return tPath
         break
      case "user extensions"
         put revEnvironmentCustomizationPath() & "/extensions" into tPath
         revIDEEnsurePath(tPath)
         return tPath
         break
      case "temp extensions"
         put revEnvironmentCustomizationPath() & "/extensions/temp" into tPath
         revIDEEnsurePath(tPath)
         return tPath
         break
      case "downloading extensions"
         put revEnvironmentCustomizationPath() & "/extensions/downloading" into tPath
         revIDEEnsurePath(tPath)
         return tPath
         break
      case "uninstalled extensions"
         put revEnvironmentCustomizationPath() & "/extensions/uninstalled" into tPath
         revIDEEnsurePath(tPath)
         return tPath
         break
      case "documentation"
         put item 1 to -3 of the filename of stack "home" & "/Documentation" into tPath
         revIDEEnsurePath(tPath)
         return tPath
         break
      case "documentation cache"
         put revEnvironmentCustomizationPath() & "/Documentation" into tPath
         revIDEEnsurePath(tPath)
         return tPath
         break
      case "api"
         return item 1 to -3 of the filename of stack "home" & "/Documentation/html_viewer/resources/data/api"
         break
      case "guide"
         return item 1 to -3 of the filename of stack "home" & "/Documentation/html_viewer/resources/data/guide"
         break
      case "examples"
         put  item 1 to -3 of the filename of stack "home" & "/examples" into tPath
         revIDEEnsurePath(tPath)
         return tPath
         break
   end switch
end revIDESpecialFolderPath

on revIDEEnsurePath pPath
   if there is a folder pPath then exit revIDEEnsurePath
   
   local tPath
   set the itemdel to "/"
   repeat for each item tFolder in pPath
      
      if tFolder contains ":" then
         put tFolder after tPath
      else
         put slash & tFolder after tPath
      end if
      
      if tPath is empty then next repeat
      if there is a folder (quote & tPath & quote) then next repeat
      
      create folder tPath
   end repeat
end revIDEEnsurePath

/*
Returns the URL to a web page

pKey (enum): The key for the page you wish the retrieve.

returns (string): an http url to a wb page

Example: 
local tExtensionSearchPage
put revIDEWebPageURL("extensionsSearch") into tExtensionSearchPage
*/
function revIDEWebPageURL pKey
   switch pKey
      case "extensions"
         return "http://ng.livecode.com/products/extensions/"
         break
   end switch
end revIDEWebPageURL

function revIDEFrameBehavior
   return the long ID of button "paletteBehavior" of stack "revCore"
end revIDEFrameBehavior

function revIDEDataViewBehavior
   return the long ID of button "DataView Behavior" of stack "DataViewControl" of stack "revCore"
end revIDEDataViewBehavior

local sMilliseconds
on revIDEStartTiming
   put the milliseconds into sMilliseconds
end revIDEStartTiming

on revIDEEndTiming
   return the milliseconds - sMilliseconds
end revIDEEndTiming

on revIDESetCursor pCursor
   lock cursor
   if pCursor is among the items of "busy,watch,array,cross,hard,ibeam,plus,help" then 
      do "set the cursor to" && pCursor
   end if
   
   lock messages
   local tDefaultStack
   put the defaultstack into tDefaultStack
   set the defaultstack to "revidelibrary"
   
   # Load the image for the cursor if it doesn't exists
   if there is not an image pCursor of me then
      local tImagePath
      put revIDEThemePath() & slash & pCursor & ".png" into tImagePath
      
      if there is not a file tImagePath then exit revIDESetCursor
      set the filename of the templateimage to tImagePath
      set the visible of the templateimage to false
      create image pCursor
      reset the templateimage
   end if
   
   if there is an image pCursor then
      set the cursor to the ID of image pCursor
   end if
   
   set the defaultstack to tDefaultStack
   unlock messages
end revIDESetCursor

on revIDEResetCursor
   unlock cursor
end revIDEResetCursor

/*
Use to tell the IDE that an action being performed is running slowly. The IDE will change the cursor and display the message as appropriate.
pMessage (String): A optional message that will be displayed when slow tasks are running
tag:affordances
*/
on revIDEShowLoading pMessage
   lock cursor
   set the cursor to watch
end revIDEShowLoading

/*
Use along with [revIDEShowLoading] to instruct the IDE when a task is running slowly. Once the task is complete, call revIDEHideLoading to reset the IDE to its normal mode
tag:affordances
*/
on revIDEHideLoading
   unlock cursor
end revIDEHideLoading

local sDefaultFolder
/*
Stores the current default folder.

Related: on revIDERestoreDefaultFolder

Example: 
on myHandler 
   revIDEStoreDefaultFolder
   set the defaultfolder to "/"
   repeat for each line tLine in the files
      # Do something to the files
   end repeat
   revIDERestoreDefaultFolder
end myHandler
*/
on revIDEStoreDefaultFolder
   put the defaultfolder into sDefaultFolder
end revIDEStoreDefaultFolder

/*
Restores the default folder to the last saved path.

Related: on revIDEStoreDefaultFolder

Example: 
on myHandler 
   revIDEStoreDefaultFolder
   set the defaultfolder to "/"
   repeat for each line tLine in the files
      # Do something to the files
   end repeat
   revIDERestoreDefaultFolder
end myHandler
*/
on revIDERestoreDefaultFolder
   set the defaultfolder to sDefaultFolder
end revIDERestoreDefaultFolder

/*
summary: Outputs a flat version of an array to the messagebox.
pArray (Array): The array you which to convert
returns (String): A text representation of the array with indented with the tab character for readability
*/
on revPutArray pArray
   if the number of elements of pArray is 0 then 
      put "not an array"
      exit revPutArray
   end if
   put revFormatArrayForDisplay(pArray)
end revPutArray

/*
summary: Converts an array into a human readable format that can be output to the message box when debugging.
pArray (Array): The array you which to convert
returns (String): A text representation of the array with indented with the tab character for readability
*/
function revFormatArrayForDisplay pArray
   if (the keys of pArray) is empty then
      put "not an array"
      exit revFormatArrayForDisplay
   end if
   
   local pLevel, pCounter, pList
   
   put 0 into pLevel
   put 0 into pCounter
   put empty into pList
   
   put __dumpArrayRecurse(pArray, pLevel, pCounter, pList) into pList
   
   return pList
end revFormatArrayForDisplay

on revIDEResizeImageInBounds tImageID, tDesiredWidth,tDesiredHeight
   if not exists(tImageID) then exit revIDEResizeImageInBounds
   if the width of tImageID < tDesiredWidth and the height of tImageID < tDesiredHeight then exit revIDEResizeImageInBounds
   
   local tCurrentWidth, tCurrentHeight
   put the width of tImageID into tCurrentWidth
   put the height of tImageID into tCurrentHeight
   
   local tscaleFactor
   put min(tDesiredWidth/tCurrentWidth,tDesiredHeight/tCurrentHeight) into tscaleFactor
   
   lock screen
   lock messages
   set the lockloc of tImageID to true
   set the width of tImageID to the width of tImageID * tscaleFactor
   set the height of tImageID to the height of tImageID * tscaleFactor
   unlock messages
   unlock screen
end revIDEResizeImageInBounds

/*
Opens an IDE stack as a palette, any additional intelligence related to the palette is managed here, for example management of multiple Property Inspectors.

pPaletteName (String):The readable name of the palette to be opened
example: revIDEOpenPalette "inspector"
*/
command revIDEOpenPalette pPaletteName
   local tExistingPalette, tPalette, tPaletteID
   
   switch pPaletteName
      case "inspector"
         --palette stack "revPropertyInspectorView"
         put revIDEPropertyInspector() into tExistingPalette
         if tExistingPalette is not empty then
            palette stack tExistingPalette
         else
            // Create new unlocked palette and open
            put revIDENextPropertyInspectorID() into tPaletteID
            put "revIDEPropertyInspector" & "_" & tPaletteID into tPalette           
            
            lock messages
            clone invisible stack "revIDEPropertyInspectorView" as tPalette
            unlock messages
            set the behavior of stack tPalette to revIDEFrameBehavior()
            palette stack tPalette
            show stack tPalette
         end if
         break
      case "project browser"
         modeless stack "revIDEProjectBrowser"
         break
      case "extension manager"
         modeless stack "revIDEExtensionManager"
         break
   end switch
end revIDEOpenPalette

/*
Returns a list of all the open Property Inspectors
*/
function revIDEOpenPalettes pType
   local tStackList
   put the mainStacks into tStackList
   switch pType
      case "inspector"
         filter tStackList with "revIDEPropertyInspector_*"
         return tStackList
         break        
   end switch
   
   return __revIDEError("Not a valid palette type")
end revIDEOpenPalettes

/*
Returns the next numeic ID for a property inspector
*/
function revIDENextPropertyInspectorID
   local tPIList
   put revIDEOpenPalettes("inspector") into tPIList
   
   if tPIList is empty then
      return 1 
   else
      set the itemDel to "_"
      sort lines of tPIList descending numeric by item 2 of each 
      return item 2 of line 1 of tPIList + 1
   end if
end revIDENextPropertyInspectorID

/*
Returns the name of the first unlocked Property Inspector
*/
function revIDEPropertyInspector
   local tPIList
   put revIDEOpenPalettes("inspector") into tPIList
   
   if tPIList is empty then
      return empty
   else
      set the itemDel to "_"
      sort lines of tPIList ascending numeric by item 2 of each
      
      repeat for each line tInspector in tPIList
         if the cLocked of stack tInspector is not true then
            return tInspector
         end if
      end repeat
      return empty
   end if
end revIDEPropertyInspector

function revIDEStackIsRevStack pLongID
   local tStackID
   put revIDEStackOfObject(pLongID) into tStackID
   
   ## Special cases of IDE stacks that don't begin with rev
   if the short name of tStackID is among the items of "message box" then
      return true
   else if the short name of tStackID begins with "com.runrev" then
      return true
   else
      return (the short name of tStackID begins with "rev")
   end if
end revIDEStackIsRevStack


function revIDEItemiseArray pArray
   if (the keys of pArray) is empty then
      put "not an array"
      exit revIDEItemiseArray
   end if
   
   local pLevel, pCounter, pList, pParent
   
   put 0 into pLevel
   put 0 into pCounter
   put empty into pParent
   put empty into pList
   
   put arrayRecurse(pArray, pLevel, pCounter, pList, pParent) into pList
   
   ## Add in all the parentLastSibling stuff into the array
   ## Was the parent a last sibling
   local tChildren
   repeat for each line tKey in the keys of pList
      put pList[tKey]["children"] into tChildren
      if tChildren is not empty then
         repeat for each item tChild in tChildren
            if pList[tKey]["lastSibling"] is true then
               put true into pList[tChild]["parentLastSibling"]
            end if
         end repeat
      end if
   end repeat
   
   return pList   
end revIDEItemiseArray

function arrayRecurse pArray, pLevel, @pCounter, @pList, pParent
   local tKeys, tOrderedKeys, tChildren, tSingleKeys, tArrayKeys
   local tNewKey, tNewSet
   
   put the keys of pArray into tKeys
   --sort lines of tKeys numeric
   sort lines of tKeys
   
   ## Put single elements first
   repeat for each line tKey in tKeys
      if pArray[tKey] is an array then
         --if tKey is "new set" then
         --put true into tNewSet
         --else
         put tKey & return after tArrayKeys
         --end if
      else
         if tKey is "new key" then
            put true into tNewKey
         else if tKey is "new set" then
            put true into tNewSet
         else
            put tKey & return after tSingleKeys
         end if
      end if
   end repeat
   
   if the last char of tArrayKeys is return then delete the last char of tArrayKeys
   if the last char of tSingleKeys is return then delete the last char of tSingleKeys
   
   if tNewKey then
      if tSingleKeys is empty then put "new key" into tSingleKeys
      else put return & "new key" after tSingleKeys
   end if
   
   if tNewSet then
      if tArrayKeys is empty then put "new set" into tArrayKeys
      else put return & "new set" after tArrayKeys
   end if
   
   --if tNewSet then put return & "new set" after tArrayKeys
   
   ## Move "newKey" to the end
   put tSingleKeys & return & tArrayKeys into tOrderedKeys
   
   repeat for each line tKey in tOrderedKeys
      add 1 to pCounter 
      put pCounter & comma after tChildren
      put pParent into pList[pCounter]["parent"]
      put pLevel + 1 into pList[pCounter]["level"]
      put tKey into pList[pCounter]["key"]
      
      if (the keys of pArray[tKey]) is empty then
         put "false" into pList[pCounter]["expanded"]
         put "element" into pList[pCounter]["type"]
         put pArray[tKey] into pList[pCounter]["value"]
      else
         put "false" into pList[pCounter]["expanded"]
         put "array" into pList[pCounter]["type"]
         put arrayRecurse(pArray[tKey], (pLevel + 1), pCounter, pList, pCounter) into pList
      end if
   end repeat
   delete the last char of tChildren
   put "true" into pList[the last item of tChildren]["lastSibling"]
   
   if pParent is not empty then
      put tChildren into pList[pParent]["children"]
   end if
   return pList
end arrayRecurse

private function __escapeStringAndConvertLineEndings pString
   replace "\" with "\\" in pString
   replace quote with ("\" & quote) in pString
   replace (numToChar(13) & CR) with (CR & numToChar(13)) in pString
   replace (CR & numToChar(13)) with CR in pString
   replace CR with "\n" in pString
   return (quote & pString & quote)
end __escapeStringAndConvertLineEndings

/*
Install the given extension's user guide
*/
on revIDEInstallUserGuide pGuideFolder, pExtensionName, pAuthor
   
   local tGuideFile
   put pGuideFolder & slash & "guide.md" into tGuideFile
   
   # Convert the user guide to a JSON array to append to user guide data
   local tGuide
   put url ("binfile:" & tGuideFile) into tGuide
   if tGuide is empty then
      return "no user guide data"
   end if
   
   put textDecode(tGuide, "utf-8") into tGuide
   
   local tGuideJSON
   put "{" & CR after tGuideJSON
   put tab & quote & "guide" & quote & ":" && quote & pExtensionName & quote & comma & CR after tGuideJSON
   put quote & "data" & quote & ":" & __escapeStringAndConvertLineEndings(tGuide) & CR after tGuideJSON
   put "}" after tGuideJSON
   
   put textEncode(tGuideJSON, "utf-8") into url ("binfile:" & pGuideFolder & slash & "guide.js")
   
   # Now regenerate the built guides
   revDocsAddGuideAndRegenerate tGuideJSON
end revIDEInstallUserGuide

/*
Install the given extension's API
*/
on revIDEInstallAPI pAPIFolder, pExtensionName, pAuthor
   
   local tAPIFile
   put pAPIFolder & slash & "api.lcdoc" into tAPIFile
   
   # Convert the new API to a JSON array to append to API data
   local tAPIJSON
   put revDocsFormatAPIAsJSON(pExtensionName, pAuthor, tAPIFile) into tAPIJSON
   if tAPIJSON is empty then
      return "no documentation found"
   end if
   
   put textEncode(tAPIJSON, "utf-8") into url ("binfile:" & pAPIFolder & slash & "api.js")
   
   # Now regenerate the built APIs
   revDocsAddAPIAndRegenerate tAPIJSON
end revIDEInstallAPI
