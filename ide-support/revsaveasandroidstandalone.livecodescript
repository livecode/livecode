script "revSaveAsAndroidStandalone"
################################################################################

local sJavaRoot

################################################################################

command revSaveAsMobileStandalone pStack, pApkFile, pTarget, pSettings
   local tPath
   put the cMobileSupport["android.sdk"] of stack "revPreferences" into tPath
   
   if tPath is empty then
      throw revIDELocalise("In order to build Android standalones" & \ 
            " you need to choose the location of your Android development SDK" & \
            " in Preferences -> Mobile Support")
   end if
   if not revIDEDeployAndroidIsValidSDK(tPath) then
      throw revIDELocalise("The chosen folder in Preferences -> Mobile Support" & \
            " is not a valid Android SDK. Please ensure you have installed it correctly," & \
            " and enabled support for") && revIDEDeployAndroidRequiredSDK()
   end if
   
   local tTargetType
   if pTarget contains "build" then
      put "android release" into tTargetType
   else
      put "android test" into tTargetType
   end if
   
   try
      put the cMobileSupport["android.jdk"] of stack "revPreferences" into sJavaRoot
      dispatch "savingMobileStandalone" to stack pStack with tTargetType, pApkFile
   end try
   
   try
      revSaveAsMobileStandaloneMain pStack, pApkFile, pTarget, pSettings
   catch tError
   end try
   
   try
      if tError is not empty then
         put empty into pApkFile
      end if
      dispatch "mobileStandaloneSaved" to stack pStack with tTargetType, pApkFile
   end try
   
   if tError is not empty then
      throw tError
   end if
end revSaveAsMobileStandalone

private function pathIsRelative pPath
   if pPath is empty then throw "empty path"
   if pPath begins with "/" then return false
   if char 2 of pPath is ":" then return false
   
   return true
end pathIsRelative

// IM-2013-05-21: [[ BZ 10904 ]] This function replicates the functionality of the apkbuilder script which has now been removed from the Android SDK platform tools
private command buildAPK pUnalignedApkFile, pUnsignedParam, pAssetArchive, pClassesFile, pLibsBuildFolder
   local tResult, tSep
   if the platform is "Win32" then 
      put ";" into tSep
   else
      put ":" into tSep
   end if
   
   // We need the 'common' classes to fix the following exception
   /*
   Exception in thread "main" java.lang.NoClassDefFoundError: com/android/prefs/AndroidLocation
	at com.android.sdklib.internal.build.DebugKeyProvider.getDefaultKeyStoreOsPath(DebugKeyProvider.java:122)
	at com.android.sdklib.build.ApkBuilder.getDebugKeystore(ApkBuilder.java:944)
	at com.android.sdklib.build.ApkBuilderMain.main(ApkBuilderMain.java:129)
    Caused by: java.lang.ClassNotFoundException: com.android.prefs.AndroidLocation
   */
   executeShellCommand pathToJava(), "-Xmx128M", "-classpath", pathToSDKClasses() & tSep & pathToCommonClasses(), "com.android.sdklib.build.ApkBuilderMain", pUnalignedApkFile, pUnsignedParam, "-v", "-z", pAssetArchive, "-f", pClassesFile, "-nf", pLibsBuildFolder
   put the result into tResult
   return tResult
end buildAPK

private function updateMinimumAPI pVersionSet, pLimit
   local tLimitType
   if pVersionSet < pLimit then
      put "lowest" into tLimitType
   else
      put "highest" into tLimitType
   end if
   
   answer "The minimum Android API is currently set to" && \
         pVersionSet && \
         "but the" && tLimitType & " Android API available from LiveCode" && the version && \
         "is" && pLimit & "." & LF & \
         "Do you want to update the minimum Android API to" && pLimit & "?" \
         with "No" or "Yes" \
         titled "Minimum Android API error"
   
   return (it is "Yes")
end updateMinimumAPI

private function badAndroidApiMessage
   return "Not a valid Android API"
end badAndroidApiMessage

private function __ArchABI pArch
   switch pArch
      case "armv6"
         return "armeabi"
      case "armv7"
         return "armeabi-v7a"
      case "arm64"
         return "arm64-v8a"
      case "x86"
      case "x86_64"
         return pArch
   end switch
end __ArchABI

constant kAndroidExternals = "revzip revxml dbsqlite dbmysql dbodbc dbpostgresql revpdfprinter"

