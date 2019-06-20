script "revDeployLibraryAndroid"
local sAndroidRoot
local sJavaRoot

local sAvailableDevices

////////////////////////////////////////////////////////////////////////////////

command deployInitialize
   deployAutoconfigure
   
   if sAndroidRoot is not empty then
      revAndroidSetADBPath pathToAdb()
   else
      revAndroidSetADBPath empty
      androidDevicesChanged
   end if
end deployInitialize

function deployListTargets
   local tTargets
   get the keys of sAvailableDevices
   sort it
   repeat for each line tTarget in it
      put "Android" && tTarget & return after tTargets
   end repeat
   delete the last char of tTargets
   return tTargets
end deployListTargets

function deployIsAvailable
   local tEnabled
   
   if sAvailableDevices is not an array then
      put false into tEnabled
   else
      put true into tEnabled
   end if
   
   return tEnabled
end deployIsAvailable

command deployDo pTargetStack, pTargetDevice
   local tError
   put empty into tError
   
   local tSettings
   put revSBGetSettings(pTargetStack, true) into tSettings
   
   local tIdentifier
   put tSettings["android,identifier"] into tIdentifier
   if tIdentifier is empty then
      put "com.yourcompany.yourapp" into tIdentifier
   end if
   
   if tError is empty then
      revStandaloneProgress "Checking target device architecture..."
      
      executeShellCommand pathToAdb(), "-s", word 2 to -1 of pTargetDevice, "shell", "getprop", "ro.product.cpu.abi"
      local tABI
      put word 1 of line 1 of the result into tABI
      if not (tABI is among the items of "armeabi-v7a,arm64-v8a,x86,x86_64") then
         put "Checking target device architecture failed:" && the result into tError
      end if
      
      local tArch
      put __ABIArch(tABI) into tArch
      repeat for each item tItem in revSBPlatformArchitectures("Android")
         put tItem is tArch into tSettings["Android",tItem]
      end repeat
   end if
   
   local tTempApkFIle
   if tError is empty then
      // SN-2014-10-27: [[ Bug 13800 ]] The APK must be "specified as a full path to an .apk file"
      put tempName() & ".apk" into tTempApkFIle
      try
         dispatch "revSaveAsMobileStandalone" to stack "revSaveAsAndroidStandalone" with \
               pTargetStack, tTempApkFile, "Test", tSettings
      catch tException
         put "Unable to build app for testing:" && tException into tError
      end try
   end if
   
   if tError is empty then
      revStandaloneProgress "Installing app on target device..."
      
      executeShellCommand pathToAdb(), "-s", word 2 to -1 of pTargetDevice, "install", "-r", tTempApkFile
      if not (the result contains "Success") then
         put "Installation of app failed:" && the result into tError
      end if
   end if
   
   if tError is empty then
      revStandaloneProgress "Launching app on target device..."
      
      executeShellCommand pathToAdb(), "-s", word 2 to -1 of pTargetDevice, "shell", "am", "start", "-n", tIdentifier & "/.mblandroid"
      if the result contains "Error:" then
         put "Launch of app failed:" && the result into tError
      end if
   end if
   
   if "revStandaloneProgress" is among the lines of the openStacks then
      close stack "revStandaloneProgress"
   end if
   
   if there is a file tTempApkFile then
      delete file tTempApkFile
   end if
   
   return tError
end deployDo

private function __ABIArch pABI
   switch pABI
      case "armeabi"
         return "armv6"
      case "armeabi-v7a"
         return "armv7"
      case "arm64-v8a"
         return "arm64"
      case "x86"
      case "x86_64"
         return pABI
   end switch
end __ABIArch

