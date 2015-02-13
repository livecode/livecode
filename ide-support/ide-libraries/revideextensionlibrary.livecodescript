script "revideextensionlibrary"
local sExtensions, sExtensionProperties

on revLoadLibrary
	insert script of me into back 
end revLoadLibrary

on revUnloadLibrary
	remove script of me from back
end revUnloadLibrary

# Called at startup. Loads all the extensions ready for use
on revIDEInitialiseExtensions
   # Install extensions that haven't yet been installed
   repeat for each line tExtensionPackage in __extensionPackagePaths()
      revIDEInstallExtension tExtensionPackage
   end repeat
   
   # Load all the extensions ready for use
   repeat for each line tExtensionModulePath in __extensionModulePaths()
      __extensionLoad tExtensionModulePath
   end repeat
end revIDEInitialiseExtensions

# Intalls the extension
on revIDEInstallExtension pExtensionPath, pType, pCallbackObject, pCallbackMessage
   if pExtensionPath begins with "http" or there is a file pExtensionPath then 
      
      # Get the ID of the extension in the cache
      local tIndex
      put __extensionCacheID("package_url",pExtensionPath) into tIndex
      
      if tIndex is empty then
          put __extensionCacheID("download_package_path",pExtensionPath) into tIndex
      end if
      
      # If the extension cache ID couldn't be found, generate a new one
      if tIndex is empty then 
         put the number of elements of sExtensions + 1 into tIndex
      end if
      
      # Add callback data to cache
      __extensionPropertySet tIndex, "callback_target", pCallbackObject
      __extensionPropertySet tIndex, "callback_handler", pCallbackMessage
      __extensionPropertySet tIndex, "name", "New Extension Installing"
      
      if pExtensionPath begins with "http" then 
         # Download the package and install
         __extensionPropertySet tIndex, "package_url", pExtensionPath
         __extensionInstallDownload tIndex
      else
         # Package already downloaded - begin installation
          __extensionPropertySet tIndex, "download_package_path", pExtensionPath
         __extensionInstallVerify tIndex
      end if
   else
      # pExtensionPath is not a URL or file on disk
      return __extensionError(tIndex,"Could not install extension. The extension must be a URL to a valid package or a path to a extension package on disk:" && pExtensionPath)
   end if
end revIDEInstallExtension

# Uninstalls the extension
on revIDEUninstallExtension pExtensionTypeID 
   # Start the uninstall process
   if pExtensionTypeID is not a number then
      put __extensionCacheID("name", pExtensionTypeID) into pExtensionTypeID
   end if
   
   __extensionUninstallCheckInUse pExtensionTypeID
end revIDEUninstallExtension

# Returns the extension data
function revIDEExtensions pType, pStatus
   # No type passed so return all extensions
   
   # Repeat over the extension array looking for elements with matching type
   local tExtensions
   repeat for each line tExtensionKey in the keys of sExtensions
      if sExtensions[tExtensionKey]["name"] is empty then 
         delete variable sExtensions[tExtensionKey]
         next repeat
      end if
      if pType is not empty and sExtensions[tExtensionKey]["type"] is not pType then next repeat
      if pStatus is not empty and sExtensions[tExtensionKey]["status"] is not pStatus then next repeat
      
      put sExtensions[tExtensionKey] into tExtensions[sExtensions[tExtensionKey]["name"]]
   end repeat
   return tExtensions
end revIDEExtensions

# Returns the extension data
function revIDEExtensionProperties pTypeID
   return sExtensionProperties[pTypeID]
end revIDEExtensionProperties

##############################
# PRIVATE INSTALLATION PROCESS
##############################