private command revSaveAsMobileStandaloneMain pStack, pApkFile, pTarget, pSettings
   local tArchABIs, tArchs
   repeat for each item tItem in revSBPlatformArchitectures("Android")
      if pSettings["android",tItem] then
         put true into tArchABIs[__ArchABI(tItem)]
         put true into tArchs[tItem]
      end if
   end repeat
   if the number of elements of tArchABIs is 0 then
      put true into tArchABIs["armeabi-v7a"]
      put true into tArchs["armv7"]
   end if
   combine tArchABIs by comma as set
   combine tArchs by comma as set
   
   dispatch "revIDEDeployAndroidInitialize" to stack "revDeployLibrary"
   
   -- If we're searching for inclusions, do it here 
   local tConfirm, tResult
   put true into tConfirm
   if pSettings["inclusions"] is "search" then 
      put false into tConfirm
      -- search for inclusions when building for simulator
      if pTarget contains "test" then
         revSBSearchForInclusions pStack, pSettings
      end if
      
   end if
   
   -- Make sure all dependencies are included
   revSBUpdateForDependencies "android", tArchs, , pSettings
   revSBEnsurePerExtensionSettings "android", pSettings
   put the result into tResult
   if tResult is not empty then
      throw tResult
   end if
   
   -- Manually remove built-in implementations from detected script library inclusions
   revSBRemoveInclusions pStack, "android", tConfirm, pSettings
   
   -- Make sure old-style keys are retained, and updated with new information
   -- from inclusions pane of standalone settings GUI
   revSBConvertSettingsForPlatform pSettings, "android"
   
   -- Compute the base folder
   local tBaseFolder
   set the itemDelimiter to slash
   put item 1 to -2 of the effective filename of stack pStack into tBaseFolder
   set the itemDelimiter to comma
   
   -- Fetch the various options we need to initially build the app-bundle
   local tName, tVersionName, tVersionCode, tExternals, tDrivers, tIcon, tSplash, tKey, tUnsigned, tStatusBarIcon
   put pSettings["name"] into tName
   put pSettings["android,icon"] into tIcon
   put pSettings["android,key"] into tKey
   
   if pSettings["android,label"] is empty then
      put tName into pSettings["android,label"]
   end if
   if pSettings["android,identifier"] is empty then
      put "com.yourcompany.yourapp" into pSettings["android,identifier"]
   end if
   if pSettings["android,version name"] is empty then
      put "1.0.0" into pSettings["android,version name"]
   end if
   if pSettings["android,version code"] is empty then
      put "1" into pSettings["android,version code"]
   end if
   if pSettings["android,initial orientation"] is empty then
      put "portrait" into pSettings["android,initial orientation"]
   end if
   if pSettings["android,status bar hidden"] is empty then
      put "false" into pSettings["android,status bar hidden"]
   end if
   
   // SN-2015-05-13: [[ AndroidVersions ]] Make sure that the minimum version is
   // the minimum version that we can use.
   local tMinimumVersion, tMaximumVersion
   put item 1 of revIDEDeployAndroidGetApisList() into tMinimumVersion
   put last item of revIDEDeployAndroidGetApisList() into tMaximumVersion
   
   if pSettings["android,minimum version"] is empty then
      put tMinimumVersion into pSettings["android,minimum version"]
   else if pSettings["android,minimum version"] < tMinimumVersion then
      // In case the minimum version is still 2.2 (for instance), we warn the user
      //  that we want to update it, before doing so.
      if updateMinimumAPI(pSettings["android,minimum version"], tMinimumVersion) then
         put tMinimumVersion into pSettings["android,minimum version"]
      else
         throw badAndroidAPIMessage()
      end if
   else if pSettings["android,minimum version"] > tMaximumVersion then
      if updateMinimumAPI(pSettings["android,minimum version"], tMaximumVersion) then
         put tMaximumVersion into pSettings["android,minimum version"]
      else
         throw badAndroidAPIMessage()
      end if
   end if
   
   put pSettings["android,statusBarIcon"] into tStatusBarIcon
   if tStatusBarIcon is empty then
      // put default sb icon path into tStatusBarIcon
      // SN-2015-01-22: [[ Bug 13213 ]] If mapFilePath returns empty, an error will be triggered
      // line 629.
      put mapFilePath(revMobileRuntimeFolder(pTarget) & slash & "notify_icon.png") into tStatusBarIcon
   end if
   
   switch pSettings["android,signing"]
      case "sign with my key"
         put true into tUnsigned
         break
      case "sign for development only"
         put false into tUnsigned
         put empty into tKey
         break;
      case "do not sign"
         put true into tUnsigned
         put empty into tKey
         break;
      default
         put true into tUnsigned
         break;
   end switch
   
   -- Compute the icon path
   if tIcon is not empty and pathIsRelative(tIcon) then
      put tBaseFolder & slash before tIcon
   end if
   
   -- Compute the key path
   if tKey is not empty and pathIsRelative(tKey) then
      put tBaseFolder & slash before tKey
   end if
   
   -- Compute the status bar icon path
   if tStatusBarIcon is not empty and pathIsRelative(tStatusBarIcon) then
      put tBaseFolder & slash before tStatusBarIcon
   end if
   
   -- Compute the externals list (includes drivers on Android)
   repeat for each word tExternal in kAndroidExternals
      if pSettings["android,include" && tExternal] then
         if tExternal begins with "db" then 
            if "revdb" is not among the lines of tExternals then
               put "revdb" & return after tExternals
            end if
            put tExternal & return after tExternals
         else if tExternal is among the words of "revpdfprinter" then
            put tExternal & return after tDrivers
         else
            put tExternal & return after tExternals
         end if
      end if
   end repeat
   delete the last char of tExternals
   delete the last char of tDrivers
   
   -- Compute the stackfiles list
   lock messages
   local tStackFiles
   revSBRelativeStackFilesList pStack, tStackFiles
   unlock messages
   
   -- Make sure the app-bundle isn't already there
   if there is a file pApkFile then
      delete file pApkFile
      if there is a file pApkFile then
         throw "unable to remove existing apk"
      end if
   end if
   
   try
      ------- Create the build folder
      local tBuildFolder, tResBuildFolder, tDrawableResBuildFolder, tLayoutResBuildFolder
      local tLibsBuildFolder, tClassesBuildFolder, tXMLResBuildFolder
      put tempName() into tBuildFolder
      create folder tBuildFolder
      if the platform is "win32" then
         put shortFilePath(tBuildFolder) into tBuildFolder
      end if
      
      put tBuildFolder & slash & "classes_app" into tClassesBuildFolder
      put tBuildFolder & slash & "lib" into tLibsBuildFolder
      
      put tBuildFolder & slash & "res-build/res" into tResBuildFolder
      put tResBuildFolder & slash & "drawable" into tDrawableResBuildFolder
      put tResBuildFolder & slash & "layout" into tLayoutResBuildFolder
      put tResBuildFolder & slash & "xml" into tXMLResBuildFolder
      create folder tClassesBuildFolder
      create folder tLibsBuildFolder
      
      local tArchLibsBuildFolders
      repeat for each item tArch in tArchABIs
         put tLibsBuildFolder & slash & tArch into tArchLibsBuildFolders[tArch]
         create folder tArchLibsBuildFolders[tArch]
         if there is not folder tArchLibsBuildFolders[tArch] then
            throw "could not create build folders"
         end if
      end repeat
      
      revSBEnsureFolder tResBuildFolder
      if there is no folder tDrawableResBuildFolder then
         create folder tDrawableResBuildFolder
      end if
      if there is no folder tLayoutResBuildFolder then
         create folder tLayoutResBuildFolder
      end if
      if there is no folder tXMLResBuildFolder then
         create folder tXMLResBuildFolder
      end if
      if there is no folder tBuildFolder or there is no folder tClassesBuildFolder or \
            there is no folder tLibsBuildFolder or \
            there is no folder tResBuildFolder or there is no folder tDrawableResBuildFolder  or \
            there is no folder tLayoutResBuildFolder or there is no folder tXMLResBuildFolder then
         throw "could not create build folders"
      end if
      
      if the platform is "win32" then
         put shortFilePath(tBuildFolder) into tBuildFolder
      end if
      
      -------- Unpack all the files used by the externals
      local tManifestFile, tManifest
      
      revStandaloneProgress "Integrating externals..."
      
      if tExternals is not empty then
         put return after tExternals
      end if
      set the itemDelimiter to "."
      
      local tResolvedFileData
      revSBUpdateSettingsForExtensions "android", pSettings
      revSBResolveCopyFilesList tBaseFolder, "android", pSettings, tResolvedFileData
      
      addPlayServiceAARsToSettings pSettings
      expandAARsAndComputeSettings tBuildFolder, pSettings
      
      repeat for each element tFile in tResolvedFileData
         -- Add to list of manifests to merge
         if tFile["name"] is "AndroidManifest.xml" then
            appendToStringList tFile["resolved"], pSettings["androidManifests"]
            next repeat
         end if
         
         -- Only look for lcext files.
         switch the last item of tFile["name"]
            case "lcext"
               if there is no file tFile["resolved"] then
                  throw "could not find referenced external -" && tFile["resolved"]
               end if
               if addExternalFromFile(tFile["resolved"], tClassesBuildFolder, tArchLibsBuildFolders) then
                  -- Add the external to the list to load
                  put char 1 to -7 of tFile["name"] & return after tExternals
               end if
               break
            case "lcextd"
               if there is no folder tFile["resolved"] then
                  throw "could not find referenced external -" && tFile["resolved"]
               end if
               if addExternalFromFolder(tFile["resolved"], tClassesBuildFolder, tArchLibsBuildFolders) then
                  -- Add the external to the list to load
                  put char 1 to -8 of tFile["name"] & return after tExternals
               end if
               break
         end switch 
         
      end repeat
      delete the last char of tExternals
      set the itemDelimiter to comma
      
      -------- Create the Manifest
      revStandaloneProgress "Writing manifest..."
      
      if tManifestFile is empty then
         put tBuildFolder & slash & "AndroidManifest.xml" into tManifestFile
      end if
      
      if tManifest is empty then
         put url ("binfile:" & mapFilePath(revMobileRuntimeFolder(pTarget) & slash & "Manifest.xml")) into tManifest
      end if
      // SN-2015-01-22: [[ Bug 13213 ]] Throw an error in case an error occurred
      if the result is not empty then
         throw "Cannot find the template Manifest"
      end if
      local tEscapedLabel
      put escapeLabel(pSettings["android,label"]) into tEscapedLabel
      replace "${LABEL}" with textEncode(tEscapedLabel, "utf-8") in tManifest
      replace "${NAME}" with ".mblandroid" in tManifest
      replace "${IDENTIFIER}" with pSettings["android,identifier"] in tManifest
      replace "${VERSION_NAME}" with pSettings["android,version name"] in tManifest
      replace "${VERSION_CODE}" with pSettings["android,version code"] in tManifest
      if there is a file tIcon then
         replace "${ICON}" with "android:icon=" & quote & "@drawable/icon" & quote in tManifest
      else
         replace "${ICON}" with empty in tManifest
      end if
      
      if pSettings["android,status bar hidden"] then
         replace "${THEME}" with "@android:style/Theme.NoTitleBar.Fullscreen" in tManifest
      else
         replace "${THEME}" with "@android:style/Theme.NoTitleBar" in tManifest
      end if
      
      local tInitialOrientation
      put pSettings["android,initial orientation"] into tInitialOrientation
      if tInitialOrientation is "landscape" then
         put "sensorLandscape" into tInitialOrientation
      end if
      replace "${ORIENTATION}" with tInitialOrientation in tManifest
      
      replace "${MIN_SDK_VERSION}" with pSettings["android,minimum version"] in tManifest
      replace "${TARGET_SDK_VERSION}" with targetSDKVersion() in tManifest
      
      local tUsesFeature, tUsesPermission, tFeatures, tPermissions, tAd
      repeat for each key tFeature in pSettings["android,device capabilities"]
         put "  <uses-feature android:name="&quote&"android."&tFeature&quote&" android:required="&quote&"${REQUIRED}"&quote&" />" into tUsesFeature
         replace "${REQUIRED}" with pSettings["android,device capabilities"][tFeature] in tUsesFeature
         put tUsesFeature & return after tFeatures
      end repeat
      repeat for each key tPermission in pSettings["android,capability permissions"]
         put "  <uses-permission android:name="&quote&"android.permission."&tPermission&quote&" />" into tUsesPermission
         put tUsesPermission & return after tPermissions
      end repeat
      repeat for each key tPermission in pSettings["android,application permissions"]
         if tPermission is "AD" then
            put true into tAd
            put "  <uses-permission android:name="&quote&"android.permission.ACCESS_NETWORK_STATE"&quote&"/>" & return into tUsesPermission
            put "  <uses-permission android:name="&quote&"android.permission.READ_PHONE_STATE"&quote&" />" after tUsesPermission
         else
            put "  <uses-permission android:name="&quote&"android.permission."&tPermission&quote&" />" into tUsesPermission
         end if
         put tUsesPermission & return after tPermissions
      end repeat
      
      replace "${USES_FEATURE}" with tFeatures in tManifest
      --replace "${USES_PERMISSION}" with tPermissions in tManifest
      
      local tService, tReceiver
      -- MW-2017-06-13: add generic service
      put "<service android:enabled=" & quote & "true" & quote && "android:name=" & quote & ".mblandroidservice" & quote & "/>" & return after tService
      
      -- IM-2012-02-28: need barebones receiver for local notifications
      put "<receiver android:name="&quote&"AppReceiver"&quote&">" into tReceiver
      --      if tBilling then
      --         -- create service & receiver sections for billing support
      --         put "<service android:name="&quote&"AppService"&quote&" />" into tService
      --         put "<intent-filter>" & return after tReceiver
      --         repeat for each item tMsgName in "IN_APP_NOTIFY,RESPONSE_CODE,PURCHASE_STATE_CHANGED"
      --            put "<action android:name="&quote&"com.android.vending.billing."&tMsgName&quote&" />" & return after tReceiver
      --         end repeat
      --         put "</intent-filter>" after tReceiver
      --      else
      --         put empty into tService
      --      end if
      put "</receiver>" after tReceiver
      
      -- PM-2014-03-17: [[ In App Purchasing Updates ]] Include new store setting in manifest.
      if pSettings["android,inAppPurchasing"] then
         get pSettings["android,billingProvider"]
         if it is "Google" then
            put "  <uses-permission android:name="&quote&"com.android.vending.BILLING"&quote&" />" & return after tPermissions
         end if
         if it is "Samsung" then
            put "  <uses-permission android:name="&quote&"com.sec.android.iap.permission.BILLING"&quote&" />" & return after tPermissions
         end if
         if it is "Amazon" and revLicenseType() is "commercial" then
            local tAmazonReceiver
            put "<receiver android:name="&quote&"com.amazon.inapp.purchasing.ResponseReceiver"&quote&">" into tAmazonReceiver
            put "<intent-filter>" & return after tAmazonReceiver
            put "<action android:name="&quote&"com.amazon.inapp.purchasing.NOTIFY"&quote&" " & return after tAmazonReceiver
            put "android:permission ="&quote&"com.amazon.inapp.purchasing.Permission.NOTIFY"&quote&"  />" & return after tAmazonReceiver
            put "</intent-filter>" after tAmazonReceiver
            put "</receiver>" after tAmazonReceiver
            put tAmazonReceiver after tReceiver
         end if
      end if
      
      local tNFCIntentFilter
      if pSettings["android,nfc"] then
         put "<intent-filter>" & return & \
               "<action android:name=" & quote & "android.nfc.action.TECH_DISCOVERED" & quote & " />" & return & \
               "</intent-filter>" & return & \
               "<meta-data android:name=" & quote & "android.nfc.action.TECH_DISCOVERED" & quote & \
               " android:resource=" & quote & "@xml/nfc_tech_filter" & quote & " />" & return into tNFCIntentFilter
         
         put "<uses-permission android:name=" & quote & "android.permission.NFC" & quote & " />" & return after tPermissions
      end if
      replace "${NFC_FILTER}" with tNFCIntentFilter in tManifest
      
      replace "${SERVICE}" with tService in tManifest
      replace "${RECEIVER}" with tReceiver in tManifest
      replace "${USES_PERMISSION}" with tPermissions in tManifest
      
      if tAd is true then
         replace "${AD_ACTIVITY}" with "<activity android:name=" & quote & "com.inneractive.api.ads.InneractiveFullScreenView" & quote & "></activity>" in tManifest
      else
         replace "${AD_ACTIVITY}" with empty in tManifest
      end if
      
      local tInstallLocation
      switch pSettings["android,installLocation"]
         case "Internal Storage Only"
         case empty
            put empty into tInstallLocation
            break
         case "Allow External Storage"
            put "android:installLocation="&quote&"auto"&quote into tInstallLocation
            break
         case "Prefer External Storage"
            put "android:installLocation="&quote&"preferExternal"&quote into tInstallLocation
            break
      end switch
      replace "${INSTALL_LOCATION}" with tInstallLocation in tManifest
      
      local tCustomUrlFilter
      if pSettings["android,customUrlScheme"] is not empty then
         put "<intent-filter>" & \
               "<action android:name="&quote&"android.intent.action.VIEW"&quote&" />" & \
               "<category android:name="&quote&"android.intent.category.DEFAULT"&quote&" />" & \
               "<category android:name="&quote&"android.intent.category.BROWSABLE"&quote&" />" & \
               "<data android:scheme="&quote&pSettings["android,customUrlScheme"]&quote&" />" & \
               "</intent-filter>" into tCustomUrlFilter
      else
         put empty into tCustomUrlFilter
      end if
      replace "${CUSTOM_URL_FILTER}" with tCustomUrlFilter in tManifest
      
      local tPushPermissions
      local tPushReceiver
      if pSettings["android,pushSenderID"] is not empty then
         put "<permission android:name="&quote&pSettings["android,identifier"]&".permission.C2D_MESSAGE"&quote&" android:protectionLevel="&quote&"signature"&quote&" />" & \
               "<uses-permission android:name="&quote&pSettings["android,identifier"]&".permission.C2D_MESSAGE"&quote&" />" & \
               "<uses-permission android:name="&quote&"com.google.android.c2dm.permission.RECEIVE"&quote&" />" into tPushPermissions
         
         put "<receiver android:name="&quote&"PushReceiver"&quote&" android:permission="&quote&"com.google.android.c2dm.permission.SEND"&quote&">" & \
               "<intent-filter>" & \
               "<action android:name="&quote&"com.google.android.c2dm.intent.RECEIVE"&quote&" />" & \
               "<category android:name="&quote&pSettings["android,identifier"]&quote&" />" & \
               "</intent-filter>" & \
               "<intent-filter>" & \
               "<action android:name="&quote&"com.google.android.c2dm.intent.REGISTRATION"&quote&" />" & \
               "<category android:name="&quote&pSettings["android,identifier"]&quote&" />" & \
               "</intent-filter>" & \
               "</receiver>" into tPushReceiver
      else
         put empty into tPushPermissions
         put empty into tPushReceiver
      end if
      replace "${PUSH_PERMISSIONS}" with tPushPermissions in tManifest
      replace "${PUSH_RECEIVER}" with tPushReceiver in tManifest
      // Add the file provider to the manifest
      local tProvider
      put "<provider " & return into tProvider
      put "      android:name=" & quote & pSettings["android,identifier"] & ".AppProvider" & quote & return after tProvider
      put "      android:authorities=" & quote & pSettings["android,identifier"] & ".fileprovider" & quote & return after tProvider
      put "      android:exported=" & quote & "true" & quote & return after tProvider
      put "      android:grantUriPermissions=" & quote & "true" & quote & " >" & return after tProvider
      // Add meta-data tag
      put "      <meta-data" & return after tProvider
      put "            android:name=" & quote & "android.support.FILE_PROVIDER_PATHS" & quote & return after tProvider
      put "            android:resource=" & quote & "@xml/file_provider_paths" & quote & " />" & return after tProvider
      put "    </provider>" & return after tProvider
      
      replace "${PROVIDER}" with tProvider in tManifest
      
      // SN-2015-09-24: [[ Bug 15875 ]] Add the screenSize configChanges value for API > 12
      //  Otherwise the app restarts on orientation change.
      local tConfigChangesProp
      put "keyboardHidden|orientation|uiMode|screenSize" into tConfigChangesProp
      replace "${CONFIG_CHANGES}" with tConfigChangesProp in tManifest
      
      // SN-2015-09-30: [[ Bug 10267 ]] Hardware Acceleration option added
      local tHardwareAccelerated
      if pSettings["android,hardware accelerated"] then
         put "true" into tHardwareAccelerated
      else
         put "false" into tHardwareAccelerated
      end if
      replace "${HARDWARE_ACCELERATED}" with tHardwareAccelerated in tManifest
      
      put tManifest into url ("binfile:" & tManifestFile)
      
      local tAdditional
      repeat for each line tManifest in pSettings["androidManifests"]
         appendToList tManifest, tAdditional
      end repeat
      put androidCreateAppManifest(tManifestFile, tAdditional) \
            into url ("binfile:" & tManifestFile)
      if the result is not empty then
         throw "could not create manifest"
      end if
      
      -------- Build the Classes
      revStandaloneProgress "Building classes..."
      
      -- MW-2012-07-05: Make sure we record all classes we build so we can clean up.
      local tCompiledClassFilePath, tBuiltClassFiles
      put empty into tBuiltClassFiles
      put pSettings["android,identifier"] into tCompiledClassFilePath
      replace "." with "/" in tCompiledClassFilePath
      
      local tLiveCodeClassesFile
      put tBuildFolder & slash & "LiveCode.jar" into tLiveCodeClassesFile
      put url ("binfile:" & mapFilePath(revMobileRuntimeFolder(pTarget) & slash & "Classes")) into url ("binfile:" & tLiveCodeClassesFile)
      if the result is not empty then
         throw "could not prepare LiveCode classes"
      end if
      
      local tClassPath
      if the platform is "win32" then
         put pathToRootClasses() & ";" & tLiveCodeClassesFile & ";" & tClassesBuildFolder & ";" into tClassPath
      else
         put pathToRootClasses() & ":" & tLiveCodeClassesFile & ":" & tClassesBuildFolder into tClassPath
      end if
      
      -- MW-2017-06-13: compile the renamed service class
      local tClass, tClassFile
      put tBuildFolder & slash & "classes_app/mblandroidservice.java" into tClassFile
      put "package" && pSettings["android,identifier"] & ";" & return into tClass
      put "import com.runrev.android.LiveCodeService;" & return after tClass
      put "public class mblandroidservice extends LiveCodeService {}" & return after tClass
      put tClass into url ("binfile:" & tClassFile)
      if the result is not empty then
         throw "could not create billing support class"
      end if
      put tClassFile & return after tBuiltClassFiles
      executeShellCommand pathToJavaC(), "-target 1.5 -source 1.5 -Xlint:none -d", tClassesBuildFolder, "-cp", tClassPath, tClassFile
      if the result is not empty then
         throw "could not compile service support class"
      end if
      put tBuildFolder & slash & "classes_app/" & tCompiledClassFilePath & "/mblandroidservice.class" & return after tBuiltClassFiles
      
      put tBuildFolder & slash  & "classes_app/mblandroid.java" into tClassFile
      put "package" && pSettings["android,identifier"] & ";" & return into tClass
      put "import com.runrev.android.LiveCodeActivity; import java.lang.reflect.*;" & return after tClass
      put "public class mblandroid extends LiveCodeActivity {public Class getServiceClass() {return mblandroidservice.class;}}" & return after tClass
      put tClass into url ("binfile:" & tClassFile)
      if the result is not empty then
         throw "could not create activity class"
      end if
      put tClassFile & return after tBuiltClassFiles
      executeShellCommand pathToJavaC(), "-target 1.5 -source 1.5 -Xlint:none -d", tClassesBuildFolder, "-cp", tClassPath, tClassFile
      if the result is not empty then
         throw "could not compile application class"
      end if
      put tBuildFolder & slash & "classes_app/" & tCompiledClassFilePath & "/mblandroid.class" & return after tBuiltClassFiles
      
      -- IM-2012-02-28: need barebones receiver for local notifications
      // build receiver class
      put tBuildFolder & slash & "classes_app/AppReceiver.java" into tClassFile
      put "package" && pSettings["android,identifier"] & ";" & return into tClass
      put "import com.runrev.android.EngineReceiver;" & return after tClass
      put "public class AppReceiver extends EngineReceiver {}" & return after tClass
      put tClass into url ("binfile:" & tClassFile)
      if the result is not empty then
         throw "could not create billing support class"
      end if
      put tClassFile & return after tBuiltClassFiles
      
      executeShellCommand pathToJavaC(), "-target 1.5 -source 1.5 -Xlint:none -d", tClassesBuildFolder, "-cp", tClassPath, tClassFile
      if the result is not empty then
         throw "could not compile billing support class"
      end if
      put tBuildFolder & slash & "classes_app/" & tCompiledClassFilePath & "/AppReceiver.class" & return after tBuiltClassFiles
      
      -- IM-2011-10-26: Generate and compile billing service & receiver classes
      --      if tBilling then
      --         local tBillingClass, tBillingClassFile
      
      --         // build service class
      --         put tBuildFolder & slash & "classes_app/AppService.java" into tBillingClassFile
      --         put "package" && pSettings["android,identifier"] & ";" & return into tBillingClass
      --         put "import com.runrev.android.billing.BillingService;" & return after tBillingClass
      --         put "public class AppService extends BillingService {}" & return after tBillingClass
      --         put tBillingClass into url ("binfile:" & tBillingClassFile)
      --         if the result is not empty then
      --            throw "could not create billing support class"
      --         end if
      --         put tBillingClassFile & return after tBuiltClassFiles
      
      --         executeShellCommand pathToJavaC(), "-target 1.5 -source 1.5 -Xlint:none -d", tClassesBuildFolder, "-cp", tClassPath, tBillingClassFile
      --         if the result is not empty then
      --            throw "could not compile billing support class"
      --         end if
      --         put tBuildFolder & slash & "classes_app/" & tCompiledClassFilePath & "/AppService.class" & return after tBuiltClassFiles
      --      end if
      
      // IM-2012-02-24: Generate push notification listener class
      if pSettings["android,pushSenderID"] is not empty then
         local tPushReceiverClass, tPushReceiverClassFile
         put tBuildFolder & slash & "classes_app/PushReceiver.java" into tPushReceiverClassFile
         
         put "package" && pSettings["android,identifier"] & ";" & return into tPushReceiverClass
         put "import com.runrev.android.*;" & return after tPushReceiverClass
         put "public class PushReceiver extends com.runrev.android.PushReceiver {}" & return after tPushReceiverClass
         put tPushReceiverClass into url ("binfile:" & tPushReceiverClassFile)
         if the result is not empty then
            throw "could not create push notification support class"
         end if
         put tPushReceiverClassFile & return after tBuiltClassFiles
         
         executeShellCommand pathToJavac(), "-target 1.5 -source 1.5 -Xlint:none -d", tClassesBuildFolder, "-cp", tClassPath, tPushReceiverClassFile
         if the result is not empty then
            throw "could not compile push notification support class"
         end if
         put tBuildFolder & slash & "classes_app/" & tCompiledClassFilePath & "/PushReceiver.class" & return after tBuiltClassFiles
      end if
      
      // Generate app content provider class
      local tProviderClass, tProviderClassFile
      put tBuildFolder & slash & "classes_app/AppProvider.java" into tProviderClassFile
      
      put "package" && pSettings["android,identifier"] & ";" & return into tProviderClass
      put "import com.runrev.android.*;" & return after tProviderClass
      put "public class AppProvider extends com.runrev.android.AppProvider {}" & return after tProviderClass
      
      put tProviderClass into url ("binfile:" & tProviderClassFile)
      if the result is not empty then
         throw "could not create app content provider"
      end if
      put tProviderClassFile & return after tBuiltClassFiles
      
      executeShellCommand pathToJavac(), "-target 1.5 -source 1.5 -Xlint:none -d", tClassesBuildFolder, "-cp", tClassPath, tProviderClassFile
      if the result is not empty then
         throw "could not compile app content provider" & return & the result
      end if
      put tBuildFolder & slash & "classes_app/" & tCompiledClassFilePath & "/AppProvider.class" & return after tBuiltClassFiles
      
      set the itemdel to slash
      local tJarCount
      repeat for each line tJarFile in pSettings["jarFiles"]
         -- use numeric jar file names as they might have the same
         -- source name
         add 1 to tJarCount
         local tTargetFile
         put tLibsBuildFolder & slash & tJarCount & ".jar" into tTargetFile
         put url ("binfile:" & tJarFile) into url ("binfile:" & tTargetFile)
      end repeat
      
      -- MW-2012-07-05: Record all asset files that are copied into /tmp
      local tAssetFiles
      put empty into tAssetFiles
      
      local tAssetArchive
      put tBuildFolder & slash & "lcandroid.ap_" into tAssetArchive
      if there is a file tAssetArchive then
         delete file tAssetArchive
      end if
      
      local tLayoutXML
      put mapFilePath(revMobileRuntimeFolder(pTarget) & slash & "livecode_inputcontrol.xml") into tLayoutXML
      put url ("binfile:" & tLayoutXML) into url ("binfile:" & tLayoutResBuildFolder & slash & "livecode_inputcontrol.xml")
      if the result is not empty then
         throw "could not copy layout file '" & tLayoutXML & "'"
      end if
      put tLayoutResBuildFolder & slash & "livecode_inputcontrol.xml" & return after tAssetFiles
      
      local tNFCTechFilter
      put mapFilePath(revMobileRuntimeFolder(pTarget) & slash & "nfc_tech_filter.xml") into tNFCTechFilter
      put url ("binfile:" & tNFCTechFilter) into url ("binfile:" & tXMLResBuildFolder & slash & "nfc_tech_filter.xml")
      if the result is not empty then
         throw "could not copy NFC tech filter file '" & tNFCTechFilter & "'"
      end if
      put tXMLResBuildFolder & slash & "nfc_tech_filter.xml" & return after tAssetFiles
      
      
      local tFileProviderPaths
      put mapFilePath(revMobileRuntimeFolder(pTarget) & slash & "file_provider_paths.xml") into tFileProviderPaths
      put url ("binfile:" & tFileProviderPaths) into url ("binfile:" & tXMLResBuildFolder & slash & "file_provider_paths.xml")
      if the result is not empty then
         throw "could not copy file provider paths file '" & tFileProviderPaths & "'"
      end if
      put tXMLResBuildFolder & slash & "file_provider_paths.xml" & return after tAssetFiles
      
      if there is a file tIcon then
         put url ("binfile:" & tIcon) into url ("binfile:" & tDrawableResBuildFolder & slash & "icon.png")
         if the result is not empty then
            throw "could not copy icon '" & tIcon & "'"
         end if
         put tDrawableResBuildFolder & slash & "icon.png" & return after tAssetFiles
      end if
      
      if there is a file tStatusBarIcon then
         put url ("binfile:" & tStatusBarIcon) into url ("binfile:" & tDrawableResBuildFolder & slash & "notify_icon.png")
         if the result is not empty then
            throw "could not copy icon '" & tStatusBarIcon & "'"
         end if
         put tDrawableResBuildFolder & slash & "notify_icon.png" & return after tAssetFiles
      end if
      
      local tResourceClasses
      generateResourceClasses tBuildFolder, tResBuildFolder, \
            tManifestFile, pSettings, tResourceClasses
      if the result is not empty then
         throw "could not build R.java" & return & the result
      end if
      if tResourceClasses is not empty then
         local tParams
         put "-target 1.5 -source 1.5 -Xlint:none -d" into tParams
         split tParams by space
         appendToList tClassesBuildFolder, tParams
         appendToList "-cp", tParams
         appendToList tClassPath, tParams
         repeat for each line tRClass in tResourceClasses
            appendToList tRClass, tParams
         end repeat
         executeShellCommandWithParams pathToJavac(), tParams
         if the result is not empty then
            throw "could not compile resource class" & return & the result
         end if
         -- Add to list of files to delete
      end if
      
      local tClassesFile
      put tBuildFolder & slash & "classes.dex" into tClassesFile
      executeShellCommand pathToDex(), "--dex", "--output=" & quote & tClassesFile & quote, tClassesBuildFolder, tLiveCodeClassesFile, tLibsBuildFolder
      if the result is not empty then
         throw "could not encode class bundle"
      end if
      
      ------- Compile the Standalone
      revStandaloneProgress "Building executable..."
      
      put tExternals into pSettings["externals"]
      
      if pSettings["android,library"] is empty then
         put "revsecurity:./librevsecurity" into pSettings["android,library"]
      else
         put return & "revsecurity:./librevsecurity" after pSettings["android,library"]
      end if
      repeat for each line tEntry in tExternals
         put return & tEntry & ":" & "./lib" & tEntry after pSettings["android,library"]
      end repeat
      repeat for each line tEntry in tDrivers
         put return & tEntry & ":" & "./lib" & tEntry after pSettings["android,library"]
      end repeat
      
      set the itemDelimiter to comma
      repeat for each item tArch in tArchs
         local tArchCodeResources
         filter pSettings["extension_code_resources"] \
               with ("*code/" & tArch & "-android*") into tArchCodeResources
         
         repeat for each line tLib in tArchCodeResources
            local tSourceFile, tSourceName
            set the itemdelimiter to slash
            put item -1 of tLib into tSourceFile
            set the itemDelimiter to "."
            put item 1 to -2 of tSourceFile into tSourceName
            set the itemDelimiter to comma
            
            if not(tSourceFile begins with "lib") then
               put "lib" before tSourceFile
            end if
            
            put url ("binfile:" & tLib) into \
                  url ("binfile:" & tArchLibsBuildFolders[__ArchABI(tArch)] & slash & tSourceFile)
            
            if (tSourceName & ":./" & tSourceFile) is not among the lines of pSettings["android,library"] then
               put return & tSourceName & ":./" & tSourceFile after pSettings["android,library"]
            end if
         end repeat
      end repeat
      
      local tDeploy
      -- Update the deploy params with script library and extension inclusions
      revSBUpdateDeployParams pStack, "android", pSettings, tDeploy
      
      -- Stackfile to use
      put the effective filename of stack pStack into tDeploy["stackfile"]
      
      -- Splash to use (if non-commercial)
      if line 3 of the revLicenseInfo is among the words of "Educational Personal" then
         -- MW-2011-03-17: Make sure absolute paths work correctly
         if pSettings["android,splash"] is not empty and pathIsRelative(pSettings["android,splash"]) then
            put tBaseFolder & slash & pSettings["android,splash"] into tDeploy["splash"]
         else
            put pSettings["android,splash"] into tDeploy["splash"]
         end if
      end if

      repeat for each key tArch in tArchLibsBuildFolders
         -- Engine to use
         put mapFilePath(revMobileRuntimeFolder(pTarget) & slash & tArch & slash & "Standalone") into tDeploy["engine"]
         // SN-2015-01-22: [[ Bug 13213 ]] Throw an error in case an error occurred
         if there is no file tDeploy["engine"] then
            throw "Could not find standalone engine"
         end if
         
         -- Output file to create
         put tArchLibsBuildFolders[tArch] & slash & "librevandroid.so" into tDeploy["output"]
         
         _internal deploy android tDeploy
         if the result is not empty then
            throw the result
         end if
      end repeat
      
      -------- Assembling Assets
      revStandaloneProgress "Assembling assets..."
      
      -- Make sure we include the res folders for each aar when packaging
      local tAarResFolderOpts
      put empty into tAarResFolderOpts
      repeat with x = 1 to the number of lines in pSettings["aarFiles"]
         local tAarResFolder
         put tBuildFolder & slash & x & "/res" into tAarResFolder
         if there is a folder tAarResFolder then
            put "-S" && quote & tAarResFolder & quote & space after tAarResFolderOpts
         end if
      end repeat      
      
      local tAaptResult
      if there is a folder tResBuildFolder then
         executeShellCommand pathToAapt(), "package", "-f", "-M", tManifestFile, \
               "-I", pathToRootClasses(),"-F", tAssetArchive, "-S", tResBuildFolder, \
               tAarResFolderOpts, "--auto-add-overlay"
      else
         executeShellCommand pathToAapt(), "package", "-f", "-M", tManifestFile, \
               "-I", pathToRootClasses(),"-F", tAssetArchive, \
               tAarResFolderOpts, "--auto-add-overlay"
      end if
      put the result into tAaptResult
      processAaptResult tAaptResult
      if tAaptResult is not empty then
         throw "could not generate package manifest"
      end if
      
      revZipOpenArchive tAssetArchive, "update"
      if the result is not empty then
         throw "could not open asset archive"
      end if
      
      addAssetsToArchive tResolvedFileData, tBaseFolder, tAssetArchive
      
      revZipCloseArchive tAssetArchive
      if the result is not empty then
         throw "asset archive creation failed"
      end if
      
      ------- Prepare the Package
      revStandaloneProgress "Preparing package..."
      
      local tUnalignedApkFile
      put tBuildFolder & slash & "lcandroid-unaligned.apk" into tUnalignedApkFile
      if there is a file tUnalignedApkFile then
         delete file tUnalignedApkFile
      end if
      
      repeat for each key tArch in tArchLibsBuildFolders
         -- MM-2014-01-29: [[ OpenSSL ]] Include the revsecurity library.
         if pSettings["android,include revsecurity"] then
            put url ("binfile:" & mapFilePath(revMobileRuntimeFolder(pTarget) & slash & tArch & slash & "revsecurity")) \
                  into url ("binfile:" & tArchLibsBuildFolders[tArch] & slash & "librevsecurity.so")
            
            // SN-2015-01-22: [[ Bug 13213 ]] Throw an error in case an error occurred
            if the result is not empty then
               throw "Could not find revsecurity library"
            end if
         end if
         
         -- Copy across the externals into the libs folder
         repeat for each line tEntry in tExternals
            -- Only copy across an external from the Runtime folder if we haven't
            -- already copied from a lcext.
            if there is a file (tArchLibsBuildFolders[tArch] & slash & "lib" & tEntry & ".so") then
               next repeat
            end if
            put url ("binfile:" & mapFilePath(revMobileRuntimeFolder(pTarget) & slash & tArch & slash & tEntry)) \
                  into url ("binfile:" & tArchLibsBuildFolders[tArch] & slash & "lib" & tEntry & ".so")
            // SN-2015-01-22: [[ Bug 13213 ]] Throw an error in case an error occurred
            if the result is not empty then
               throw "Could not find " & tEntry && "library"
            end if
         end repeat
         
         -- Copy across any other driver libraries into the libs folder
         repeat for each line tEntry in tDrivers
            -- Only copy across the library if it's not already there
            if there is a file (tArchLibsBuildFolders[tArch] & slash & "lib" & tEntry & ".so") then
               next repeat
            end if
            put url ("binfile:" & mapFilePath(revMobileRuntimeFolder(pTarget) & slash & tArch & slash & tEntry)) \
                  into url ("binfile:" & tArchLibsBuildFolders[tArch] & slash & "lib" & tEntry & ".so")
            if the result is not empty then
               throw "Could not find " & tEntry && "library"
            end if
         end repeat
      end repeat

      -- Work out whether to produce an unsigned apk
      local tUnsignedParam
      if (pTarget contains "build") and tUnsigned then
         put "-u" into tUnsignedParam
      else
         put empty into tUnsignedParam
      end if
      
      -- Build the APK
      // IM-2013-05-21: [[ BZ 10904 ]] call our own internal function instead of (removed) apkbuilder script
      buildAPK tUnalignedApkFile, tUnsignedParam, tAssetArchive, tClassesFile, tLibsBuildFolder
      if the result contains "Debug Certificate Expired" then
         throw "debug certificate has expired, please refresh"
      end if
      repeat for each key tArch in tArchLibsBuildFolders
         if not (the result contains "=> lib/" & tArch & "/librevandroid.so") then
            throw "apk preparation failed"
         end if
      end repeat

      ------- Signing the Package
      if pTarget contains "build" and tKey is not empty then
         revStandaloneProgress "Signing package..."
         
         if not tKey begins with "/" then
            put tBaseFolder & slash before tKey
         end if
         
         if there is no file tKey then
            throw "could not file keystore '" & tKey & "'"
         end if
         
         local tKeystoreAlias
         ask "Please provide the keystore alias to use"
         if it is empty then
            throw "no keystore alias provided for keystore"
         end if
         put it into tKeystoreAlias
         
         local tKeystorePassword
         ask password clear "Please provide the keystore password"
         if it is empty then
            throw "no password provided for keystore"
         end if
         put it into tKeystorePassword
         
         local tPrivateKeyPassword
         ask password clear "Please provide the private key password"
         if it is empty then
            throw "no password provided for private key"
         end if
         put it into tPrivateKeyPassword
         
         // IM-2014-09-04: [[ Bug 13343 ]] Specify SHA1 when signing APK as specified in android signing docs.
         executeShellCommand pathToJarSigner(), "-sigalg", "SHA1withRSA", "-digestalg", "SHA1", "-keystore", tKey, "-storepass", tKeystorePassword, "-keypass", tPrivateKeyPassword, tUnalignedApkFile, tKeystoreAlias
         if the result begins with "jarsigner error" then
            throw "signing failed -" && word 4 to -1 of the result
         else if the result begins with "jarsigner: Certificate chain not found" then
            throw "signing failed -" && word 2 to -1 of the result
         else if (the result begins with "jarsigner: key associated with") and (line 1 of the result ends with "not a private key") then
            throw "signing failed - key password was incorrect or" && word 2 to -1 of the result
         end if
      end if
      
      ------- Finalize the Package
      revStandaloneProgress "Finalizing package..."
      
      executeShellCommand pathToZipAlign(), "-f", "-v", "4", tUnalignedApkFile, pApkFile
      if not (line -1 of the result begins with "Verification succes")  or there is no file pApkFile then
         throw "apk finalization failed"
      end if
   catch tError
   end try
   
   if there is a file tUnalignedApkFile then delete file tUnalignedApkFile
   if there is a file tAssetArchive then delete file tAssetArchive
   if there is a file tLiveCodeClassesFile then delete file tLiveCodeClassesFile
   
   repeat for each element tFolder in tArchLibsBuildFolders
      repeat for each line tFile in files(tFolder)
         delete file (tFolder & slash & tFile)
      end repeat
      if there is a folder tFolder then delete folder tFolder
   end repeat
   
   set the itemDelimiter to slash
   repeat for each line tJarFile in pSettings["jarFiles"]
      if there is a file (tLibsBuildFolder & slash & item -1 of tJarFile) then 
         delete file (tLibsBuildFolder & slash & item -1 of tJarFile)
      end if
   end repeat
   if there is a folder tLibsBuildFolder then delete folder tLibsBuildFolder
   if there is a file tClassesFile then delete file tClassesFile
   repeat for each line tClassFile in tBuiltClassFiles
      if there is a file tClassFile then delete file tClassFile
   end repeat
   repeat while there is a folder (tBuildFolder & slash & "classes_app" & slash & tCompiledClassFilePath)
      delete folder (tBuildFolder & slash & "classes_app" & slash & tCompiledClassFilePath)
      if the result is not empty then
         exit repeat
      end if
      delete the last item of tCompiledClassFilePath
   end repeat
   set the itemDelimiter to comma
   repeat for each line tFIle in tAssetFiles
      if there is a file tFile then delete file tFile
   end repeat
   if there is a folder tLayoutResBuildFolder then delete folder tLayoutResBuildFolder
   if there is a folder tDrawableResBuildFolder then delete folder tDrawableResBuildFolder
   if there is a folder tResBuildFolder then delete folder tResBuildFolder
   if there is a file tManifestFile then delete file tManifestFile
   if there is a folder tClassesBuildFolder then delete folder tClassesBuildFolder
   if there is a folder tBuildFolder then delete folder tBuildFolder
   
   if tError is not empty then
      throw tError
   end if
