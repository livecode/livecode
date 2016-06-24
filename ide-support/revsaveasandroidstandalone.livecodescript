script "revSaveAsAndroidStandalone"
################################################################################

local sJavaRoot
constant kArchs = "armeabi"

################################################################################

command revSaveAsMobileStandalone pStack, pApkFile, pTarget
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
      revSaveAsMobileStandaloneMain pStack, pApkFile, pTarget
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
   local tResult
   
   executeShellCommand pathToJava(), "-Xmx128M", "-classpath", pathToSDKClasses(), "com.android.sdklib.build.ApkBuilderMain", pUnalignedApkFile, pUnsignedParam, "-v", "-z", pAssetArchive, "-f", pClassesFile, "-nf", pLibsBuildFolder
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
         "but the" & tLimitType & " Android API available from LiveCode" && the version && \
         "is" && pLimit & "." & LF & \
         "Do you want to update the minimum Android API to" && pLimit & "?" \
         with "No" or "Yes" \
         titled "Minimum Android API error"
   
   return (it is "Yes")
end updateMinimumAPI

private function badAndroidApiMessage
   return "Not a valid Android API"
end badAndroidApiMessage

constant kAndroidExternals = "revzip revxml dbsqlite dbmysql dbodbc dbpostgresql"
private command revSaveAsMobileStandaloneMain pStack, pApkFile, pTarget
   dispatch "revIDEDeployAndroidInitialize" to stack "revDeployLibrary"
   
   local tSettings
   put the customProperties["cRevStandaloneSettings"] of stack pStack into tSettings
   
   -- If we're searching for inclusions, do it here 
   local tConfirm
   put true into tConfirm
   if tSettings["inclusions"] is "search" then 
      put false into tConfirm
      -- search for inclusions when building for simulator
      if pTarget contains "test" then
         revSBSearchForInclusions pStack, tSettings
      end if
      
   end if
   
   -- Make sure all dependencies are included
   revSBUpdateForDependencies tSettings
   
   -- Manually remove built-in implementations from detected script library inclusions
   revSBRemoveInclusions pStack, "android", tConfirm, tSettings
   
   -- Make sure old-style keys are retained, and updated with new information
   -- from inclusions pane of standalone settings GUI
   revSBConvertSettingsForPlatform tSettings, "android"
   
   -- Compute the base folder
   local tBaseFolder
   set the itemDelimiter to slash
   put item 1 to -2 of the effective filename of stack pStack into tBaseFolder
   set the itemDelimiter to comma
   
   -- Fetch the various options we need to initially build the app-bundle
   local tName, tFiles, tVersionName, tVersionCode, tExternals, tIcon, tSplash, tKey, tUnsigned, tStatusBarIcon
   put tSettings["name"] into tName
   put tSettings["files"] into tFiles
   put tSettings["android,icon"] into tIcon
   put tSettings["android,key"] into tKey
   
   if tSettings["android,label"] is empty then
      put tName into tSettings["android,label"]
   end if
   if tSettings["android,identifier"] is empty then
      put "com.yourcompany.yourapp" into tSettings["android,identifier"]
   end if
   if tSettings["android,version name"] is empty then
      put "1.0.0" into tSettings["android,version name"]
   end if
   if tSettings["android,version code"] is empty then
      put "1" into tSettings["android,version code"]
   end if
   if tSettings["android,initial orientation"] is empty then
      put "portrait" into tSettings["android,initial orientation"]
   end if
   if tSettings["android,status bar hidden"] is empty then
      put "false" into tSettings["android,status bar hidden"]
   end if
   
   // SN-2015-05-13: [[ AndroidVersions ]] Make sure that the minimum version is
   // the minimum version that we can use.
   local tMinimumVersion, tMaximumVersion
   put item 1 of revIDEDeployAndroidGetApisList() into tMinimumVersion
   put last item of revIDEDeployAndroidGetApisList() into tMaximumVersion
   
   if tSettings["android,minimum version"] is empty then
      put tMinimumVersion into tSettings["android,minimum version"]
   else if tSettings["android,minimum version"] < tMinimumVersion then
      // In case the minimum version is still 2.2 (for instance), we warn the user
      //  that we want to update it, before doing so.
      if updateMinimumAPI(tSettings["android,minimum version"], tMinimumVersion) then
         put tMinimumVersion into tSettings["android,minimum version"]
      else
         throw badAndroidAPIMessage()
      end if
   else if tSettings["android,minimum version"] > tMaximumVersion then
      if updateMinimumAPI(tSettings["android,minimum version"], tMaximumVersion) then
         put tMaximumVersion into tSettings["android,minimum version"]
      else
         throw badAndroidAPIMessage()
      end if
   end if
   
   put tSettings["android,statusBarIcon"] into tStatusBarIcon
   if tStatusBarIcon is empty then
      // put default sb icon path into tStatusBarIcon
      // SN-2015-01-22: [[ Bug 13213 ]] If mapFilePath returns empty, an error will be triggered
      // line 629.
      put mapFilePath(revMobileRuntimeFolder(pTarget) & slash & "notify_icon.png") into tStatusBarIcon
   end if
   
   switch tSettings["android,signing"]
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
      if tSettings["android,include" && tExternal] then
         if tExternal begins with "db" then 
            if "revdb" is not among the lines of tExternals then
               put "revdb" & return after tExternals
            end if
            put tExternal & return after tExternals
         else
            put tExternal & return after tExternals
         end if
      end if
   end repeat
   delete the last char of tExternals
   
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
      local tBuildFolder, tResBuildFolder, tDrawableResBuildFolder, tLayoutResBuildFolder, tClassesBuildFolder, tLibsBuildFolder, tArmLibsBuildFolder
      put tempName() into tBuildFolder
      create folder tBuildFolder
      if the platform is "win32" then
         put shortFilePath(tBuildFolder) into tBuildFolder
      end if
      
      put tBuildFolder & slash & "classes_app" into tClassesBuildFolder
      put tBuildFolder & slash & "libs" into tLibsBuildFolder
      
      // MERG: TODO when support for other Archs is added this needs work
      put tLibsBuildFolder & slash & kArchs into tArmLibsBuildFolder
      put tBuildFolder & slash & "res" into tResBuildFolder
      put tResBuildFolder & slash & "drawable" into tDrawableResBuildFolder
      put tResBuildFolder & slash & "layout" into tLayoutResBuildFolder
      create folder tClassesBuildFolder
      create folder tLibsBuildFolder
      create folder tArmLibsBuildFolder
      create folder tResBuildFolder
      create folder tDrawableResBuildFolder
      create folder tLayoutResBuildFolder
      if there is no folder tBuildFolder or there is no folder tClassesBuildFolder or \
            there is no folder tLibsBuildFolder or there is no folder tArmLibsBuildFolder or \
            there is no folder tResBuildFolder or there is no folder tDrawableResBuildFolder  or \
            there is no folder tLayoutResBuildFolder then
         throw "could not create build folders"
      end if
      
      if the platform is "win32" then
         put shortFilePath(tBuildFolder) into tBuildFolder
      end if
      
      -------- Unpack all the files used by the externals
      // MERG-2014-11-18: [[ TemplateManifest ]] Look for a template manifest provided by the developer in copy files
      local tManifestFile
      
      revStandaloneProgress "Integrating externals..."
      
      if tExternals is not empty then
         put return after tExternals
      end if
      set the itemDelimiter to "."
      repeat for each line tFile in tFiles
         revSBResolveIncludedExternal "android", tFile
         
         local tFilePath, tFileName
         computeAssetLocation tBaseFolder, tFile, tFilePath, tFileName
         
         -- Look for manifest template
         local tManifest
         if tFilePath ends with "AndroidManifest.xml" then
            put url ("binfile:" & tFilePath) into tManifest
         end if
         
         -- Only look for lcext files.
         if the last item of tFilePath is not "lcext" then
            next repeat
         end if
         
         -- Only process if the target exists
         if there is no file tFilePath then
            throw "could not find referenced external -" && tFile
         end if
         
         -- MERG-2013-09-05: [[ Bug 11152 ]] Only list the external if it was successfully
         --   extracted.
         -- Now attempt to process the external
         if addExternalFromFile(tFilePath, tClassesBuildFolder, tLibsBuildFolder) then
            -- Add the external to the list to load
            put char 1 to -7 of tFileName & return after tExternals
         end if
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
      // SN-2015-01-22: [[ Bug 13213 ]] Throw an error in case an error occured
      if the result is not empty then
         throw "Cannot find the template Manifest"
      end if
      replace "${LABEL}" with tSettings["android,label"] in tManifest
      replace "${NAME}" with ".mblandroid" in tManifest
      replace "${IDENTIFIER}" with tSettings["android,identifier"] in tManifest
      replace "${VERSION_NAME}" with tSettings["android,version name"] in tManifest
      replace "${VERSION_CODE}" with tSettings["android,version code"] in tManifest
      if there is a file tIcon then
         replace "${ICON}" with "android:icon=" & quote & "@drawable/icon" & quote in tManifest
      else
         replace "${ICON}" with empty in tManifest
      end if
      
      if tSettings["android,status bar hidden"] then
         replace "${THEME}" with "@android:style/Theme.NoTitleBar.Fullscreen" in tManifest
      else
         replace "${THEME}" with "@android:style/Theme.NoTitleBar" in tManifest
      end if
      replace "${ORIENTATION}" with tSettings["android,initial orientation"] in tManifest
      replace "${MIN_SDK_VERSION}" with tSettings["android,minimum version"] in tManifest
      
      local tUsesFeature, tUsesPermission, tFeatures, tPermissions, tAd
      repeat for each line tFeature in the keys of tSettings["android,device capabilities"]
         put "  <uses-feature android:name="&quote&"android."&tFeature&quote&" android:required="&quote&"${REQUIRED}"&quote&" />" into tUsesFeature
         replace "${REQUIRED}" with tSettings["android,device capabilities"][tFeature] in tUsesFeature
         put tUsesFeature & return after tFeatures
      end repeat
      repeat for each line tPermission in the keys of tSettings["android,capability permissions"]
         put "  <uses-permission android:name="&quote&"android.permission."&tPermission&quote&" />" into tUsesPermission
         put tUsesPermission & return after tPermissions
      end repeat
      repeat for each line tPermission in the keys of tSettings["android,application permissions"]
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
      if tSettings["android,inAppPurchasing"] then
         get tSettings["android,billingProvider"]
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
      
      replace "${SERVICE}" with tService in tManifest
      replace "${RECEIVER}" with tReceiver in tManifest
      replace "${USES_PERMISSION}" with tPermissions in tManifest
      
      if tAd is true then
         replace "${AD_ACTIVITY}" with "<activity android:name=" & quote & "com.inneractive.api.ads.InneractiveFullScreenView" & quote & "></activity>" in tManifest
      else
         replace "${AD_ACTIVITY}" with empty in tManifest
      end if
      
      local tInstallLocation
      switch tSettings["android,installLocation"]
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
      if tSettings["android,customUrlScheme"] is not empty then
         put "<intent-filter>" & \
               "<action android:name="&quote&"android.intent.action.VIEW"&quote&" />" & \
               "<category android:name="&quote&"android.intent.category.DEFAULT"&quote&" />" & \
               "<category android:name="&quote&"android.intent.category.BROWSABLE"&quote&" />" & \
               "<data android:scheme="&quote&tSettings["android,customUrlScheme"]&quote&" />" & \
               "</intent-filter>" into tCustomUrlFilter
      else
         put empty into tCustomUrlFilter
      end if
      replace "${CUSTOM_URL_FILTER}" with tCustomUrlFilter in tManifest
      
      local tPushPermissions
      local tPushReceiver
      if tSettings["android,pushSenderID"] is not empty then
         put "<permission android:name="&quote&tSettings["android,identifier"]&".permission.C2D_MESSAGE"&quote&" android:protectionLevel="&quote&"signature"&quote&" />" & \
               "<uses-permission android:name="&quote&tSettings["android,identifier"]&".permission.C2D_MESSAGE"&quote&" />" & \
               "<uses-permission android:name="&quote&"com.google.android.c2dm.permission.RECEIVE"&quote&" />" into tPushPermissions
         
         put "<receiver android:name="&quote&"PushReceiver"&quote&" android:permission="&quote&"com.google.android.c2dm.permission.SEND"&quote&">" & \
               "<intent-filter>" & \
               "<action android:name="&quote&"com.google.android.c2dm.intent.RECEIVE"&quote&" />" & \
               "<category android:name="&quote&tSettings["android,identifier"]&quote&" />" & \
               "</intent-filter>" & \
               "<intent-filter>" & \
               "<action android:name="&quote&"com.google.android.c2dm.intent.REGISTRATION"&quote&" />" & \
               "<category android:name="&quote&tSettings["android,identifier"]&quote&" />" & \
               "</intent-filter>" & \
               "</receiver>" into tPushReceiver
      else
         put empty into tPushPermissions
         put empty into tPushReceiver
      end if
      replace "${PUSH_PERMISSIONS}" with tPushPermissions in tManifest
      replace "${PUSH_RECEIVER}" with tPushReceiver in tManifest
      
      -- SN-2014-02-07: Add the email attachments provider in the manifest
      local tProvider
      put "<provider " & return into tProvider
      put "      android:name=" & quote & tSettings["android,identifier"] & ".AppProvider" & quote & return after tProvider
      put "      android:authorities=" & quote & tSettings["android,identifier"] & ".attachmentprovider" & quote & return after tProvider
      put "      android:exported=" & quote & "true" & quote & return after tProvider
      put "      android:grantUriPermissions=" & quote & "true" & quote & " >" & return after tProvider
      put "    </provider>" & return after tProvider
      
      replace "${PROVIDER}" with tProvider in tManifest
      
      // SN-2015-09-24: [[ Bug 15875 ]] Add the screenSize configChanges value for API > 12
      //  Otherwise the app restarts on orientation change.
      local tConfigChangesProp
      put "keyboardHidden|orientation" into tConfigChangesProp
      if tSettings["android,minimum version"] > 12 then
         put "|screenSize" after tConfigChangesProp
      end if
      replace "${CONFIG_CHANGES}" with tConfigChangesProp in tManifest
      
      // SN-2015-09-30: [[ Bug 10267 ]] Hardware Acceleration option added
      local tHardwareAccelerated
      if tSettings["android,hardware accelerated"] then
         put "true" into tHardwareAccelerated
      else
         put "false" into tHardwareAccelerated
      end if
      replace "${HARDWARE_ACCELERATED}" with tHardwareAccelerated in tManifest
      
      #put tManifest into url ("binfile:c:/scratch/manifestdump.xml")
      put tManifest into url ("binfile:" & tManifestFile)
      if the result is not empty then
         throw "could not create manifest"
      end if
      
      -------- Build the Classes
      revStandaloneProgress "Building classes..."
      
      -- MW-2012-07-05: Make sure we record all classes we build so we can clean up.
      local tCompiledClassFilePath, tBuiltClassFiles
      put empty into tBuiltClassFiles
      put tSettings["android,identifier"] into tCompiledClassFilePath
      replace "." with "/" in tCompiledClassFilePath
      
      local tClass, tClassFile
      put tBuildFolder & slash  & "classes_app/mblandroid.java" into tClassFile
      put "package" && tSettings["android,identifier"] & ";" & return into tClass
      put "import com.runrev.android.LiveCodeActivity;" & return after tClass
      put "public class mblandroid extends LiveCodeActivity {}" & return after tClass
      put tClass into url ("binfile:" & tClassFile)
      if the result is not empty then
         throw "could not create activity class"
      end if
      put tClassFile & return after tBuiltClassFiles
      
      local tLiveCodeClassesFile
      put tBuildFolder & slash & "LiveCode.jar" into tLiveCodeClassesFile
      put url ("binfile:" & mapFilePath(revMobileRuntimeFolder(pTarget) & slash & "Classes")) into url ("binfile:" & tLiveCodeClassesFile)
      if the result is not empty then
         throw "could not prepare LiveCode classes"
      end if
      
      local tClassPath
      if the platform is "win32" then
         put pathToRootClasses() & ";" & tLiveCodeClassesFile into tClassPath
      else
         put pathToRootClasses() & ":" & tLiveCodeClassesFile into tClassPath
      end if
      -- MM-2011-08-16: Set to target JDK version 1.5.  This is expected target bytecode that will be encoded by dex.
      executeShellCommand pathToJavaC(), "-target 1.5 -source 1.5 -Xlint:none -d", tClassesBuildFolder, "-cp", tClassPath, tClassFile
      if the result is not empty then
         throw "could not compile application class"
      end if
      put tBuildFolder & slash & "classes_app/" & tCompiledClassFilePath & "/mblandroid.class" & return after tBuiltClassFiles
      
      -- IM-2012-02-28: need barebones receiver for local notifications
      // build receiver class
      put tBuildFolder & slash & "classes_app/AppReceiver.java" into tClassFile
      put "package" && tSettings["android,identifier"] & ";" & return into tClass
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
      --         put "package" && tSettings["android,identifier"] & ";" & return into tBillingClass
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
      if tSettings["android,pushSenderID"] is not empty then
         local tPushReceiverClass, tPushReceiverClassFile
         put tBuildFolder & slash & "classes_app/PushReceiver.java" into tPushReceiverClassFile
         
         put "package" && tSettings["android,identifier"] & ";" & return into tPushReceiverClass
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
      
      -- SN-2014-02-07: Generate email attachment provider
      local tAttachmentClass, tAttachmentClassFile      
      put tBuildFolder & slash & "classes_app/AppProvider.java" into tAttachmentClassFile
      
      put "package" && tSettings["android,identifier"] & ";" & return into tAttachmentClass
      put "import android.content.*;" & return after tAttachmentClass
      put "import android.net.Uri;" & return after tAttachmentClass
      put "import android.app.*;" & return after tAttachmentClass
      put "import android.database.*;" & return after tAttachmentClass
      put "import java.io.*;" & return after tAttachmentClass
      put "import android.util.Log;" & return after tAttachmentClass
      put "import android.os.ParcelFileDescriptor;" & return after tAttachmentClass
      put "import com.runrev.android.AttachmentProvider;" & return after tAttachmentClass
      put "public class AppProvider extends ContentProvider {" & return after tAttachmentClass
      put "public static final String URI = " & quote & "content://" & tSettings["android,identifier"] & ".attachmentprovider" & quote & ";" & return after tAttachmentClass
      put "public static final String AUTHORITY = " & quote & tSettings["android,identifier"] & ".attachmentprovider" & quote & ";" & return after tAttachmentClass
      put "private com.runrev.android.AttachmentProvider m_attachments;" & return after tAttachmentClass
      put "@Override" & return & "public boolean onCreate() { m_attachments = new AttachmentProvider(getContext()); return true; }" & return after tAttachmentClass
      put "@Override" & return & "public ParcelFileDescriptor openFile(Uri uri, String mode) throws FileNotFoundException { Log.i(" & quote & "revandroid" & quote & ", uri.toString()); return m_attachments.doOpenFile(uri); }" & return after tAttachmentClass
      put "@Override" & return & "public Cursor query (Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder) { Log.i(" & quote & "revandroid" & quote & ", " & quote & "query: " & quote & " + uri.toString()); return m_attachments.doQuery(uri, projection, selection, selectionArgs, sortOrder); }" & return after tAttachmentClass
      put "@Override" & return & "public Uri insert (Uri uri, ContentValues p_values) { return m_attachments.doInsert(uri, p_values); }" & return after tAttachmentClass
      put "@Override" & return & "public int update (Uri uri, ContentValues values, String selection, String[] selectionArgs) { return 0; }" & return after tAttachmentClass
      put "@Override" & return & "public int delete (Uri uri, String selection, String[] selectionArgs) { return m_attachments.doDelete(uri, selection, selectionArgs); }" & return after tAttachmentClass
      put "@Override" & return & "public String getType (Uri uri){ return m_attachments.doGetType(uri); }" & return after tAttachmentClass      
      put "}" & return after tAttachmentClass
      put tAttachmentClass into url ("binfile:" & tAttachmentClassFile)
      if the result is not empty then
         throw "could not create email attachment provider"
      end if
      put tAttachmentClassFile & return after tBuiltClassFiles
      
      executeShellCommand pathToJavac(), "-target 1.5 -source 1.5 -Xlint:none -d", tClassesBuildFolder, "-cp", tClassPath, tAttachmentClassFile
      if the result is not empty then
         throw "could not compile email attachment provider" & return & the result
      end if
      put tBuildFolder & slash & "classes_app/" & tCompiledClassFilePath & "/AppProvider.class" & return after tBuiltClassFiles
      
      local tClassesFile
      put tBuildFolder & slash & "classes.dex" into tClassesFile
      executeShellCommand pathToDex(), "--dex", "--output=" & quote & tClassesFile & quote, tClassesBuildFolder, tLiveCodeClassesFile
      if the result is not empty then
         throw "could not encode class bundle"
      end if
      
      ------- Compile the Standalone
      revStandaloneProgress "Building executable..."
      
      local tDeploy
      -- Update the deploy params with script library and extension inclusions
      revSBUpdateDeployParams pStack, pTarget, tSettings, tDeploy
      
      -- Engine to use
      put mapFilePath(revMobileRuntimeFolder(pTarget) & slash & "Standalone") into tDeploy["engine"]
      // SN-2015-01-22: [[ Bug 13213 ]] Throw an error in case an error occured
      if there is no file tDeploy["engine"] then
         throw "Could not find standalone engine"
      end if
      -- Stackfile to use
      put the effective filename of stack pStack into tDeploy["stackfile"]
      
      -- Externals to reference (on Android we include drivers in this list)
      repeat for each line tEntry in tExternals
         put tEntry & return after tDeploy["externals"]
      end repeat
      delete the last char of tDeploy["externals"]
      
      -- Output file to create
      put tArmLibsBuildFolder & slash & "librevandroid.so" into tDeploy["output"]
      
      -- Splash to use (if non-commercial)
      if line 3 of the revLicenseInfo is among the words of "Educational Personal" then
         -- MW-2011-03-17: Make sure absolute paths work correctly
         if tSettings["android,splash"] is not empty and pathIsRelative(tSettings["android,splash"]) then
            put tBaseFolder & slash & tSettings["android,splash"] into tDeploy["splash"]
         else
            put tSettings["android,splash"] into tDeploy["splash"]
         end if
      end if
      
      _internal deploy android tDeploy
      if the result is not empty then
         throw the result
      end if
      
      -------- Assembling Assets
      revStandaloneProgress "Assembling assets..."
      
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
      
      -- MW-2012-07-04: On Linux, 'aapt' can complain about libz version info
      --   so we delete that line from the result before checking for an error.
      if there is a folder (tBuildFolder & slash & "res") then
         executeShellCommand pathToAapt(), "package", "-f", "-M", tManifestFile, "-I", pathToRootClasses(),"-F", tAssetArchive, "-S", (tBuildFolder & slash & "res")
      else
         executeShellCommand pathToAapt(), "package", "-f", "-M", tManifestFile, "-I", pathToRootClasses(),"-F", tAssetArchive
      end if
      get the result
      if line 1 of it contains "no version information available" then
         delete line 1 of it
      end if
      // SN-2014-10-27: [[ Bug 13800 ]] libpng 1.6+ raises a warning for the PNG files having embedded sRGB profile
      if line 1 of it contains "iCCP: Not recognizing known sRGB profile that has been edited" then
         delete line 1 of it
      end if
      if it is not empty then
         throw "could not generate package manifest"
      end if
      
      revZipOpenArchive tAssetArchive, "update"
      if the result is not empty then
         throw "could not open asset archive"
      end if
      
      addAssetsToArchive tFiles, tBaseFolder, tAssetArchive
      
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
      
      -- MM-2014-01-29: [[ OpenSSL ]] Include the revsecurity library.
      if tSettings["android,include revsecurity"] then
         put url ("binfile:" & mapFilePath(revMobileRuntimeFolder(pTarget) & slash & "revsecurity")) into url ("binfile:" & tArmLibsBuildFolder & slash & "librevsecurity.so")
         
         // SN-2015-01-22: [[ Bug 13213 ]] Throw an error in case an error occured
         if the result is not empty then
            throw "Could not find revsecurity library"
         end if
      end if
      
      -- Copy across the externals into the libs folder
      repeat for each line tEntry in tExternals
         -- Only copy across an external from the Runtime folder if we haven't
         -- already copied from a lcext.
         if there is a file (tArmLibsBuildFolder & slash & "lib" & tEntry & ".so") then
            next repeat
         end if
         put url ("binfile:" & mapFilePath(revMobileRuntimeFolder(pTarget) & slash & tEntry)) into url ("binfile:" & tArmLibsBuildFolder & slash & "lib" & tEntry & ".so")
         // SN-2015-01-22: [[ Bug 13213 ]] Throw an error in case an error occured
         if the result is not empty then
            throw "Could not find " & tEntry && "library"
         end if
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
      if not (the result contains "=> lib/armeabi/librevandroid.so") then
         throw "apk preparation failed"
      end if
      
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
   if there is a file (tArmLibsBuildFolder & slash & "librevandroid.so") then delete file  (tArmLibsBuildFolder & slash & "librevandroid.so")
   if there is a file (tArmLibsBuildFolder & slash & "librevsecurity.so") then delete file  (tArmLibsBuildFolder & slash & "librevsecurity.so")
   repeat for each line tEntry in tExternals
      if there is a file (tArmLibsBuildFolder & slash & "lib" & tEntry & ".so") then delete file (tArmLibsBuildFolder & slash & "lib" & tEntry & ".so")
   end repeat
   if there is a folder tArmLibsBuildFolder then delete folder tArmLibsBuildFolder
   if there is a folder tLibsBuildFolder then delete folder tLibsBuildFolder
   if there is a file tClassesFile then delete file tClassesFile
   repeat for each line tClassFile in tBuiltClassFiles
      if there is a file tClassFile then delete file tClassFile
   end repeat
   set the itemDelimiter to slash
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