# Download the extension
on __extensionInstallDownload pExtensionID
   # Check the file extension is correct
   local tURL
   put __extensionPropertyGet(pExtensionID,"package_url") into tURL
   
   set the itemdel to "."
   if the last item of tURL is not "lce" then return __extensionError(pExtensionID,"Could not download extension. The package must have the file extension 'lce':" && tURL)
   
   local tPackageFilePath
   set the itemdel to "/"
   put the last item of tURL into tPackageFilePath
   
   __extensionPropertySet pExtensionID, "download_package_path", revIDESpecialFolderPath("downloading extensions") & slash & tPackageFilePath
   __extensionPropertySet pExtensionID, "status", "downloading"
   __extensionPropertySet pExtensionID, "progress_message", "Downloading"
   __extensionPropertySet pExtensionID, "progress", 0
   __extensionPropertySet pExtensionID, "author", "Unknown Author"
   __extensionPropertySet pExtensionID, "label", tPackageFilePath
   
   # Put is a first stab at the name
   set the itemdel to "."
   local tName
   put tPackageFilePath into tName
   delete the last item of tName
   repeat while the last item of tName is a number
      delete the last item of tName
   end repeat
   __extensionPropertySet pExtensionID, "name", tName
   
   # Send update to refresh UI with new package installation
   __extensionsChanged
   
   # Update progress
   __extensionSendProgressUpdate pExtensionID, "Downloading", 0
   
   # Download the extension to a file
   libURLDownloadToFile tURL, __extensionPropertyGet(pExtensionID,"download_package_path"), "__extensionInstallDownloadComplete"
end __extensionInstallDownload

# Dowload complete
on __extensionInstallDownloadComplete pURL, pDownloadStatus
   # Get the index of the extension
   local tExtensionID
   
   put __extensionCacheID("package_url", pURL) into tExtensionID
   
   # Update progress
   __extensionSendProgressUpdate tExtensionID, "Download Complete", 100
   
   # Start installing by verifying package
   send "__extensionInstallVerify" && tExtensionID to me in 0 milliseconds
end __extensionInstallDownloadComplete

# Verify extension package is valid
on __extensionInstallVerify pExtensionID
   # Update progress
   __extensionSendProgressUpdate pExtensionID, "Verifying extension", 0
   
   # Get the path to the extension
   local tExtensionPath
   put __extensionPropertyGet(pExtensionID, "download_package_path") into tExtensionPath
   
   # Check the package exists
   if there is not a file tExtensionPath then return __extensionError(pExtensionID,"Could not install extension. Package does not exists: " && tExtensionPath)
   
   # Check the file extension is valid
   set the itemdel to "."
   if the last item of tExtensionPath is not "lce" then return __extensionError(pExtensionID,"Could not install extension. The package extension '"&the last item of tExtensionPath&"' Is not valid. Must be 'lce'.")
   
   # Check the manifest contains a name
   local tExtensionName
   put __extensionManifestValue(pExtensionID, "name") into tExtensionName
   if tExtensionName is "error" then return __extensionError(pExtensionID,"Could not install extension. The package manifest must contain a valid name (com.livecode.extensions.<developer_ID>.<extension_name>)")
   __extensionPropertySet pExtensionID, "name", tExtensionName
   
   # Check the manifest contains a version
   local tExtensionVersion
   put __extensionManifestValue(pExtensionID, "version") into tExtensionVersion
   if tExtensionVersion is "error" then return __extensionError(pExtensionID,"Could not install extension. The package manifest must contain a valid version number (1.2.3 - major,minor,maintenance)")
   __extensionPropertySet pExtensionID, "version", tExtensionVersion
   
   # Check the manifest contains an author
   local tExtensionAuthor
   put __extensionManifestValue(pExtensionID, "author") into tExtensionAuthor
   if tExtensionAuthor is "error" then return __extensionError(pExtensionID,"Could not install extension. The package manifest must contain an author")
   __extensionPropertySet pExtensionID, "author", tExtensionAuthor
   
   # Check the manifest contains an type
   local tExtensionType
   put __extensionManifestValue(pExtensionID, "type") into tExtensionType
   if tExtensionType is "error" then return __extensionError(pExtensionID,"Could not install extension. The package manifest must contain a type")
   __extensionPropertySet pExtensionID, "type", tExtensionType
   
   # Check the manifest contains an title
   local tExtensionTitle
   put __extensionManifestValue(pExtensionID, "title") into tExtensionTitle
   if tExtensionTitle is "error" then return __extensionError(pExtensionID,"Could not install extension. The package manifest must contain a title")
   __extensionPropertySet pExtensionID, "title", tExtensionTitle
   
   # Build the type ID from the name and version
   __extensionPropertySet pExtensionID, "type_id", tExtensionName & "." & tExtensionVersion
   
   # Next step
   send "__extensionInstallRemoveOlderVersions" && pExtensionID to me in 0 milliseconds