end revSaveAsMobileStandaloneMain

################################################################################

private function escapeLabel pLabel
   replace "&" with "&amp;" in pLabel
   replace "@" with "\@" in pLabel
   replace "?" with "\?" in pLabel
   replace "<" with "&lt;" in pLabel
   replace ">" with "&gt;" in pLabel
   replace "'" with "\'" in pLabel
   replace quote with "\" & quote in pLabel
   return pLabel
end escapeLabel
   
private command processAaptResult @xResult   
   -- MW-2012-07-04: On Linux, 'aapt' can complain about libz version info
   --   so we delete that line from the result before checking for an error.
   if line 1 of xResult contains "no version information available" then
      delete line 1 of xResult
   end if
   // SN-2014-10-27: [[ Bug 13800 ]] libpng 1.6+ raises a warning for the PNG files having embedded sRGB profile
   if line 1 of xResult contains "iCCP: Not recognizing known sRGB profile that has been edited" then
      delete line 1 of xResult
   end if
end processAaptResult

private command generateResourceClasses pBuildFolder, pResBuildFolder, \
   pManifest, @xSettings, @rGenerated
   if xSettings["aarFiles"] is empty then
      return empty
   end if
   
   local tOutputFolder, tAaptResult, tManifest
   put pBuildFolder & slash & "gen" into tOutputFolder
   revSBEnsureFolder tOutputFolder
   -- at the moment we just generate resource classes for each included aar
   repeat with x = 1 to the number of lines in xSettings["aarFiles"]
      put pBuildFolder & "/" & x & "/AndroidManifest.xml" into tManifest
      executeShellCommand pathToAapt(), "package", "-f", "-M", tManifest, \
            "-I", pathToRootClasses(), "-S", pBuildFolder & slash & x & "/res", "-J", \
            tOutputFolder & "/", "-m"
      put the result into tAaptResult
      processAaptResult tAaptResult
      if tAaptResult is not empty then
         return tAaptResult
      end if
   end repeat
   local tGenerated
   put revSBEnumerateFolder(tOutputFolder) into tGenerated
   filter tGenerated with "*/R.java"
   if tGenerated is not empty then
      repeat for each line tLine in tGenerated
         if rGenerated is empty then
            put tOutputFolder & slash &tLine into rGenerated
         else
            put return & tOutputFolder & slash & tLine after rGenerated
         end if
      end repeat
   end if
   return empty
