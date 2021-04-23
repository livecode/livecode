script "InstallerInstallerUIUpdateDownloadCardBehavior"
local sFileName

on preOpenCard
   setTitleField the long id of field "Page Title", "Downloading Update", "LiveCode Updater"
   set the uEnabled of control "Cancel" to true
   set the uLabel of control "Cancel" to "Cancel"
   set the uDefault of control "Cancel" to false
   show control "Cancel"
   hide control "Back"
   hide control "Next"
   
   put empty into field "Page Description"
   set the uProgress of control "Progress" to 0
   put "Downloading..." into field "Phase"
   
   downloadNewVersion the uRemoteURL of me, the uRemoteChecksum of me, getUpdateFolderLocation()
end preOpenCard

on closeCard
   resetAll
end closeCard

private command downloadNewVersion pUrl, pCheckSum, pDestination
   -- HSC - get the path and name of the download destination
   --
   local tFileName
   set the itemDel to slash
   put the last item of pURL into tFileName
   put pDestination & slash & tFileName into sFileName
   
   -- MM - check to see if if there is a cached version and its checkum matches the server version
   -- if not, download latest revison
   --
   if there is no file sFileName or not md5CheckSumValidate(sFileName, pCheckSum) then       
            
      -- MM - do some clean up, removing all old cached installers
      -- preserve the last update check file though
      --
      local tOldFolder
      put the folder into tOldFolder
      set the itemDel to slash
      set the folder to item 1 to -1 of pDestination
      repeat for each line tFile in the files
         if not (tFile begins with ".") and not (tFile begins with "update_") then
            delete file the folder & slash & tFile
            if the result is not empty then
               return the result
            end if 
         end if
      end repeat
      set the folder to tOldFolder
      
      -- MM - set progress callbacks and begin download
      --
      set the itemDel to slash
      put urlEncode(the last item of pURL) into the last item of pURL
      set the itemDel to comma
      libURLSetStatusCallback "downloadLatestRevisionUpdates", the long id of me
      libURLDownloadToFile pUrl, sFileName, "downloadLatestRevisionComplete"
   else
      
      -- The file already exists execute it
      --
      launchDownloadedInstaller
      if the result is not empty then
         set the uError of card "Finish" to the result
         go to card "Finish"
      else
         actionCancel
      end if   
         
   end if
end downloadNewVersion

-- HSC - test when a download is complete
--
on downloadLatestRevisionComplete pURL, pStatus
   if pStatus is among the words "downloaded cached" then
      launchDownloadedInstaller
      if the result is not empty then
         set the uError of card "Finish" to the result
         go to card "Finish"
      else
         actionCancel
      end if  
   else
      set the uError of card "Finish" to "Error downloading new version"
      go to card "Finish"
   end if
end downloadLatestRevisionComplete

-- MM - update the UI with the instaler download progress
--
on downloadLatestRevisionUpdates pURL, pStatus
   if item 1 of pStatus is "loading" then
      set the uProgress of control "Progress" to ((item 2 of pStatus / item 3 of pStatus) * 100)
   else if pStatus is among the words "error timeout" then
      set the uError of card "Finish" to "Error downloading new version"
      go to card "Finish"
   end if
end downloadLatestRevisionUpdates

-- HSC - launch the installer that was just downloaded
--
private command launchDownloadedInstaller
   put "Launching..." into field "Phase"   
   if sFileName is not empty and there is a file sFileName then
      local tCommand
      if the platform is "MacOS" then
         
         -- For OS X the installer will be a DMG.
         -- Mount the DMG and extract the name of mounted volume.
         --
         local tMountedVolumes, tVolumePath
         put shell ("hdiutil mount" && quote & sFileName & quote) into tMountedVolumes
         put empty into tVolumePath
         set the itemdelimiter to tab
         repeat for each line tLine in tMountedVolumes
            repeat for each item tItem in tLine
               if tItem contains "LiveCode" then
                  put tItem into tVolumePath
               end if
            end repeat
         end repeat
         set the itemDel to comma
         
         -- If we have successfuly mounted the DMG and found the volume then extract the app path 
         -- from the DMG and build the launch command, remebering to tell the installer to unmount when done.
         --
         if tVolumePath is not empty and there is a folder tVolumePath then
            local tOldFolder
            put the folder into tOldFolder
            set the folder to tVolumePath
            get the folders
            set the folder to tOldFolder
            filter it with "*.app"
            if the number of lines in it is not 1 then
               return "Could not find installer within DMG"
            end if
            put tVolumePath & "/" & it & "/Contents/MacOS/installer" into tCommand
            put quote & tCommand & quote && "install" && the loc of this stack && "-unmount" into tCommand
         else 
            return "Could not mount downloaded disk image."
         end if
         
      else
         
         switch the platform
            case "Win32"
               put quote & sFileName & quote && "install" &&  the loc of this stack into tCommand
               break
            case "linux"
               local tChmodResult
               get shell("chmod +x " & quote & sFileName & quote)
               if it is not empty then
                  return "Launching downloaded installer - Setting executable permission failed - " & it
               end if
               put sFileName && "install" &&  the loc of this stack into tCommand
               break
         end switch
         
      end if
      
      -- Attempt to launch the installer.
      --
      open process tCommand for neither
      if the result is not empty then
         return "Could not launch downloaded installer:"  && the result
      end if      
        
      return empty
      
   end if
   
   return "Could not find downloaded installer"
end launchDownloadedInstaller

-- MM - returns true if the md5 check sum for the file matches the check sum passed
--
private function md5CheckSumValidate pFile, pCheckSum
   return md5CheckSum(pFile) is pCheckSum
end md5CheckSumValidate

-- MM - returns the md5 check sum for the given file
--
private function md5CheckSum pFile
   if there is a file pFile then
      local tCheckSum 
      get md5Digest(URL ("binfile:" & pFile))
      get binaryDecode("H*", it, tCheckSum) 
      return tCheckSum      
   end if
end md5CheckSum