end __extensionInstallVerify

on __extensionInstallRemoveOlderVersions pExtensionID
   __extensionSendProgressUpdate pExtensionID, "Removing older versions", 7
   
   local tName
   put __extensionPropertyGet(pExtensionID, "name") into tName
   
   repeat for each line tIndex in the keys of sExtensions
      if tIndex is pExtensionID then next repeat
      if sExtensions[tIndex]["name"] is tName then
         revIDEUninstallExtension tIndex
      end if
   end repeat
   
   # Next step
   send "__extensionInstallExtract" && pExtensionID to me in 0 milliseconds
end __extensionInstallRemoveOlderVersions

# Extract the extension files
on __extensionInstallExtract pExtensionID
   # Update progress
   __extensionSendProgressUpdate pExtensionID, "Extracting extension", 15
   
   # Get the path to the package file
   local tExtensionPackageFile
   put __extensionPropertyGet(pExtensionID, "download_package_path") into tExtensionPackageFile
   
   # Create directory to extract into
   local tExtensionDirectory
   put revIDESpecialFolderPath("temp extensions") & slash & __extensionPropertyGet(pExtensionID, "type_id") into tExtensionDirectory
   revIDEEnsurePath(tExtensionDirectory)
   
   # Extract the icon,docs and executable module into the folder
   revZipOpenArchive tExtensionPackageFile, "read"
   
   # Work out the root of the zip
   local tZipItems, tZipRoot
   put revZipEnumerateItems(tExtensionPackageFile) into tZipItems
   if the last char of line 1 of tZipItems is "/" then
      put line 1 of tZipItems into tZipRoot
   else
      put empty into tZipRoot
   end if
   
   # Extract all the files
   local tDirectory
   set the itemdel to slash
   repeat for each line tFile in tZipItems
      put tExtensionDirectory & slash & item 1 to -2 of tFile into tDirectory
      revIDEEnsurePath(tDirectory)
      revZipExtractItemToFile tExtensionPackageFile, tFile, tExtensionDirectory & slash & tFile
   end repeat 
   
   # add path to icon file to the data array
   __extensionPropertySet pExtensionID, "icon", revIDESpecialFolderPath("user extensions") & slash & __extensionPropertyGet(pExtensionID,"type_id") & "/support/icon.png"
   
   revZipCloseArchive tExtensionPackageFile
   
   # Next step
   send "__extensionInstallGuide" && pExtensionID to me in 0 milliseconds
end __extensionInstallExtract

# Install the guide into the IDE
on __extensionInstallGuide pExtensionID
   # Update progress
   __extensionSendProgressUpdate pExtensionID, "Adding user guide to documentation", 30
   
   # Path to user guide
   local tUserGuideFolder
   put revIDESpecialFolderPath("temp extensions") & slash & __extensionPropertyGet(pExtensionID, "type_id") & slash & "docs/guide" into tUserGuideFolder
   
    # Get the name and author required by user guide install function
   local tExtensionName, tExtensionAuthor
   put __extensionPropertyGet(pExtensionID, "name") into tExtensionName
   put __extensionPropertyGet(pExtensionID, "author") into tExtensionAuthor
   
   # Install user guide into the IDE
   revIDEInstallUserGuide tUserGuideFolder, tExtensionName, tExtensionAuthor
   
   send "__extensionInstallAPI" && pExtensionID to me in 500 milliseconds