end generateResourceClasses

private command appendToStringList pValue, @xList
   if xList is empty then
      put pValue into xList
   else
      put return & pValue after xList
   end if
end appendToStringList

private command addPlayServiceAARsToSettings  @xSettingsA
   local tPlayServicesPath
   put pathToPlayServices() into tPlayServicesPath
   
   repeat for each line tPlayService in xSettingsA["android,play dependencies"]
      local tPlayVersion
      set the itemDel to "-"
      put the last item of tPlayService into tPlayVersion
      delete the last item of tPlayService
      set the itemDel to comma
      
      local tPlayServiceAARPath
      put tPlayServicesPath & slash & \
            "play-services-" & tPlayService & slash & \
            tPlayVersion & slash & \
            "play-services-" & tPlayService & "-" & tPlayVersion & ".aar" \
            into tPlayServiceAARPath
      if there is not a file tPlayServiceAARPath then
         throw "unable to locate play service dependency" && tPlayService && "version" && tPlayVersion
      end if
      
      appendToStringList tPlayServiceAARPath, xSettingsA["aarFiles"]
   end repeat
end addPlayServiceAARsToSettings

private command expandAARsAndComputeSettings tBuildFolder, @xSettingsA
   local tCount, tDefaultFolder
   put the folder into tDefaultFolder
   repeat for each line tAAR in xSettingsA["aarFiles"]
      local tExtractFolder, tExtractCmd, tExtractedFiles
      add 1 to tCount
      put tBuildFolder & slash & tCount \
            into tExtractFolder
      revSBEnsureFolder tExtractFolder
      set the folder to tExtractFolder
      executeShellCommand pathToJar(), "xf", tAAR
      updateSettingsForExtractedAAR tExtractFolder, xSettingsA
   end repeat
   set the folder to tDefaultFolder