command deployAutoconfigure
   local tSDK
   put the cMobileSupport["android.sdk"] of stack "revPreferences" into tSDK
   if tSDK is not empty and not deployIsValidSDK(tSDK) then
      put empty into tSDK
   end if
   set the cMobileSupport["android.sdk"] of stack "revPreferences" to tSDK
   put tSDK into sAndroidRoot
   
   if sAndroidRoot is not empty then
      local tJDK
      put the cMobileSupport["android.jdk"] of stack "revPreferences" into tJDK
      if tJDK is empty or not deployIsValidJDK(tJDK) then
         deployAutoconfigureJDK
         put the result into tJDK
      end if
      if tJDK is not empty then
         set the cMobileSupport["android.jdk"] of stack "revPreferences" to tJDK
         put tJDK into sJavaRoot
      end if
   else
      put empty into sJavaRoot
   end if
end deployAutoconfigure

command deployAutoconfigureJDK
   local tJDK
   switch the platform
      case "win32"
         configureJavaRootWindows
         put the result into tJDK
         break
      case "macos"
         put line 1 of shell("/usr/libexec/java_home") into tJDK
         break
      case "linux"
         break
   end switch
   
   if deployIsValidJDK(tJDK) then
      return tJDK
   end if
   
   return empty
end deployAutoconfigureJDK

command deployAutoconfigureSDK
   return empty
end deployAutoconfigureSDK

on androidDevicesChanged
   local tDevices
   put revAndroidListDevices() into tDevices
   
   put empty into sAvailableDevices
   repeat for each line tDevice in tDevices
      if tDevice contains "no serial number" then
         put quote&quote into word 1 to -2 of tDevice
      end if
      put word 2 of tDevice into sAvailableDevices[word 1 of tDevice]
   end repeat
   
   revIDEDeployTargetsChanged
end androidDevicesChanged

////////////////////////////////////////////////////////////////////////////////

function deployIsValidJDK pPath
   if there is no folder pPath then
      --return "could not find registered jdk at" && tJavaHome
      return false
   end if
   
   if there is no file pathToJavaC(pPath) then
      --return "missing javac tool, make sure your JDK install has not become corrupted"
      return false
   end if
   
   // newer versions of SDK Tools require Java 8
   if pathToSDKClasses() contains "sdklib-" and not pPath contains "jdk1.8" then
      answer error "The version of Java installed in your system is not compatible with the current version of Android SDK Tools. Please install Java 8 to continue."
      return false
   end if
   
   return true
end deployIsValidJDK

constant kDeployAndroidTargetSDK = "28"
function deployTargetSdkVersion
   return kDeployAndroidTargetSDK
end deployTargetSdkVersion

function deployRequiredSDK
   local tMap
   put deployGetVersionsMap() into tMap
   local tAndroidVersion
   put word 1 of tMap[kDeployAndroidTargetSDK] into tAndroidVersion
   return format("%s (API %s)", tAndroidVersion, kDeployAndroidTargetSDK)
end deployRequiredSDK

function deployIsValidSDK pPath 
   if there is no folder (pPath & slash & "platforms") or \
         there is no folder (pPath & slash & "platform-tools") or \
         there is no folder (pPath & slash & "tools") then
      --return "could not find platforms, platform-tools or tools folders at that path."
      return false
   end if
   
   if there is no folder pathToRootPlatform(pPath) then
      --return "could not find install of required target SDK, make sure it has been installed with the Android SDK Manager"
      return false
   end if
   
   if there is no file pathToRootClasses(pPath) then
      --return "missing root classes, make sure your SDK install has not become corrupted"
      return false
   end if
   
   if there is no file pathToAapt(pPath) then
      --return "missing aapt tool, make sure your SDK install has not become corrupted"
      return false
   end if
   
   if there is no file pathToAdb(pPath) then
      --return "missing adb tool, make sure your SDK install has not become corrupted"
      return false
   end if
   
   if there is no file pathToSDKClasses(pPath) then
      --return "missing sdklib.jar, make sure your SDK install has not become corrupted"
      return false
   end if
   
   if there is no file pathToZipAlign(pPath) then
      --return "missing zipalign tool, make sure your SDK install has not become corrupted"
      return false
   end if
   
   if there is no file pathToDex(pPath) then
      --return "missing dex tool, make sure your SDK install has not become corrupted"
      return false
   end if
   
   return true