end __extensionInstallGuide

# Install the extension API into the IDE
on __extensionInstallAPI pExtensionID
   # Update progress
   __extensionSendProgressUpdate pExtensionID, "Adding API to documentation", 45
   
   # Path to user guide
   local tUserAPIFolder
   put revIDESpecialFolderPath("temp extensions") & slash & __extensionPropertyGet(pExtensionID, "type_id") & slash & "docs/api" into tUserAPIFolder
   
   # Get the name and author required by API install function
   local tExtensionName, tExtensionAuthor
   put __extensionPropertyGet(pExtensionID, "name") into tExtensionName
   put __extensionPropertyGet(pExtensionID, "author") into tExtensionAuthor
   
   # Install API into the IDE
   // revIDEInstallAPI tUserAPIFolder, tExtensionName, tExtensionAuthor
   revIDEInstallAPI tUserAPIFolder, tExtensionName, tExtensionAuthor
   
   # Next step
   send "__extensionInstallLoad" && pExtensionID to me in 0 milliseconds
end __extensionInstallAPI

on __extensionInstallLoad pExtensionID
   # Update progress
   __extensionSendProgressUpdate pExtensionID, "Loading extension", 60
   
   # Path to module file
   local tModuleFile
   put revIDESpecialFolderPath("temp extensions") & slash & __extensionPropertyGet(pExtensionID, "type_id") & slash & "module.lcm" into tModuleFile
   
   # Load module file into the engine
   load extension tModuleFile
   //put the result && return &  the loadedextensions
   
   put __extensionProperties(__extensionPropertyGet(pExtensionID, "name")) into sExtensionProperties[ __extensionPropertyGet(pExtensionID, "name")]
   
   # Next step
   send "__extensionInstallMakeLive" && pExtensionID to me in 0 milliseconds
end __extensionInstallLoad

# Moves to temp extracted files into their final live location
on __extensionInstallMakeLive pExtensionID
   # Update progress
   __extensionSendProgressUpdate pExtensionID, "Copying extension into install location", 75
   
   # Get path the directories
   local tTempInstallPath, tFinalPath
   put revIDESpecialFolderPath("temp extensions") & slash & __extensionPropertyGet(pExtensionID, "type_id") into tTempInstallPath
   put revIDESpecialFolderPath("user extensions") into tFinalPath
   __extensionPropertySet pExtensionID, "install_path", tFinalPath & slash & __extensionPropertyGet(pExtensionID, "type_id")
   
   # Copy folder from temp directory into live directory
   revCopyFolder tTempInstallPath,tFinalPath
   
   # Next step
   send "__extensionInstallFinalise" && pExtensionID to me in 0 milliseconds
end __extensionInstallMakeLive

# Delete installation files and original package
on __extensionInstallFinalise pExtensionID
   # Update progress
   __extensionSendProgressUpdate  pExtensionID, "Removing temp files", 90
   
   # Delete temp install folder
   local tTempInstallPath
   put revIDESpecialFolderPath("temp extensions") & slash & __extensionPropertyGet(pExtensionID, "type_id") into tTempInstallPath
   if there is a folder tTempInstallPath then
      revDeleteFolder tTempInstallPath
   end if
   
   # Delete temp package file
   local tTempInstallPackage
   put revIDESpecialFolderPath("downloading extensions") & slash & __extensionPropertyGet(pExtensionID, "type_id") & ".lce" into tTempInstallPackage
   if there is a file tTempInstallPackage then
      revDeleteFolder tTempInstallPackage
   end if
   
   # Update the cache
   __extensionSendProgressUpdate  pExtensionID, "Complete", 100
   __extensionPropertySet pExtensionID, "status", "installed"
   
   # Notify palettes
   __extensionsChanged
end __extensionInstallFinalise