end expandAARsAndComputeSettings
   
constant kSupportedAARFiles = "AndroidManifest.xml,classes.jar,R.txt"   
constant kSupportedAARFolders = "res"
private command updateSettingsForExtractedAAR pFolder, @xSettingsA
   local tExtractedItem
   repeat for each item tItem in kSupportedAARFiles
      put pFolder & slash & tItem into tExtractedItem
      if there is not a file tExtractedItem then
         next repeat
      end if
      switch tItem
         case "AndroidManifest.xml"
            appendToStringList tExtractedItem, xSettingsA["androidManifests"]
            break
         case "classes.jar"
            appendToStringList tExtractedItem, xSettingsA["jarFiles"]
            break
         case "R.txt"
            -- Don't need to do anything with this
            break
      end switch
   end repeat
   repeat for each item tItem in kSupportedAARFolders
      put pFolder & slash & tItem into tExtractedItem
      if there is not a folder tExtractedItem then
         next repeat
      end if     
      switch tItem
         case "res"
            -- don't need to do anything with this- our call to aapt
            -- deals with including the res files in the APK
            break
      end switch
   end repeat
end updateSettingsForExtractedAAR

private command addAssetsToArchive pFileData, pBaseFolder, pArchive
   local tDeviceConfig, tConfigFile, tCustomConfigFile, tFonts
   
   put mapFIlePath(revMobileRuntimeFolder() & slash & "lc_device_config.txt") into tConfigFile
   if there is not a file tConfigFile then
      put empty into tConfigFile
   end if
   
   repeat for each element tFile in pFileData
      if there is a file tFile["resolved"] then
         if tConfigFile is not empty and tFile["name"] ends with "lc_device_config.txt" then
            // need to merge config files
            put tFile["resolved"] into tCustomConfigFile
         else if revSBFileIsFontFile(tFile["name"]) then
            -- MM-2012-03-07: [[ Custom fonts ]] If any fonts are found, add to the list so that they are handled correctly
            put tFile["resolved"] & return after tFonts
         else if tFile["name"] ends with ".lcext" or tFile["name"] ends with ".dylib" then
            -- MW-2012-07-05: [[ Externals ]] If an 'lcext' file or dylib file is found then we ignore it.
         else if tFile["name"] ends with "AndroidManifest.xml" then
            -- MW-2015-04-24: [[ TemplateAppMetadata ]] If an template manifest is found then we ignore it.
         else
            revZipAddUncompressedItemWithFile pArchive, "assets/" & tFile["name"], tFile["resolved"]
         end if
      else if there is a folder tFile["resolved"] then
         addAssetsFromFolderToArchive tFile["resolved"], "assets" & slash & tFile["name"], pArchive, tFonts
      else
         throw "could not find referenced asset to include -" && tFile["resolved"]
      end if
   end repeat
   
   if tCustomConfigFile is empty then
      if tConfigFile is not empty then
         revZipAddUncompressedItemWithFile pArchive, "assets/lc_device_config.txt", tConfigFile
      end if
   else
      put url ("binfile:" & tConfigFile) & return into tDeviceConfig
      put url ("binfile:" & tCustomConfigFile) after tDeviceConfig
      revZipAddUncompressedItemWithData pArchive, "assets/lc_device_config.txt", "tDeviceConfig"
   end if
   
   -- MM-2012-03-07: [[ Custom fonts ]] Add any fonts found to the assets/fonts folder
   set the itemDel to slash
   repeat for each line tFont in tFonts
      if tFont is not empty then
         revZipAddUncompressedItemWithFile pArchive, "assets/fonts/" & item -1 of tFont, tFont
      end if
   end repeat
   set the itemDel to comma   