private command computeAssetLocation pBaseFolder, pFile, @rPath, @rName
   local tIsAbsolute
   put pFile begins with "/" or char 2 to 3 of pFile is ":/" into tIsAbsolute
   
   set the itemDelimiter to slash
   if the last item of pFile is "*" then
      delete the last item of pFile
   end if
   
   if tIsAbsolute then
      put item -1 of pFile into rName
      put pFile into rPath
   else
      -- MM-2011-09-07 [[ Bug 10345 ]] Use the full path to the file (rather than just name), ensureing we create sub-folders.
      put pFile into rName
      put pBaseFolder & slash & pFile into rPath
   end if
end computeAssetLocation

private command addAssetsToArchive pFiles, pBaseFolder, pArchive
   local tDeviceConfig, tConfigFile, tCustomConfigFile, tFonts
   
   put mapFIlePath(revMobileRuntimeFolder() & slash & "lc_device_config.txt") into tConfigFile
   if there is not a file tConfigFile then
      put empty into tConfigFile
   end if
   
   repeat for each line tFile in pFiles
      local tName
      computeAssetLocation pBaseFolder, tFile, tFile, tName
      if there is a file tFile then
         if tConfigFile is not empty and tName is "lc_device_config.txt" then
            // need to merge config files
            put tFile into tCustomConfigFile
         else if tFile ends with ".ttf" or tFile ends with ".ttc" then
            -- MM-2012-03-07: [[ Custom fonts ]] If any fonts are found, add to the list so that they are handled correctly
            put tFile & return after tFonts
         else if tFile ends with ".lcext" or tFile ends with ".dylib" then
            -- MW-2012-07-05: [[ Externals ]] If an 'lcext' file or dylib file is found then we ignore it.
         else if tFile ends with "AndroidManifest.xml" then
            -- MW-2015-04-24: [[ TemplateAppMetadata ]] If an template manifest is found then we ignore it.
         else
            revZipAddUncompressedItemWithFile pArchive, "assets/" & tName, tFile
         end if
      else if there is a folder tFile then
         addAssetsFromFolderToArchive tFile, "assets" & slash & tName, pArchive, tFonts
      else
         throw "could not find referenced asset to include -" && tFile
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
   
   local tOldFolder
   put the folder into tOldFolder
   set the folder to pFolder
   if the folder is not pFolder then
      throw "could not descend into referenced asset folder -" && pFolder
   end if
   
   repeat for each line tFile in the files
      if tFile ends with ".ttf" or tFile ends with ".ttc" then
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
private function addExternalFromFile pExternal, pClassesFolder, pLibsFolder
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
      
      repeat for each word tArch in kArchs
         filter tZipItems with "*" & tArch & "*" into tZipItemsFiltered
         put pLibsFolder & slash & tArch & slash & "lib" & tExternalName & ".so" into tLibFileA[tArch]
         
         repeat for each line tZipItem in tZipItemsFiltered
            if tZipItem ends with ".so" then
               revZipExtractItemToFile pExternal, tZipItem, \
                     pLibsFolder & slash & tArch & slash & the last item of tZipItem 
            else 
               # will be External-<Arch> file
               revZipExtractItemToFile pExternal, tZipItem, tLibFileA[tArch]
            end if
         end repeat
      end repeat
      
      revZipCloseArchive pExternal
   end if
   
   if there is a file tClassesFile then
      repeat for each word tArch in kArchs
         if there is no file tLibFileA[tArch] then
            throw "android external contains classes but no library for " & tArch
         end if
      end repeat
      
      return true
   end if
   
   return false