##############################
# PRIVATE UNINSTALL
##############################

# Check if the widget to be uninstalled is in use
on __extensionUninstallCheckInUse pExtensionID
   # Update progress
   if pExtensionID is not a number then __extensionError("Could not remove extension '" & pExtensionID & "' because it is not a valid index") 
   
   __extensionSendProgressUpdate pExtensionID, "Ckecking if extension is in use", 20
   
   # Next step
   send "__extensionUninstallUnload" && pExtensionID to me in 0 milliseconds
end __extensionUninstallCheckInUse

# Unload the extension
on __extensionUninstallUnload pExtensionID
   # Update progress
   __extensionSendProgressUpdate pExtensionID, "Unloading extension", 40
   
   # Get the extension name
   local tName
   put __extensionPropertyGet(pExtensionID, "name") into tName
   
   # Check if it's loaded
   if tName is among the lines of the loadedExtensions then
      unload extension tName
   end if
   
   # Next step
   send "__extensionUninstallDeleteFiles" && pExtensionID to me in 0 milliseconds
end __extensionUninstallUnload

# Delete the files associated to the extension
on __extensionUninstallDeleteFiles pExtensionID
   # Update progress
   __extensionSendProgressUpdate pExtensionID, "Deleting extension files", 60
   
   # Get the extension isntall path
   local tPath
   put __extensionPropertyGet(pExtensionID, "install_path") into tPath
   
   # Make sure the path contains the folder extension as a check before deleting a folder
   if tPath contains "extension" then
      revDeleteFolder tPath
   end if
   
   # Next step
   send "__extensionUninstallDocs" && pExtensionID to me in 0 milliseconds
end __extensionUninstallDeleteFiles

# Remove the guide from the IDE
on __extensionUninstallDocs pExtensionID
   # Update progress
   __extensionSendProgressUpdate pExtensionID, "Removing API and user guide from documentation", 80
   
   revDocsRegenerateBuiltDictionaryData
   
   send "__extensionUninstallUpdateCache" && pExtensionID to me in 0 milliseconds
end __extensionUninstallDocs

# remove from cache
on __extensionUninstallUpdateCache pExtensionID
   # Update progress
   __extensionSendProgressUpdate pExtensionID, "Removing extension data", 90
   
   delete variable sExtensions[pExtensionID]
   
   # Notify IDE uninstallation complete
   __extensionSendProgressUpdate pExtensionID, "Complete", 100
   
   # Update the IDE
   __extensionsChanged
end __extensionUninstallUpdateCache



##############################
# PRIVATE SHARED
##############################

function __extensionProperties pTypeID
   # Get the internal cache ID
   local tIndex
   put __extensionCacheID("name", pTypeID) into tIndex
   
   # Now get the path to the manifest file
   local tManifestPath
   put __extensionPropertyGet(tIndex, "install_path") & slash & "manifest.xml" into tManifestPath
   
   if not there is a file tManifestPath then return empty
   
   # Create the XML tree
   local tXMLTree, tProperties, tExtensionData
   put revXMLCreateTreeFromFile(tManifestPath,true,true,false) into tXMLTree
   put revXMLChildNames(tXMLTree,"package",return,"property",true) into tProperties
   repeat with x = 1 to the number of lines in tProperties
      local tName
      put revXMLAttribute(tXMLTree,"package" & "/" & line x of tProperties,"name") into tName
      put revXMLAttribute(tXMLTree,"package" & "/" & line x of tProperties,"name") into tExtensionData[tName]["label"]
      put revXMLAttribute(tXMLTree,"package" & "/" & line x of tProperties,"get") into tExtensionData[tName]["type"]
      //put "number" into tExtensionData[tName]["type"]
      put 100 into tExtensionData[tName]["order"]
      put "basic" into tExtensionData[tName]["group"]
      put empty into tExtensionData[tName]["default"]
      put true into tExtensionData[tName]["user_visible"]
      put empty into tExtensionData[tName]["options"]
      put empty into tExtensionData[tName]["related"]
      put false into tExtensionData[tName]["read_only"]
   end repeat
   
   revXMLDeleteTree tXMLTree
   return tExtensionData