end addAssetsToArchive

private command addAssetsFromFolderToArchive pFolder, pBaseName, pArchive, @pFonts
   set the itemDelimiter to slash
   
   -- Ignore any .svn folders
   if the last item of pFolder is ".svn" then
      exit addAssetsFromFolderToArchive
   end if

   if char -1 of pFolder is slash then
      delete char -1 of pFolder
   end if
   
   local tOldFolder
   put the folder into tOldFolder
   set the folder to pFolder
   if the folder is not pFolder then
      throw "could not descend into referenced asset folder -" && pFolder
   end if
   
   repeat for each line tFile in the files
      if revSBFileIsFontFile(tFile) then
         -- MM-2012-03-07: [[ Custom fonts ]] If any fonts are found, add to the list so that they are handled correctly
         put the folder & slash & tFile & return after pFonts
      else if tFile ends with ".lcext" or tFile ends with ".dylib" then
         -- MW-2012-07-05: [[ Externals ]] If an 'lcext' file or dylib file is found then we ignore it.
      else
         revZipAddUncompressedItemWithFile pArchive, pBaseName & slash & tFile, the folder & slash & tFile
      end if
   end repeat
   
   repeat for each line tFolder in the folders
      if tFolder is ".." then
         next repeat
      end if
      
      addAssetsFromFolderToArchive pFolder & slash & tFolder, pBaseName & slash & tFolder, pArchive, pFonts
   end repeat
   
   set the folder to tOldFolder
end addAssetsFromFolderToArchive

################################################################################

private function fileIsZipArchive pFile
   open file pFile for binary read
   if the result is empty then
      read from file pFile for 4 bytes
      close file pFile
   end if
   if it is empty then
      throw "could not open external -" && pFile
   end if
   
   return byte 1 of it is "P" and byte 2 of it is "K" and byte 3 of it is numToChar(3) and byte 4 of it is numToChar(4)
end fileIsZipArchive

-- MERG-2013-09-05: [[ Bug 11152 ]]  Change to function - returns true if external was successfully
--   extracted.
private function addExternalFromFile pExternal, pClassesFolder, pArchLibsFolders
   -- If the file is not a zip archive then there is nothing to do.
   if not fileIsZipArchive(pExternal) then
      exit addExternalFromFile
   end if
   
   -- Get the name of the external.
   local tExternalName
   set the itemDelimiter to slash
   get the last item of pExternal
   set the itemDelimiter to "."
   put the first item of it into tExternalName
   set the itemDelimiter to slash
   
   -- Compute the class / lib names.
   local tClassesFile, tLibFileA
   put pClassesFolder & slash & "lib" & tExternalName & ".jar" into tClassesFile
   
   -- Otherwise open the archive and check for appropriate android components
   revZipOpenArchive pExternal, "read"
   if the result is empty then
      
      -- MERG-2015-10-14: [[ ThirdpartyLibs ]] Add support for thirdparty librares
      local tZipItems
      put revZipEnumerateItems(pExternal) into tZipItems
      filter tZipItems with "Android/*"
      
      local tZipItemsFiltered
      filter tZipItems with regex pattern ".*(\.jar|Classes)$" into tZipItemsFiltered
      repeat for each line tZipItem in tZipItemsFiltered
         if tZipItem ends with ".jar" then
            revZipExtractItemToFile pExternal, tZipItem, \
                  pClassesFolder & slash & the last item of tZipItem 
         else 
            # will be Classes file
            revZipExtractItemToFile pExternal, tZipItem, tClassesFile
         end if
      end repeat
      
      repeat for each key tArch in pArchLibsFolders
         local tPattern
         put "^Android/(?:External-" & tArch & "|lib.+" & tArch & "\.so)$" into tPattern
         filter tZipItems with regex pattern tPattern into tZipItemsFiltered
         put pArchLibsFolders[tArch] & slash & "lib" & tExternalName & ".so" into tLibFileA[tArch]
         
         repeat for each line tZipItem in tZipItemsFiltered
            if tZipItem ends with ".so" then
               revZipExtractItemToFile pExternal, tZipItem, \
                     pArchLibsFolders[tArch] & slash & the last item of tZipItem 
            else 
               # will be External-<Arch> file
               revZipExtractItemToFile pExternal, tZipItem, tLibFileA[tArch]
            end if
         end repeat
      end repeat
      
      revZipCloseArchive pExternal
   end if
   
   if there is a file tClassesFile then
      repeat for each key tArch in pArchLibsFolders
         if there is no file tLibFileA[tArch] then
            throw "android external contains classes but no library for " & tArch
         end if
      end repeat
   end if
   
   return tLibFileA is an array
end addExternalFromFile

private function addExternalFromFolder pExternal, pClassesFolder, pArchLibsFolders
   -- Get the name of the external.
   local tExternalName
   set the itemDelimiter to slash
   get the last item of pExternal
   set the itemDelimiter to "."
   put the first item of it into tExternalName
   set the itemDelimiter to slash
   
   -- Compute the class / lib names.
   local tClassesFile, tLibFileA
   put pClassesFolder & slash & "lib" & tExternalName & ".jar" into tClassesFile
   
   local tItems
   put files(pExternal & "/Android") into tItems
   
   local tItemsFiltered
   filter tItems with regex pattern ".*(\.jar|Classes)$" into tItemsFiltered
   repeat for each line tItem in tItemsFiltered
      if tItem ends with ".jar" then
         revSBCopyFile pExternal & "/Android/" & tItem, \
               pClassesFolder & slash & the last item of tItem 
      else 
         # will be Classes file
         revSBCopyFile pExternal & "/Android/" & tItem, \
               tClassesFile
      end if
   end repeat
   
   repeat for each key tArch in pArchLibsFolders
      local tPattern
      put "^(?:External-" & tArch & "|lib.+" & tArch & "\.so)$" into tPattern
      filter tItems with regex pattern tPattern into tItemsFiltered
      put pArchLibsFolders[tArch] & slash & "lib" & tExternalName & ".so" into tLibFileA[tArch]
      repeat for each line tItem in tItemsFiltered
         if tItem ends with ".so" then
            revSBCopyFile pExternal & "/Android/" & tItem, \
                  pClassesFolder & slash & the last item of tItem 
         else 
            # will be External-<Arch> file
            revSBCopyFile pExternal & "/Android/" & tItem, \
                  tLibFileA[tArch]
         end if
      end repeat
   end repeat
   
   if there is a file tClassesFile then
      repeat for each key tArch in pArchLibsFolders
         if there is no file tLibFileA[tArch] then
            throw "android external contains classes but no library for " & tArch
         end if
      end repeat
   end if
   
   return tLibFileA is an array
end addExternalFromFolder

################################################################################

function revMobileRuntimeFolder pTarget
   local tOverridePath, tUserPath
   
   // IM-2013-12-16: [[ Bug 11594 ]] Check override is not empty to avoid accidentally detecting "/Android" path
   if revOverrideRuntimePath() is not empty then
      put revOverrideRuntimePath() & slash & "Android" into tOverridePath
      if there is a folder tOverridePath then
         return tOverridePath
      end if
   end if
   
   // IM-2013-12-16: [[ Bug 11594 ]] Check override is not empty to avoid accidentally detecting "/Android" path
   if revEnvironmentUserRuntimePath() is not empty then
      put revEnvironmentUserRuntimePath() & slash & "Android" into tUserPath
      if there is a folder tUserPath then
         return tUserPath
      end if
   end if
   
   return revEnvironmentRuntimePath() & slash & "Android"