end addExternalFromFile

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

private function pathToRootClasses pRoot
   // SN-2014-10-16: [[ ScriptifiedStack ]] Stack name updated
   dispatch function "pathToRootClasses" to stack "revDeployLibraryAndroid" with pRoot
   return the result
end pathToRootClasses

private function pathToSDKClasses pRoot
   // SN-2014-10-16: [[ ScriptifiedStack ]] Stack name updated
   dispatch function "pathToSDKClasses" to stack "revDeployLibraryAndroid" with pRoot
   return the result
end pathToSDKClasses

private function pathToAapt pRoot
   // SN-2014-10-16: [[ ScriptifiedStack ]] Stack name updated
   dispatch function "pathToAapt" to stack "revDeployLibraryAndroid" with pRoot
   return the result
end pathToAapt

private function pathToAdb pRoot
   // SN-2014-10-16: [[ ScriptifiedStack ]] Stack name updated
   dispatch function "pathToAdb" to stack "revDeployLibraryAndroid" with pRoot
   return the result
end pathToAdb

private function pathToZipAlign pRoot
   // SN-2014-10-16: [[ ScriptifiedStack ]] Stack name updated
   dispatch function "pathToZipAlign" to stack "revDeployLibraryAndroid" with pRoot
   return the result
end pathToZipAlign