end __extensionProperties

function __extensionError pExtensionID, pErrorMessage
   __extensionSendProgressUpdate pExtensionID, "Error:" && pErrorMessage, 100
end __extensionError

# Send notication that widget has been added/removed
on __extensionsChanged
    revIDEMessageSend "ideExtensionsChanged"
end __extensionsChanged

# Sent progress update on installation of widget
on __extensionSendProgressUpdate pExtensionID, pMessage, pProgress
   # Get progress information for extension
   __extensionPropertySet pExtensionID, "progress_message", pMessage
   __extensionPropertySet pExtensionID, "progress", pProgress
   
   local tName
   //revputarray sExtensions
   put __extensionPropertyGet(pExtensionID, "name") into tName
   revIDEMessageSend "ideExtensionStatusChanged", (tName & comma & pMessage & comma & pProgress)
   
   //local tCallbackTarget, tCallBackHandler
   //put __extensionPropertyGet(pExtensionID, "callback_target") into tCallbackTarget
   //put __extensionPropertyGet(pExtensionID, "callback_handler") into tCallBackHandler
   //if exists(tCallbackTarget) then
      //dispatch tCallBackHandler to tCallbackTarget with pMessage,pProgress
   //end if
   // Send message to registered targets
end __extensionSendProgressUpdate

# Returns: List of the paths to the installed modules
function __extensionModulePaths
   revIDEStoreDefaultFolder
   
   local tExtensionModulePaths
   set the itemdel to "."
   repeat for each line tExtensionFolder in __extensionFolders()
      set the defaultfolder to tExtensionFolder
      repeat for each line tFolder in the folders
         if tFolder begins with "." then next repeat
         if tFolder is among the items of "downloading,temp,uninstalled" then next repeat
         
         local tExtensionModulePath
         put  tExtensionFolder & slash & tFolder & slash & "module.lcm" into tExtensionModulePath
         if there is a file tExtensionModulePath then
            if tExtensionModulePaths is empty then
               put tExtensionModulePath into tExtensionModulePaths
            else
               put return & tExtensionModulePath after tExtensionModulePaths
            end if
         end if
      end repeat
   end repeat
   
   revIDERestoreDefaultFolder
   
   return tExtensionModulePaths
end __extensionModulePaths

# Returns: List of the packages that are in the directory but not installed
function __extensionPackagePaths
   
end __extensionPackagePaths

# Calls Weservice and returns updates
function __extensionUpdates
end __extensionUpdates

# Returns the ID of the internal array given the extension typeID
private function __extensionCacheID pPropertyToSearch, pValue
   repeat for each line tExtensionID in the keys of sExtensions
      if sExtensions[tExtensionID][pPropertyToSearch] is pValue then
         return tExtensionID
      end if
   end repeat
end __extensionCacheID

# Gets a property from the internal extension cache
function __extensionPropertyGet pExtensionID, pProperty
   return sExtensions[pExtensionID][pProperty]
end __extensionPropertyGet

# Sets a property in the internal extension cache
on __extensionPropertySet pExtensionID, pProperty, pValue
   put pValue into sExtensions[pExtensionID][pProperty]
end __extensionPropertySet