end deployIsValidSDK

function deployGetSDK
   return sAndroidRoot
end deployGetSDK

function deployGetJDK
   return sJavaRoot
end deployGetJDK

////////////////////////////////////////////////////////////////////////////////

function deployGetVersionsMap
   local tMap
   put "4.1 - Jelly Bean" into tMap[16]
   put "4.2" into tMap[17]
   put "4.3" into tMap[18]
   put "4.4 - KitKat" into tMap[19]
   put "5.0 - Lollipop" into tMap[21]
   put "5.1" into tMap[22]
   put "6.0 - Marshmallow" into tMap[23]
   put "7.0 - Nougat" into tMap[24]
   put "7.1" into tMap[25]
   put "8.0 - Oreo" into tMap[26]
   put "8.1" into tMap[27]
   put "9.0 - Pie" into tMap[28]
   put "10 - Q" into tMap[29]
   return tMap
end deployGetVersionsMap

constant kDefaultMinAPIVersion = 9
function deployGetApiFromVersion pVersion
   local tAPI
   put kDefaultMinAPIVersion into tAPI
   
   local tMap
   put deployGetVersionsMap() into tMap
   repeat for each key tAPIKey in tMap
      if pVersion is tMap[tAPIKey] then
         put tAPIKey into tAPI
         exit repeat
      end if
   end repeat
   
   return tAPI
end deployGetApiFromVersion

////////////////////////////////////////////////////////////////////////////////

private command executeShellCommand
   local tShell
   repeat with i = 2 to the paramCount
      if param(i) contains space and not (param(i) begins with "-") then
         put quote & param(i) & quote & space after tShell
      else
         put param(i) & space after tShell
      end if
   end repeat
   
   get doShellCommand(param(1), tShell)
   
   return it
end executeShellCommand

private function doShellCommand pCommand, pArguments
   local tCommandPath
   set the itemDelimiter to slash
   put item 1 to -2 of pCommand into tCommandPath
   
   local tOldPath
   put $PATH into tOldPath
   if the platform is "win32" then
      replace slash with backslash in tCommandPath
      put tCommandPath & ";" & sJavaRoot & "\bin" & ";" before $PATH
   else if the platform is "macos" then
      put tCommandPath & ":" before $PATH
   else if the platform is "linux" then
      put tCommandPath & ":" & sJavaRoot & "/bin" & ":" before $PATH
   end if
   
   set the hideConsoleWindows to true
   get shell(item -1 of pCommand && pArguments)
   
   put tOldPath into $PATH
   
   return it
end doShellCommand

////////////////////////////////////////////////////////////////////////////////

function pathToRootPlatform pRoot
   if pRoot is empty then
      put sAndroidRoot into pRoot
   end if

   return pRoot & slash & "platforms/android-" & kDeployAndroidTargetSDK
end pathToRootPlatform

function pathToRootClasses pRoot
   return pathToRootPlatform(pRoot) & slash & "android.jar"
end pathToRootClasses

function pathToSDKClasses pRoot
   if pRoot is empty then
      put sAndroidRoot into pRoot
   end if
   local tLibs,tSdkLib
   put files(pRoot&"/tools/lib") into tLibs
   if tLibs contains "sdklib.jar" then 
      return pRoot & slash & "tools/lib/sdklib.jar"
   else
      filter tLibs with "sdklib-[0-9]*" into tSdkLib
      if tSdkLib is empty then return empty
      return pRoot & "/tools/lib/" & tSdkLib
   end if
end pathToSDKClasses

function pathToCommonClasses pRoot
   if pRoot is empty then
      put sAndroidRoot into pRoot
   end if
   local tCommonLib
   filter files(pRoot&"/tools/lib") with "common-*" into tCommonLib
   if tCommonLib is empty then return empty
   return pRoot & slash & "tools/lib/" & tCommonLib