end revMobileRuntimeFolder

################################################################################

private command executeAdbCommand pCommand
   local tAdb
   put pathToAdb() into tAdb
   if tAdb is empty or there is not a file tAdb then
      exit executeAdbCommand
   end if
   
   get doShellCommand(tAdb, pCommand)
   
   return it
end executeAdbCommand

private command appendToList pValue, @xList
   get the number of elements in xList
   put pValue into xList[it + 1]
end appendToList

private command prependToList pValue, @xList
   insertIntoList pValue, xList, 1
end prependToList

private command insertIntoList pValue, @xList, pAt
   local tNewList
   repeat with x = the number of elements in xList down to 1
      if x >= pAt then
         put xList[x] into tNewList[x+1]
      else
         put xList[x] into tNewList[x]
      end if
   end repeat
   put pValue into tNewList[pAt]
   put tNewList into xList
end insertIntoList
   
private command executeShellCommand
   local tArray
   repeat with i = 2 to the paramCount
      appendToList param(i), tArray
   end repeat
   
   executeShellCommandWithParams param(1), tArray
   return the result
end executeShellCommand

private command executeShellCommandWithParams pCommand, pParamArray
   local tShell
   repeat for each element tParam in pParamArray
      if tParam contains space and not (tParam begins with "-") then
         put quote & tParam & quote into tParam
      end if   
      put tParam & space after tShell
   end repeat
   
   get doShellCommand(pCommand, tShell)
   
   return it
end executeShellCommandWithParams

private function doShellCommand pCommand, pArguments
   --makeBuildLogEntry "Executing command:" && pCommand && pArguments
   
   local tCommandPath
   set the itemDelimiter to slash
   put item 1 to -2 of pCommand into tCommandPath
   if the platform is "win32" then
      put shortFilePath(tCommandPath) into tCommandPath
   end if
   
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
   write item -1 of pCommand && pArguments & return to stderr
   get shell(item -1 of pCommand && pArguments)
   
   put tOldPath into $PATH
   
   return it
end doShellCommand

################################################################################

constant kDeployLibrary = "revDeployLibraryAndroid"
private function targetSDKVersion
   dispatch function "deployTargetSdkVersion" to stack kDeployLibrary
   return the result
end targetSDKVersion

private function pathToRootClasses pRoot
   dispatch function "pathToRootClasses" to stack kDeployLibrary with pRoot
   return the result
end pathToRootClasses

private function pathToSDKClasses pRoot
   dispatch function "pathToSDKClasses" to stack kDeployLibrary with pRoot
   return the result
end pathToSDKClasses

private function pathToCommonClasses pRoot
   dispatch function "pathToCommonClasses" to stack kDeployLibrary with pRoot
   return the result
end pathToCommonClasses

private function pathToAapt pRoot
   dispatch function "pathToAapt" to stack kDeployLibrary with pRoot
   return the result
end pathToAapt

private function pathToAdb pRoot
   dispatch function "pathToAdb" to stack kDeployLibrary with pRoot
   return the result
end pathToAdb

private function pathToZipAlign pRoot
   dispatch function "pathToZipAlign" to stack kDeployLibrary with pRoot
   return the result
end pathToZipAlign

// SN-2015-07-02: [[ GypifiedAndroid ]] Get the right folder for android binaries
private function pathToRepoAndroidBinaries pArch
   local tBuild
   if revEnvironmentBinariesPath() contains "/Debug" then
      put "Debug" into tBuild
   else
      put "Release" into tBuild
   end if
   return revEnvironmentRepositoryPath() & "/build-android-" & pArch & "/livecode/out/" & tBuild
end pathToRepoAndroidBinaries

private function pathToDex pRoot
   dispatch function "pathToDex" to stack kDeployLibrary with pRoot
   return the result
end pathToDex

private function pathToJavaC pRoot
   dispatch function "pathToJavaC" to stack kDeployLibrary with pRoot
   return the result
end pathToJavaC

private function pathToJava pRoot
   dispatch function "pathToJava" to stack kDeployLibrary with pRoot
   return the result
end pathToJava

private function pathToJarSigner pRoot
   dispatch function "pathToJarSigner" to stack kDeployLibrary with pRoot
   return the result
end pathToJarSigner

private function pathToJar pRoot
   dispatch function "pathToJar" to stack kDeployLibrary with pRoot
   return the result
end pathToJar

private function pathToPlayServices pRoot
   dispatch function "pathToPlayServices" to stack kDeployLibrary with pRoot
   return the result
end pathToPlayServices

################################################################################

-- MW-2013-06-13: [[ CloneAndRun ]] Map a file path from where the sb expects them to be to local
--   files in the build folder.
function mapFilePath pPath
   if revEnvironmentIsInstalled() then
      // SN-2015-01-19: [[ Bug 13213 ]] Linux filepaths are case-sensitive.
      //   so we try to caselessly match the targetted file, with the actual files.
      set the itemDelimiter to slash
      
      // Get the list of files in the targetted directory
      local tOldFolder, tFiles
      put the defaultFolder into tOldFolder
      set the defaultFolder to item 1 to -2 of pPath
      
      put the files into tFiles
      set the defaultFolder to tOldFolder
      
      local tLineOffset
      put lineOffset(last item of pPath, tFiles) into tLineOffset
      
      // We return empty if we can't find the specified file
      // That will allow us to throw an error in such a case.
      if tLineOffset is 0 then
         return empty
      else
         put line tLineOffset of tFiles into item -1 of pPath
      end if
      return pPath
   end if
   
   local tPath
   local tBinariesFolder, tComponent, tSuffix
   local tArch, tFoundArch
   repeat for each item tItem in revSBPlatformArchitectures("Android")
      if pPath contains ("/" & __ArchABI(tItem) & "/") then
         put tItem into tArch
      end if

      if tFoundArch is empty and \
            there is a folder pathToRepoAndroidBinaries(tItem) then
         put tItem into tFoundArch
      end if
   end repeat

   if tArch is empty then
      put tFoundArch into tArch
   end if

   put pathToRepoAndroidBinaries(tArch) into tBinariesFolder
   
   // SN-2015-07-02: [[ GypifiedAndroid ]] Throw an error if there is
   //  not the expected build folder
   if there is no folder tBinariesFolder then
      throw "Error: Android binaries folder" && tBinariesFolder && "could not be found"
   end if
   
   set the itemDelimiter to slash
   put the last item of pPath into tComponent
   
   if the editionType is "community" then
      put "Community" into tSuffix
   else
      put "Commercial" into tSuffix
   end if
   
   switch tComponent
      case "notify_icon.png"
         put merge("[[tBinariesFolder]]/notify_icon.png") into tPath
         break
      case "Manifest.xml"
         put merge("[[tBinariesFolder]]/Manifest.xml") into tPath
         break
      case "nfc_tech_filter.xml"
         put merge("[[tBinariesFolder]]/nfc_tech_filter.xml") into tPath
         break
      case "file_provider_paths.xml"
         put merge("[[tBinariesFolder]]/file_provider_paths.xml") into tPath
         break
      case "livecode_inputcontrol.xml"
         put merge("[[tBinariesFolder]]/livecode_inputcontrol.xml") into tPath
         break
      case "Classes"
         put merge("[[tBinariesFolder]]/Classes-[[tSuffix]]") into tPath
         break
      case "Standalone"
         put merge("[[tBinariesFolder]]/Standalone-[[tSuffix]]") into tPath
         break
      case "revzip"
         put merge("[[tBinariesFolder]]/RevZip") into tPath
         break
      case "revdb"
         put merge("[[tBinariesFolder]]/RevDb") into tPath
         break
      case "revxml"
         put merge("[[tBinariesFolder]]/RevXml") into tPath
         break
      case "dbsqlite"
         put merge("[[tBinariesFolder]]/DbSqlite") into tPath
         break
      case "dbmysql"
         put merge("[[tBinariesFolder]]/DbMysql") into tPath
         break
      case "revsecurity"
         put merge("[[tBinariesFolder]]/RevSecurity") into tPath
         break
      case "revpdfprinter"
         put merge("[[tBinariesFolder]]/RevPdfPrinter") into tPath
         break
      default
         put pPath into tPath
         break
   end switch
   
   return tPath
end mapFilePath

################################################################################

function androidCreateAppManifest pMainManifest, pAdditionalManifestsA
   local tMainArrayA, tAdditionalArraysA
   put androidManifestToArray(pMainManifest) into tMainArrayA
   repeat for each element tManifest in pAdditionalManifestsA
      appendToList androidManifestToArray(tManifest), tAdditionalArraysA
   end repeat
   
   MergeManifestArrays tMainArrayA, tAdditionalArraysA
   return ConvertArrayToXML(tMainArrayA, "utf-8")
end androidCreateAppManifest

command MergeManifestArrays @xManifestArrayA, pAdditionalsA
   -- Warning: at the moment we only support merging of
   -- features, permissions, and application metadata
   repeat for each element tAdditionalA in pAdditionalsA
      -- everything we're interested in comes under the 
      -- manifest root node.
      MergeManifestArray xManifestArrayA[1]["@children"], tAdditionalA[1]["@children"]
   end repeat
end MergeManifestArrays

private function FindIndex pArray, pName
   repeat for each key tIndex in pArray
      if pArray[tIndex]["@name"] is pName then
         return tIndex
      end if
   end repeat
   throw "no index found for" && pName
end FindIndex

private function FindEndOfBlockIndex pArray
   local tIndex
   repeat for each key tIndex in pArray
      if pArray[tIndex]["@children"] is not empty then
         exit repeat
      end if
   end repeat
   return tIndex
end FindEndOfBlockIndex