// SN-2015-07-02: [[ GypifiedAndroid ]] Get the right folder for android binaries
private function pathToRepoAndroidBinaries
   local tBuild
   if revEnvironmentBinariesPath() contains "/Debug" then
      put "Debug" into tBuild
   else
      put "Release" into tBuild
   end if
   return revEnvironmentRepositoryPath() & "/build-android-armv6/livecode/out/" & tBuild
end pathToRepoAndroidBinaries

private function pathToDex pRoot
   // SN-2014-10-16: [[ ScriptifiedStack ]] Stack name updated
   dispatch function "pathToDex" to stack "revDeployLibraryAndroid" with pRoot
   return the result
end pathToDex

private function pathToJavaC pRoot
   // SN-2014-10-16: [[ ScriptifiedStack ]] Stack name updated
   dispatch function "pathToJavaC" to stack "revDeployLibraryAndroid" with pRoot
   return the result
end pathToJavaC

private function pathToJava pRoot
   // SN-2014-10-16: [[ ScriptifiedStack ]] Stack name updated
   dispatch function "pathToJava" to stack "revDeployLibraryAndroid" with pRoot
   return the result
end pathToJava

private function pathToJarSigner pRoot
   // SN-2014-10-16: [[ ScriptifiedStack ]] Stack name updated
   dispatch function "pathToJarSigner" to stack "revDeployLibraryAndroid" with pRoot
   return the result
end pathToJarSigner

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
   set the itemDelimiter to slash
   put pathToRepoAndroidBinaries() into tBinariesFolder
   
   // SN-2015-07-02: [[ GypifiedAndroid ]] Throw an error if there is
   //  not the expected build folder
   if there is no folder tBinariesFolder then
      throw "Error: Android binaries folder" && tBinariesFolder && "could not be found"
   end if
   
   put the last item of pPath into tComponent
   put the editionType into tSuffix
   put toUpper(char 1 of tSuffix) into char 1 of tSuffix
   switch tComponent
      case "notify_icon.png"
         put merge("[[tBinariesFolder]]/notify_icon.png") into tPath
         break
      case "Manifest.xml"
         put merge("[[tBinariesFolder]]/Manifest.xml") into tPath
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
      default
         put pPath into tPath
         break
   end switch
   
   return tPath
end mapFilePath

################################################################################