end pathToCommonClasses

private function searchTools pRoot, pCommand
   // search the shared tools folder
   if there is not a folder pRoot & slash & "tools" then return empty
   
   local tPath
   
   put pRoot & slash & "tools" & slash & pCommand into tPath
   
   if there is a file tPath then
      return tPath
   else
      return empty
   end if
end searchTools

function searchBuildTools pRoot, pCommand
   // the build tools folder contains version subfolders containing the actual executables,
   // so we search through each of them starting with the most recent
   if there is not a folder pRoot & slash & "build-tools" then return empty
   
   local tOldFolder, tFolders, tOldItem
   
   put the folder into tOldFolder
   set the folder to pRoot & slash & "build-tools"
   
   put the folders into tFolders
   
   set the folder to tOldFolder
   
   if tFolders is empty then return empty
   
   put the itemdelimiter into tOldItem
   set the itemdelimiter to "."
   
   // sort version folders descending by x.x.x version numbers
   sort lines of tFolders descending numeric by item 3 of each
   sort lines of tFolders descending numeric by item 2 of each
   sort lines of tFolders descending numeric by item 1 of each
   
   set the itemdelimiter to tOldItem
   
   local tPath
   repeat for each line tVersion in tFolders
      put pRoot & slash & "build-tools" & slash & tVersion & slash & pCommand into tPath
      if there is a file tPath then return tPath
   end repeat
   
   return empty
end searchBuildTools

function searchMobileRuntimeFolder pCommand
   local tPath
   put revEnvironmentUserRuntimePath() & slash & "Android" & slash & pCommand into tPath
   if there is a file tPath then return tPath
   
   put revEnvironmentRuntimePath() & slash & "Android" & slash & pCommand into tPath
   if there is a file tPath then return tPath
   
   return empty
end searchMobileRuntimeFolder

private function executableName pCommand
   if the platform is "win32" then
      return pCommand & ".exe"
   else
      return pCommand
   end if
end executableName

private function pathToPlatformTool pRoot, pCommand
   if pRoot is empty then
      put sAndroidRoot into pRoot
   end if
   
   -- IM-2013-05-21: Update to look in "build-tools/<version>" folder
   local tPath
   put searchBuildTools(pRoot, pCommand) into tPath
   if tPath is not empty then return tPath
   
   -- MM-2011-08-09: Update to look in platform-tools folder if not present in platforms/android-8/tools/
   if there is a file pRoot & slash & "platforms/android-8/tools/" & pCommand then
      return pRoot & slash & "platforms/android-8/tools/" & pCommand
   end if
   
   return pRoot & slash & "platform-tools/" & pCommand
end pathToPlatformTool

function pathToAapt pRoot
   return pathToPlatformTool(pRoot, executableName("aapt"))
end pathToAapt

function pathToAdb pRoot
   if pRoot is empty then
      put sAndroidRoot into pRoot
   end if
   return pRoot & slash & "platform-tools" & slash & executableName("adb")
end pathToAdb

function pathToZipAlign pRoot
   if pRoot is empty then
      put sAndroidRoot into pRoot
   end if
   
   local tExe
   put executableName("zipalign") into tExe
   
   // IM-2014-07-11: [[ Bug 12719 ]] First search the tools folder
   local tPath
   put searchTools(pRoot, tExe) into tPath
   if tPath is not empty then return tPath
   
   // IM-2014-07-11: [[ Bug 12719 ]] zipalign was moved into build-tools as of version 19.1.0 so search there as well
   put searchBuildTools(pRoot, tExe) into tPath
   if tPath is not empty then return tPath
   
   return pRoot & slash & "tools" & slash & tExe
end pathToZipAlign

function pathToDex pRoot
   local tCommand = "dx"
   if the platform is "win32" then put ".bat" after tCommand
   
   return pathToPlatformTool(pRoot, tCommand)