command MergeManifestArray @xManifestArrayA, pAdditionalsA
   -- Find the index of the end of this block and insert new items
   -- before it
   local tEndOfBlock
   put FindEndOfBlockIndex(xManifestArrayA) into tEndOfBlock
   repeat for each key tIndex in pAdditionalsA
      local tData
      put pAdditionalsA[tIndex] into tData
      switch tData["@name"] 
         case "application"
            -- Find the index of the application sub-array
            local tAppIndex
            put FindIndex(xManifestArrayA, "application") into tAppIndex
            MergeManifestArray xManifestArrayA[tAppIndex]["@children"], \
                  tData["@children"]
            break
         case "uses-sdk"
            -- ensure the maximum min sdk is used
            -- Find the index of the uses-sdk element
            local tUsesSDKIndex
            put FindIndex(xManifestArrayA, "uses-sdk") into tUsesSDKIndex
            local tExisting, tNew
            put xManifestArrayA[tUsesSDKIndex]["@attributes"]["android:minSDKVersion"] \
                  into tExisting
            put tData["@attributes"]["android:minSDKVersion"] into tNew
            put max(tExisting, tNew) into \
                  xManifestArrayA[tUsesSDKIndex]["@attributes"]["android:minSDKVersion"]
            break
         case "activity"
            local tActivityIndex
            put FindIndex(xManifestArrayA, "activity") into tActivityIndex
            local tExistingA, tNewA
            put xManifestArrayA[tActivityIndex]["@attributes"] \
                  into tExistingA
            put tData["@attributes"] into tNewA
            repeat for each line tKey in the keys of tNewA
               // do not merge the "theme", "name" and "exported" attributes
               if tKey is "android:theme" or tKey is "android:name" or tKey is "android:exported" then
                  next repeat
               end if
               put tNewA[tKey] into tExistingA[tKey]
            end repeat
            put tExistingA into xManifestArrayA[tActivityIndex]["@attributes"]
            break
         default
            insertIntoList tData, xManifestArrayA, tEndOfBlock
            add 1 to tEndOfBlock
            break
      end switch
   end repeat
end MergeManifestArray

function androidManifestToArray pManifestFile
   local tXML
   put url("binfile:" & pManifestFile) into tXML
   return ConvertXMLToArray(tXML, "utf-8", true, "")
end androidManifestToArray

/*
Handlers for converting XML text to LiveCode arrays and vice versa. 
Based on an implementation by Trevor DeVore of Blue Mango Learning Systems.
*/

/**
Converts an XML tree into a LiveCode multi-dimensional array.

Parameters:
pXML: The xml to convert.
pStoreEncodedAs: 
Encoding to use. Must be a value that can be passed to textEncode. Default is "utf-8".
*/
function ConvertXMLToArray pXML, pStoreEncodedAs
   local theArray,theResult,theRootNode,theTreeID
   local theXMLEncoding, tVersion
   
   ## Create an XML tree from XML text
   put revCreateXMLTreeWithNamespaces(pXML, true, true, false) into theTreeID
   
   if theTreeID is an integer then
      ## Determine the encoding of the XML, default to UTF-8
      put matchText(pXML, "<?xml (.)encoding=" & quote & "([^" & quote & "])", tVersion, theXMLEncoding) into theResult
      if theXMLEncoding is empty then put "utf-8" into theXMLEncoding
      
      ## Now convert to array. 
      ## The 1st dimension has one key which is the name of the root node.
      put revXMLRootNode(theTreeID) into theRootNode
      if theRootNode is not empty and not(theRootNode begins with "xmlerr,") then
         put ConvertXMLNodeToArray(theTreeID, theRootNode, theXMLEncoding, pStoreEncodedAs) \
               into theArray[1]
         put theRootNode into theArray[1]["@name"]
      end if
      
      revDeleteXMLTree theTreeID
   end if
   
   return theArray
end ConvertXMLToArray


/**
Converts a revXML created XML Tree to an array.
Parameters:
pXMLTree: The xml tree id.
pStoreEncodedAs: See docs for ConvertXMLToArray.

Description:
See docs for ConvertXMLToArray.
*/
function ConvertXMLTreeToArray pXMLTree, pStoreEncodedAs
   return ConvertXMLToArray(revXMLText(pXMLTree), pStoreEncodedAs)
end ConvertXMLTreeToArray


/**
Converts a multi-dimensional array to an XML tree.
Parameters:
pArray: The array to convert.
pArrayEncoding: Encoding used in the array. Must be a value that can be passed to textDecode.
pStoreEncodedAs: Encoding to use. Must be a value that can be passed to textEncode. Default is "utf-8".

Description:
The array should be keyed numerically, in order of appearance of nodes.
pArray[1] is assumend to be the root node in the XML tree.
each node is an array with three possible keys, 
- @name (required): The name of the node
- @attributes: The attributes of the node, stored as an array of 
  name/value pairs
- @children: A numerically keyed array of child nodes
- @value: The text value of the node
Returns: Text of the XML represented by the array, or error message.
*/
function ConvertArrayToXML pArray, pArrayEncoding, pStoreEncodedAs
   local theError,theRootNode,theXML,theXMLTree
   
   ## if pArrayEncoding is empty then current platform encoding is assumed
   if pStoreEncodedAs is empty then put "UTF-8" into pStoreEncodedAs
   
   ## Create XML for root node. Note that we take extra steps in order to support
   ## converting an array that only represents part of a tree rather than the entire tree.
   ## In this case there may be multiple nodes at the root level.
   put pArray[1]["@name"] into theRootNode 
   put "<" & theRootNode & "/>" into theXML
   
   local tEncoding
   ## Create XML needed to create tree
   put format("<?xml version=\"1.0\" encoding=\"%s\"?>", \
         pStoreEncodedAs) into tEncoding
   put tEncoding before theXML
   put revCreateXMLTreeWithNamespaces(theXML, true, true, false) into theXMLTree
   
   if theXMLTree is an integer then
      ## Loop over all nodes at root level
      ## Create tree using helper function
      local tName
      repeat for each element tNode in pArray
         put tNode["@name"] into tName
         replace space with "-" in tName
         ConvertArrayDimensionToXML tNode, theXMLTree, slash & tName, \
               tName, pArrayEncoding, pStoreEncodedAs
         put the result into theError
         
         if theError is not empty then exit repeat
      end repeat
      
      if theError is not empty then
         ## something went wrong, clean bad tree
         revDeleteXMLTree theXMLTree
      end if
   else
      put theXMLTree into theError
   end if
   
   if theError is not empty then
      return theError for error
   else
      return tEncoding & return & \
            revXMLText(theXMLTree, theRootNode, true) \
            for value
   end if
end ConvertArrayToXML
 
private command ConvertArrayDimensionToXML pArray, pTreeID, pNode, pName, pArrayEncoding, pStoreEncodedAs
   local theError,theKey,theKeys,tName
   put pName into tName
   repeat for each key tKey in pArray
      if tKey is "@name" then next repeat
      ## Look for attributes. These will be added as attributes to pNode.
      if tKey is "@attributes" or tKey is "@attr" then
         repeat for each key theKey in pArray[tKey]
            local theAttr
            put theKey into theAttr
            replace space with "-" in theAttr
            
            revSetXMLAttribute pTreeID, pNode, theAttr, \
                  EncodeString(pArray[tKey][theKey], \
                  pArrayEncoding, pStoreEncodedAs)
            if the result begins with "xmlerr," then 
               put the result && "(setting attribute" && theKey && "for node" && pNode & ")" into theError
            end if
            
            if theError is not empty then exit repeat
         end repeat
         
      else if tKey is "@value" then
         ## This XML tree is using complex structure. Node is the value of the parent node
         revPutIntoXMLNode pTreeID, pNode, EncodeString(pArray[tKey], pArrayEncoding, pStoreEncodedAs)
         if the result begins with "xmlerr," then
            put the result && "(adding child node" && tName && "to node" && pNode & ")" into theError
         end if
         
      else if tKey is "@children" then
         local tCounts
         repeat for each element tChildNode in pArray[tKey]
            add 1 to tCounts[tChildNode["@name"]]
            ## Node has children. Add node to XML tree then call self recursivly to create children nodes. 
            revAddXMLNode pTreeID, pNode, tChildNode["@name"], empty
            if the result begins with "xmlerr," then
               put the result && "(adding node" && tName & ")" into theError
            end if
            
            if theError is empty then
               ConvertArrayDimensionToXML tChildNode, pTreeID, \
                     pNode & slash & tChildNode["@name"] & "[" & tCounts[tChildNode["@name"]] & "]", \
                     tChildNode["@name"], pArrayEncoding, pStoreEncodedAs
               put the result into theError
            end if
         end repeat
      else
         ## Node has no children but possibly a value. Create node and add value (which may be empty).
         revAddXMLNode pTreeID, pNode, tKey, \
               EncodeString(pArray[tKey], pArrayEncoding, pStoreEncodedAs)
         if the result begins with "xmlerr," then
            put the result && "(adding child node" && tKey && "to node" && pNode & ")" into theError
         end if
      end if
      if theError is not empty then exit repeat
   end repeat
   
   return theError
end ConvertArrayDimensionToXML
 
private function ConvertXMLNodeToArray pTreeID, pNode, pXMLTreeEncoding, pStoreEncodedAs
   local theArrayA,theAttributes,theChildNode,theKey
   
   ## Look for attributes of the node. Store as array in "@attributes" key
   put revXMLAttributes(pTreeID, pNode, tab, cr) into theAttributes
   if theAttributes is not empty then
      put EncodeString(theAttributes, pXMLTreeEncoding, pStoreEncodedAs) into theAttributes
      split theAttributes by cr and tab -- create array
      put theAttributes into theArrayA["@attributes"]
   end if
   
   ## Look for children nodes. 
   set the itemDelimiter to slash
   put revXMLFirstChild(pTreeID, pNode) into theChildNode
   if theChildNode is empty or theChildNode begins with "xmlerr," then
      local theValue
      put EncodeString(revXMLNodeContents(pTreeID, pNode), pXMLTreeEncoding, pStoreEncodedAs) into theValue
      if word 1 to -1 of theValue is empty and the keys of theArrayA is not empty then
         ## Empty node that has attributes
         return theArrayA
         ## Force value into @value
         put theValue into theArrayA["@value"]
         return theArrayA
      end if
   else
      ## Child nodes were found. Recursively call self and store result in array.
      set the wholeMatches to true
      local tIndex
      repeat while theChildNode is not empty and not (theChildNode begins with "xmlerr,")
         add 1 to tIndex
         put the last item of theChildNode into theKey
         set the itemdelimiter to "["
         if the number of items in theKey > 1 then
            put item 1 to -2 of theKey into theKey
         end if    
         set the itemdelimiter to slash
         put ConvertXMLNodeToArray(pTreeID, theChildNode, pXMLTreeEncoding, pStoreEncodedAs) into theArrayA["@children"][tIndex]
         put theKey into theArrayA["@children"][tIndex]["@name"]
         put revXMLNextSibling(pTreeID, theChildNode) into theChildNode
      end repeat
      
      return theArrayA
   end if
end ConvertXMLNodeToArray
 
private function EncodeString pString, pInEncoding, pOutEncoding
   if pInEncoding is pOutEncoding then
      return pString
   end if
   
   if pInEncoding is not empty then
      -- if pOutEncoding is empty then pString will be converted to the current platform encoding
      put textDecode(pString, pInEncoding) into pString
   end if
   
   if pOutEncoding is not empty then
      -- if pInEncoding is empty then pString is assumed to be in the current platform encoding
      put textEncode(pString, pOutEncoding) into pString
   end if
   
   return pString
end EncodeString