# Loads an extension
on __extensionLoad pExtensionPath
   try
      # Load the extension into the engine
      load extension pExtensionPath
      
      
      set the itemdel to "/"
      put item 1 to -2 of pExtensionPath into pExtensionPath
      
      # Find entry in the cache if it exists
      
      local tIndex
      put __extensionCacheID("install_path", pExtensionPath) into tIndex
      
      # If no extry is in the cache, create one
      if tIndex is not a number then
         put the number of elements of sExtensions + 1 into tIndex
      end if
      
      # Pull key into out of manifest etc into extensions cache
      __extensionPropertySet tIndex, "install_path", pExtensionPath
      
      local tVersion,tName
      put __extensionManifestValueFromFile(tIndex,"name") into tName
      put __extensionManifestValueFromFile(tIndex,"version") into tVersion
      
      __extensionPropertySet tIndex, "name", tName
      __extensionPropertySet tIndex, "version", tVersion
      __extensionPropertySet tIndex, "title", __extensionManifestValueFromFile(tIndex,"title")
      __extensionPropertySet tIndex, "author", __extensionManifestValueFromFile(tIndex,"author")
      __extensionPropertySet tIndex, "type", __extensionManifestValueFromFile(tIndex,"type")
      __extensionPropertySet tIndex, "icon", pExtensionPath & "/support/icon.png"
      __extensionPropertySet tIndex, "type_id", tName & "." & tVersion
      __extensionPropertySet tIndex, "status", "installed"
      __extensionPropertySet tIndex, "label", __extensionManifestValueFromFile(tIndex,"title")
      
      put __extensionProperties(tName) into sExtensionProperties[tName]
   catch tError
      # Think about deleting the module here?
   end try
end __extensionLoad

function __extensionManifestValue pExtensionID, pProperty
   # Get the path to the package file
   local tExtensionPackageFile
   put __extensionPropertyGet(pExtensionID, "download_package_path") into tExtensionPackageFile
   
   if not there is a file tExtensionPackageFile then __extensionsError(pExtensionID,"Could not extract manifest because package was not found in downloads folder")
   
   # A zip can come compressed with a base folder or without. So work out what the 
   # root folder is before trying to extract files
   revZipOpenArchive tExtensionPackageFile, "read"
   
   local tZipItems, tZipRoot
   put revZipEnumerateItems(tExtensionPackageFile) into tZipItems
   
   if the last char of line 1 of tZipItems is "/" then
      put line 1 of tZipItems into tZipRoot
   else
      put empty into tZipRoot
   end if
   
   # Extract the package manfiest to a variable to read key data
   local tManifestData, tManifestXMLTree, tValue
   revZipExtractItemToVariable tExtensionPackageFile, (tZipRoot & "manifest.xml"), "tManifestData"
   
   revZipCloseArchive tExtensionPackageFile
   
   put revXMLCreateTree(tManifestData,true,true,false) into tManifestXMLTree
   put revXMLNodeContents(tManifestXMLTree,"/package/" & pProperty) into tValue
   if tValue begins with "xmlerr" then
      return empty
   else
      return tValue
   end if
end __extensionManifestValue

function __extensionManifestValueFromFile pExtensionID, pProperty
   # Get the path to the manifest file
   local tExtensionInstallPath, tManifestPath
   put __extensionPropertyGet(pExtensionID, "install_path") into tExtensionInstallPath
   put tExtensionInstallPath & slash & "manifest.xml" into tManifestFilepath
   
   if not there is a file tManifestFilepath then __extensionsError(pExtensionID,"Could not get extension manifest information because extension folder does not contain a manifest file")
   
   # Extract the package manfiest to a variable to read key data
   local tManifestXMLTree, tValue
   
   put revXMLCreateTreeFromFile(tManifestFilepath,true,true,false) into tManifestXMLTree
   put revXMLNodeContents(tManifestXMLTree,"/package/" & pProperty) into tValue
   if tValue begins with "xmlerr" then
      return empty
   else
      return tValue
   end if
end __extensionManifestValueFromFile

private function __extensionFolders
   return revIDESpecialFolderPath("user extensions") & return & revIDESpecialFolderPath("extensions") into tExtensionFolders
end __extensionFolders

# Returns a value from the manifest
//private on __extensionManifestValue pExtensionID, pManifestKey
//end __extensionManifestValue


##############################
# CALLBACKS
##############################

on extensionUpdateDataReceived
end extensionUpdateDataReceived