end pathToDex

function pathToJavaC pRoot
   if pRoot is empty then
      put sJavaRoot into pRoot
   end if
   return pRoot & slash & "bin" & slash & executableName("javac")
end pathToJavaC

function pathToJava pRoot
   if pRoot is empty then
      put sJavaRoot into pRoot
   end if
   return pRoot & slash & "bin" & slash & executableName("java")
end pathToJava

function pathToJarSigner pRoot
   if pRoot is empty then
      put sJavaRoot into pRoot
   end if
   return pRoot & slash & "bin" & slash & executableName("jarsigner")
end pathToJarSigner

function pathToJar pRoot
   if pRoot is empty then
      put sJavaRoot into pRoot
   end if
   return pRoot & slash & "bin" & slash & executableName("jar")
end pathToJar

function pathToPlayServices  pRoot
   if pRoot is empty then
      put sAndroidRoot into pRoot
   end if
   return pRoot & slash & "extras/google/m2repository/com/google/android/gms"
end pathToPlayServices

////////////////////////////////////////////////////////////////////////////////

private command configureJavaRootWindows
   local tVersion, tIs64Bit
   put true into tIs64Bit
   put getRegistryValue("HKEY_LOCAL_MACHINE\SOFTWARE\JavaSoft\Java Development Kit", "CurrentVersion", tIs64Bit) into tVersion
   if tVersion is empty then
      put false into tIs64Bit
      put getRegistryValue("HKEY_LOCAL_MACHINE\SOFTWARE\JavaSoft\Java Development Kit", "CurrentVersion", tIs64Bit) into tVersion
   end if
   
   local tJavaHome
   put getRegistryValue("HKEY_LOCAL_MACHINE\SOFTWARE\JavaSoft\Java Development Kit\" & tVersion, "JavaHome", tIs64Bit) into tJavaHome
   
   return tJavaHome
end configureJavaRootWindows

private function getRegistryValue pKey, pValue, pUse64Bit
   local tRegCmd, tValue
   if pUse64Bit then
      put $SystemRoot & slash & "sysnative" & slash & "reg.exe" into tRegCmd
      if there is no file tRegCmd then
         return empty
      end if
   else
      put "reg.exe" into tRegCmd
   end if
   set the hideConsoleWindows to true
   
   // IM 12/05/2011 - added check for error return from reg.exe
   // reg.exe returns an error if the key or value is not found
   
   //return word 3 to -1 of the last line of (word 1 to -1 of shell(tRegCmd && "QUERY" && quote & pKey & quote && "/v" && quote & pValue & quote))
   put the last line of (word 1 to -1 of shell(tRegCmd && "QUERY" && quote & pKey & quote && "/v" && quote & pValue & quote)) into tValue
   if word 1 of tValue is "ERROR:" then
      return empty
   end if
   return word 3 to -1 of tValue
end getRegistryValue

private function listRegistryRecursive pKey, pUse64Bit
   local tRegCmd
   if pUse64Bit then
      put $SystemRoot & slash & "sysnative" & slash & "reg.exe" into tRegCmd
   else
      put empty into tRegCmd
   end if
   if there is no file tRegCmd then
      put "reg.exe" into tRegCmd
   end if
   
   local tKeys
   put word 1 to -1 of shell(tRegCmd && "QUERY" && quote & pKey & quote && "/s") into tKeys
   
   local tCurrentKey, tKeyArray
   repeat for each line tLine in tKeys
      put word 1 to -1 of tLine into tLine
      if tLine is empty then
         next repeat
      end if
      
      if tLine begins with " " then
         put word 3 to -1 of tLine into tKeyArray[tCurrentKey]
      else
         put tLine into tCurrentKey
         delete char 1 to the number of chars of pKey + 1 of tCurrentKey
      end if
   end repeat
   
   return tKeyArray
end listRegistryRecursive
