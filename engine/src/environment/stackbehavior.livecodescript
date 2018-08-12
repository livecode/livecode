script "EnvironmentStackBehavior"
################################################################################

local sTest
local sVersion
local sUnlocked
local sDevelopment
local sToken

local sGlobalLicensePath
local sLicensePath
local sToolsPath
local sEnginePath
local sHomePath
local sLogPath
local sRepositoryPath

local sActivateReason
local sActivateError
local sActivateEmail
local sActivatePassword
local sActivateHashedPassword
local sActivateSession
local sActivateChallenge
local sActivateToken

constant kActivationUrl="http://www.runrev.com/activation-community/livecode"

on startup pTest
   put pTest into sTest   
   
   put the version & "-" & the buildNumber into sVersion   
   put false into sUnlocked
   put false into sDevelopment
   
   put empty into sActivateReason
   put empty into sActivateEmail
   put empty into sActivatePassword
   put empty into sActivateHashedPassword
   
   -- When testing in the IDE we don't do some things that would be annoying...
   if not sTest then
      -- Set up the stack and such for activation
      set the navigationArrows to false
      set the allowInterrupts to false
      reset cursors
      set the loc of me to the screenLoc
   end if
   
   configureFonts the short name of me
   
   -- Configure the appropriate paths
   switch the platform
      case "win32"
         set the itemDelimiter to "/"
         put the effective filename of this stack into sEnginePath
         put item 1 to -2 of sEnginePath into sToolsPath
         put specialFolderPath(0x0023) & "/RunRev/Licenses" into sGlobalLicensePath
         put specialFolderPath(0x001a) & "/RunRev/Licenses" into sLicensePath
         break
      case "macos"
         set the itemDelimiter to "/"
         put the effective filename of this stack into sEnginePath
         put item 1 to -3 of sEnginePath & "/Tools" into sToolsPath
         put "/Library/Application Support/RunRev/Licenses" into sGlobalLicensePath
         put specialFolderPath("cusr") & "/Library/Application Support/RunRev/Licenses" into sLicensePath
         break
      default
         set the itemDelimiter to "/"
         put the effective filename of this stack into sEnginePath
         put item 1 to -2 of sEnginePath into sToolsPath
         put empty into sGlobalLicensePath
         put specialFolderPath("home") & "/.runrev/licenses" into sLicensePath
         break
   end switch
   
   guessRepositoryPath
   
   -- Run things as if the 'REV_TOOLS_PATH' var was set.
   if sRepositoryPath is not empty then
      if there is a folder (sRepositoryPath & "/ide-svn/Toolset") then
         put sRepositoryPath & "/ide-svn" into $REV_TOOLS_PATH
      else
         put sRepositoryPath & "/ide" into $REV_TOOLS_PATH
      end if
   end if
   
   --   if $0 is "activate" then
   --      performFacelessActivation
   --      quit 0
   --   else if $0 is "deactivate" then
   --      performFacelessDeactivation
   --      quit 0
   --   end if
   
   if $REV_TOOLS_PATH is not empty then
      put $REV_TOOLS_PATH into sToolsPath
      put sToolsPath & slash & "environment_log.txt" into sLogPath
      put true into sDevelopment
   end if
   
   if there is a file (sToolsPath & "/Startup.rev") then
      put sToolsPath & "/Startup.rev" into sHomePath
   else if  there is a file (sToolsPath & "/mchome.mc") then
      put sToolsPath & "/mchome.mc" into sHomePath
   else if there is a file (sToolsPath & "/Toolset/home.rev") then
      put sToolsPath & "/Toolset/home.rev" into sHomePath
   else if there is a file (sToolsPath & "/Toolset/home.livecodescript") then
      put sToolsPath & "/Toolset/home.livecodescript" into sHomePath
   else
      put the folder & "/Toolset/home.rev" into sHomePath
   end if
   
   log "Engine Path = " & sEnginePath
   log "Repository Path = " & sHomePath
   log "Tools Path = " & sToolsPath
   log "Global License Path = " & sGlobalLicensePath
   log "License Path = " & sLicensePath
   log "Home Path = " & sHomePath
   
   -- Make sure the licenses folder exists
   ensureFolder sLicensePath
   
   -- Now start the licensing process
   /*local tActivationDetails, tFirstRun
   fetchActivationDetails  tActivationDetails
   if the result is "not found" then
      put true into tFirstRun
   else
      put false into tFirstRun
      
      if the result is not "found" then
         put "automatic" into tActivationDetails["type"]
         put empty into tActivationDetails["email"]
         put empty into tActivationDetails["password"]
      end if
   end if
   
   -- Compute the token
   put activationLicenseFileForVersion(sVersion) into sToken*/
   
   -- EJB Activation Removal
   local tLicense
   
   fetchLicenseForVersion sVersion, tLicense
   licenseUnlock tLicense
   
   if firstRun() then
      go to card "AccountSignUp"
      return empty
   else
      return sHomePath
   end if
   
   /*local tLicense
   fetchLicenseForVersion sVersion, tLicense
   switch the result
      case "found"
         -- Attempt to unlock
         licenseUnlock tLicense
         switch the result
            case "licensed"
               -- The license we have for this version is valid
               if sTest then
                  answer "Success!"
               end if
               return sHomePath
            default
               -- Some other error occurred, so fall through to activation.
               put "The license for this version has become corrupted, please activate to continue." into sActivateReason
               break
         end switch
         break
      case "not found"
         if not tFirstRun then
            put "No license found for this version, please activate to continue." into sActivateReason
         end if
         break
      case "invalid"
         put "The current license for this version has become corrupted, please activate to continue." into sActivateReason
         break
      default
         break
   end switch */
   
   -- If we got to here, something went wrong, so make sure the token is the local
   -- one.
   /*put empty into sGlobalLicensePath
   put activationLicenseFileForVersion(sVersion) into sToken
   
   -- If we have previously saved activation details then what we do now depends on those.
   if tActivationDetails is an array then
      put tActivationDetails["email"] into sActivateEmail
      put empty into sActivatePassword
      put hexDecode(tActivationDetails["password"]) into sActivateHashedPassword      
   end if
   
   go to card "Automatic Activation"
   return empty */
end startup

on shutdown
   send "revUnloadLibrary" to stack "revLibURL"
   
   set the allowInterrupts to true
   
   if sUnlocked then
      return sHomePath
   end if
   
   return ""
end shutdown

on log pMessage
   if sDevelopment is true and sLogPath is not empty then
      open file sLogPath for text append
      write pMessage & return to file sLogPath
      close file sLogPath
   end if
end log

command configureFonts pStack
   switch the platform
      case "win32"
         if word 2 of the systemVersion >= 6  and "Segoe UI" is among the lines of the fontNames then
            set the textFont of stack pStack to "Segoe UI"
            set the textSize of stack pStack to 12
         else if word 2 of the systemVersion >= 5 and "Tahoma" is among the lines of the fontNames then
            set the textFont of stack pStack to "Tahoma"
            set the textSize of stack pStack to 11
         else
            set the textFont of stack pStack to "MS Sans Serif"
            set the textSize of stack pStack to 10
         end if
         break
      case "linux"
         set the textFont of stack pStack to "Helvetica"
         set the textSize of stack pStack to 12
         break
      case "macos"
         set the textFont of stack pStack to "Lucida Grande"
         set the textSize of stack pStack to 11
         break
   end switch
end configureFonts

function resolveImageId pImage
   return the id of image (pImage & ".png") of card "Resources" of stack "EnvironmentResources"
end resolveImageId

function resolveImageLongId pImage
   return the long id of image (pImage & ".png") of card "Resources" of stack "EnvironmentResources"
end resolveImageLongId

######################################################################

/*local sMovingOffset

on mouseDown
   put the clickLoc into sMovingOffset
end mouseDown

on mouseUp
   put empty into sMovingOffset
end mouseUp

on mouseRelease
   put empty into sMovingOffset
end mouseRelease

on mouseMove
   if sMovingOffset is not empty then
      get the globalLoc of the mouseLoc
      set the topLeft of me to item 1 of it - item 1 of sMovingOffset, item 2 of it - item 2 of sMovingOffset
   end if
end mouseMove*/

################################################################################

function titleColor
   return "#ffffff"
end titleColor

function headingColor
   return "#a7a7a7"
end headingColor

function subtitleColor
   return "#aaaaaa"
end subtitleColor

function bodyTextDullColor
   return "#878787"
end bodyTextDullColor

function livecodeColor
   return "#ffffff"
   --return "#569e54"
end livecodeColor

function livecodeLightColor
   return "#87af65"
end livecodeLightColor

function primaryErrorColor
   return "#FF9900"
   --return "#f3be26"
end primaryErrorColor

function secondaryErrorColor
   return "#f4dd9a"
end secondaryErrorColor

-- Larger font size for input fields and labels and such
function primaryFont
   return "Arial"
end primaryFont
function primaryFontSize
   return 16
end primaryFontSize

function entryFont
   return "Arial"
end entryFont
function entryFontSize
   return 18
end entryFontSize

-- Title of dialog
function titleFont
   return "Arial"
end titleFont
function titleFontSize
   return 38
end titleFontSize

-- Error and info message font
function messageFont
   if the platform is "win32" then
      return the textFont of this stack
   end if
   return "Arial"
end messageFont
function messageFontSize
   if the platform is "win32" then
      return the textSize of this stack
   end if
   return 12
end messageFontSize

################################################################################

function urlForCreateAccount
   return "http://www.runrev.com/links/livecode/activate-signup"
end urlForCreateAccount

command doCreateAccount
   launch url urlForCreateAccount()
end doCreateAccount

command doQuit
   if there is no stack "Home" then
      quit 0
   end if
end doQuit

command doContinueToLiveCode
   if there is no stack "Home" then
      close this stack
      exit to top
   end if
end doContinueToLiveCode

################################################################################

command clearActivationError
   put empty into sActivateError
end clearActivationError

function activationReason
   return sActivateReason
end activationReason

function activationError
   return sActivateError
end activationError

function activationLogin
   local tResult
   put sActivateEmail into tResult["email"]
   put sActivatePassword into tResult["password"]
   put sActivateHashedPassword into tResult["hash"]
   return tResult
end activationLogin

command performFacelessActivation
   -- Check we can faceless activate
   if sGlobalLicensePath is empty then
      write "All-user activation not supported on this platform." & return to stdout
      quit 1
   end if
   
   -- Check we can create/write into the license folder
   ensureFolder sGlobalLicensePath
   put empty into url ("file:" & sGlobalLicensePath & slash & "access.txt")
   if there is no file (sGlobalLicensePath & slash & "access.txt") then
      write "Unable to write to all-user license file location - make sure you are running as administrator." & return to stdout
      quit 2
   end if
   delete file (sGlobalLicensePath & slash & "access.txt")
   
   -- Check the required parameters have been supplied
   if $1 is not "-file" or $3 is not "-passphrase" or $4 is empty then
      write "Usage: <exe> activate -file <licensefile> -passphrase <passphrase>" & return to stdout
      quit 3
   end if
   
   -- Get the parameters
   local tLicenseFile
   put $2 into tLicenseFile
   if there is no file tLicenseFile then
      write "Specified license file could not be found." & return to stdout
      quit 4
   end if
   
   local tPassphrase
   put the sha1Digest of $4 into tPassphrase
   
   local tLicense
   put url ("file:" & tLicenseFile) into tLicense
   if the result is not empty then
      write "Could not load specified license file." & return to stdout
      quit 5
   end if
   
   -- Actually do the activation
   licenseUnlock tLicense, tPassphrase, true
   switch the result
      case "wrong challenge"
         write "Incorrect passphrase for that license." & return to stdout
         quit 6
      case "wrong version"
         write "The provided license was generated for a different version of LiveCode." & return to stdout
         quit 7
      case "invalid"
         write "The provided file is not a valid LiveCode license." & return to stdout
         quit 8
      case "lapsed"
         write "The provided license is no longer valid." & return to stdout
         quit 9
         
      case "trial licensed"
      case "trial expired"
         write "Trial licenses cannot be used with faceless activation." & return to stdout
         quit 10
         
      case "prerelease expired"
         write "The pre-release you are trying to activate has expired. Please download a newer version." & return to stdout
         quit 11
         
      case "prerelease licensed"
      case "licensed"
         storeLicenseForVersion sVersion, tLicense, tPassphrase, true
         break
         
      default
         write "An unknown error occurred." & return to stdout
         quit 12
   end switch
   
   write "All-user activation succeeded for version" && sVersion & return to stdout
end performFacelessActivation

command performFacelessDeactivation
   -- Check we can faceless activate
   if sGlobalLicensePath is empty then
      write "All-user activation not supported on this platform." & return to stdout
      quit 1
   end if
   
   -- Check we can create/write into the license folder
   ensureFolder sGlobalLicensePath
   put empty into url ("file:" & sGlobalLicensePath & slash & "access.txt")
   if there is no file (sGlobalLicensePath & slash & "access.txt") then
      write "Unable to write to all-user license file location - try running as administrator." & return to stdout
      quit 2
   end if
   delete file (sGlobalLicensePath & slash & "access.txt")
   
   -- Actually do the deactivation
   local tLicenseFile
   put activationLicenseFileForVersion(sVersion, true) into tLicenseFile
   
   if there is no file tLicenseFile then
      write "No all-user activation found for this version." & return to stdout
      quit 3
   end if
   
   delete file tLicenseFile
   if the result is not empty then
      write "Unable to remove all-user activation file for this version - try running as administrator." & return to stdout
      quit 4
   end if
   
   write "All-user deactivation succeeded for version" && sVersion & return to stdout
end performFacelessDeactivation

command performAutomaticActivation pEmail, pPassword, pHashedPassword
   -- Make sure we have libUrl
   if the backScripts is empty then
      send "revLoadLibrary" to stack "revLibURL"
   end if
   
   -- Clear out the automatic activation state
   put empty into sActivateSession
   put empty into sActivateChallenge
   put empty into sActivateToken
   
   -- Store the last attempted login
   put pEmail into sActivateEmail
   if pPassword is not empty then
      put pPassword into sActivatePassword
      put empty into sActivateHashedPassword
   else
      put empty into sActivatePassword
      put pHashedPassword into sActivateHashedPassword
   end if
   
   -- Hide the input panel
   if there is a group "Input" then
      hide group "Input"
   end if
   
   -- First do a 'login' request to fetch session and challenge
   local tSession, tChallenge
   automaticActivationLogin pEmail
   if the result is not "success" then
      return "error"
   end if
   
   -- Build up the token for authentication purposes
   local tToken
   put hexEncode(the md5Digest of (pHashedPassword & the md5Digest of sActivateChallenge)) into sActivateToken
   
   -- Now do a 'list' request to fetch the licenses the user can choose from
   local tListResponse
   automaticActivationList
   if the result is not "success" then
      return "error"
   end if   
end performAutomaticActivation

private command automaticActivationLogin pEmail
   -- Get the version name for the activation request.
   local tVersionName
   set the itemDelimiter to "-"
   put item 1 to -2 of sVersion into tVersionName
   set the itemDelimiter to comma
   
   -- Build the request
   local tPostData
   put "action=login" & return & \
         "email=" & pEmail & return & \
         "version=" & tVersionName into tPostData
   
   -- Submit the request
   local tPostResult, tPostError
   enterRemoteOperation
   post encodeForPost(tPostData) to kActivationUrl
   put the result into tPostError
   put it into tPostResult
   leaveRemoteOperation
   if the result is "cancelled" then
      put empty into sActivateError
      return "error"
   end if
   
   -- If a network error was returned, do the right thing
   if tPostError is not empty then
      automaticActivationSetPostError tPostError
      return "error"
   end if
   
   -- Extract the reponse
   split tPostResult by return and "="
   
   -- If it was unsuccessful, then do the right thing
   if tPostResult["result"] is not "success" then
      automaticActivationSetProtocolError tPostResult["result"]
      return "error"
   end if
   
   -- Act on the response
   put tPostResult["session"] into sActivateSession
   put hexDecode(tPostResult["challenge"]) into sActivateChallenge
   
   return "success"
end automaticActivationLogin

private command automaticActivationList
   -- Get the version name for the activation request.
   local tVersionName
   set the itemDelimiter to "-"
   put item 1 to -2 of sVersion into tVersionName
   set the itemDelimiter to comma
   
   -- Build the request
   local tPostData
   put "action=list" & return & \
         "session=" & sActivateSession & return & \
         "token=" & sActivateToken & return & \
         "version=" & tVersionName into tPostData
   
   -- Submit the request
   local tPostResult, tPostError
   enterRemoteOperation
   post encodeForPost(tPostData) to kActivationUrl
   put the result into tPostError
   put it into tPostResult
   leaveRemoteOperation
   if the result is "cancelled" then
      put empty into sActivateError
      return "error"
   end if
   
   -- If a network error was returned, do the right thing
   if tPostError is not empty then
      automaticActivationSetPostError tPostError
      return "error"
   end if
   
   -- Extract the reponse
   split tPostResult by return and "="
   
   -- If it was unsuccessful, then do the right thing
   if tPostResult["result"] is not "success" and tPostResult["result"] is not "nolicenses" then
      automaticActivationSetProtocolError tPostResult["result"]
      return "error"
   end if
   
   -- Act on the response for 'nolicenses'
   if tPostResult["result"] is "nolicenses" then
      put "An error occurred while processing the request. Please try again." into sActivateError
      return "error"
   end if
   
   -- Act on the response for success
   local tLicense
   put tPostResult into tLicense
   delete variable tLicense["result"]
   combine tLicense with return and "="   
   licenseUnlock tLicense
   switch the result
      case "licensed"
         -- The license we have for this version is valid
         storeActivationDetails
         storeLicenseForVersion sVersion, tLicense   
         if not sTest then
            libUrlResetAll
            repeat for each line tSocket in the openSockets
               close socket tSocket
            end repeat
            close this stack
         else
            answer "Success!"
         end if
         exit to top   
      default
         -- Some other error occurred, so fall through to activation.
         automaticActivationSetProtocolError "invalid"
         return "error"
         break
   end switch   
end automaticActivationList

private command enterRemoteOperation
   show group "Processing"
   dispatch "startLoading" to group "Processing"
   return empty
end enterRemoteOperation

private command leaveRemoteOperation
   hide group "Processing"
   dispatch "stopLoading" to group "Processing"
   return empty
end leaveRemoteOperation

private command automaticActivationSetPostError pPostError
   put "An error occurred while attempting to contact the server. Please try again later." into sActivateError
end automaticActivationSetPosterror

private command automaticActivationSetProtocolError pError
   switch pError
      case "unknown"
         -- license not found
         put "The server could not generate the requested license. Please try again." into sActivateError
         break
      case "unavailable"
         -- service unavailable
         put "The activation server is unavailable. Please try again later." into sActivateError
         break
      case "notsupported"
         -- version too old
         put "Automatic activation is not supported for this version. Please use the manual system." into sActivateError
         break
      case "timedout"
         -- session timedout
         put "The session timed-out. Please try again." into sActivateError
         break
      case "nosession"
         -- session vanished
         put "The session has lapsed. Please try again." into sActivateError
         break
      case "nouser"
         -- user not found
         put "The email you have entered is unrecognised." into sActivateError
         break
      case "nolicense"
         -- some sort of backend mismatch problem
         put "An error occurred generating the license key. Please try again." into sActivateError
         break
      case "disabled"
         -- account disabled
         put "That account has been disabled. Please contact support@runrev.com if you believe this to be in error." into sActivateError
         break
      case "wrongpassword"
         -- password wrong
         put "The password you have entered does not match that email address." into sActivateError
         break
      case "invalid"
      default
         -- bad request
         put "An error has occurred while contacting the server. Please try again." into sActivateError
         break
   end switch
end automaticActivationSetProtocolError

private function encodeForPost pData
   local tPostData
   set the itemDelimiter to "="
   repeat for each line tLine in pData
      put item 1 of tLine & "=" & the urlEncode of item 2 to -1 of tLine & "&" after tPostData
   end repeat
   delete the last char of tPostData
   return tPostData
end encodeForPost

################################################################################

function activationDetailsFile
   return sLicensePath & slash & "livecode-community.lcld"
end activationDetailsFile

function activationLicenseFileForVersion pVersion, pForceGlobal
   local tLeaf
   replace "." with "_" in pVersion
   put "livecode-community-" & pVersion & ".lclk" into tLeaf
   
   local tPath
   if sGlobalLicensePath is not empty then
      put sGlobalLicensePath & slash & tLeaf into tPath
      if there is a file tPath or pForceGlobal then
         return tPath
      end if
   end if
   
   return sLicensePath & slash & tLeaf
end activationLicenseFileForVersion

command fetchActivationDetails @rDetails
   local tDetailsFile
   put activationDetailsFile() into tDetailsFile
   
   if there is no file tDetailsFile then
      return "not found"
   end if
   
   local tDetails
   put url ("file:" & tDetailsFile) into tDetails
   if the first line of tDetails is not "---- LIVECODE ACTIVATION DETAILS (R1) ----" or \
          the last line of tDetails is not "---- LIVECODE ACTIVATION DETAILS (R1) ----" then
      return "invalid"
   end if
   
   delete the first line of tDetails
   delete the last line of tDetails
   
   split tDetails by return and "="
   put tDetails into rDetails
   return "found"
end fetchActivationDetails

command fetchLicenseForVersion pVersion, @rLicense
   local tLicenseFile
   put activationLicenseFileForVersion(pVersion) into tLicenseFile
   
   if there is no file tLicenseFile then
      return "not found"
   end if
   
   local tKey
   put url ("file:" & tLicenseFile) into tKey
   
   if the first line of tKey is not "---- LIVECODE ACTIVATION KEY (R1) ----" or \
          the last line of tKey is not "---- LIVECODE ACTIVATION KEY (R1) ----" then
      return "invalid"
   end if
   
   put line 2 to -2 of tKey into rLicense   
   return "found"
end fetchLicenseForVersion

command storeActivationDetails
   local tDetails
   put "---- LIVECODE ACTIVATION DETAILS (R1) ----" & return into tDetails
   put "email=" & sActivateEmail & return after tDetails
   if sActivateHashedPassword is not empty then
      put "password=" & hexEncode(sActivateHashedPassword) & return after tDetails
   else
      put "password=" & hexEncode(the md5Digest of sActivatePassword) & return after tDetails
   end if
   put "---- LIVECODE ACTIVATION DETAILS (R1) ----" after tDetails
   put tDetails into url ("file:" & activationDetailsFile())
end storeActivationDetails

command storeLicenseForVersion pVersion, pLicense, pForceGlobal
   local tKey
   put "---- LIVECODE ACTIVATION KEY (R1) ----" & return into tKey
   put pLicense & return after tKey
   put "---- LIVECODE ACTIVATION KEY (R1) ----" after tKey
   put tKey into url ("file:" & activationLicenseFileForVersion(pVersion, pForceGlobal))
end storeLicenseForVersion

################################################################################

-- This command unlocks the product with the given license.
command licenseUnlock pLicense
   local tSettings
   split pLicense by return and "="
   
   --   if "name" is not among the lines of the keys of pLicense or \
         --         "organization" is not among the lines of the keys of pLicense or \
         --         "time" is not among the lines of the keys of pLicense then
   --      return "invalid"
   --   end if
   
   -- Only include the token if it is not global
   if sGlobalLicensePath is not empty and sToken begins with sGlobalLicensePath then
      put empty into tSettings["token"]
   else
      put sToken into tSettings["token"]
   end if
   
   put pLicense["name"] into tSettings["name"]
   put pLicense["organization"] into tSettings["organization"]
   put "community" into tSettings["class"]
   put 1 into tSettings["multiplicity"]   
   put "macosx,windows,linux,ios,android,html5" into tSettings["deploy"]
   
   put "0"           into tSettings["scriptlimit"]
   put "0"           into tSettings["dolimit"]
   put "0"           into tSettings["usinglimit"]
   put "0"           into tSettings["insertlimit"]
   
   put true into sUnlocked
   
   do "set the revLicenseLimits to tSettings"   
   return "licensed"
end licenseUnlock

################################################################################

-- Encode a string of bytes as hexadecimal
function hexEncode pValue
   if pValue is empty then
      return empty
   end if
   
   local tHexValue
   get binaryDecode("H" & the length of pValue * 2, pValue, tHexValue)
   return tHexValue
end hexEncode

function hexDecode pHexValue
   if pHexValue is empty then
      return empty
   end if
   
   return binaryEncode("H" & the length of pHexValue, pHexValue)
end hexDecode

private command ensureFolder pFolder
   set the itemDelimiter to slash
   repeat with x = 2 to the number of items of pFolder
      if there is no folder item 1 to x of pFolder then
         create folder (item 1 to x of pFolder)
      end if
   end repeat
end ensureFolder

private function utf8ToNative pUtf8
   return uniDecode(uniEncode(pUtf8, "UTF8"), "Roman")
end utf8ToNative

################################################################################
on accountCreate pFirstname, pEmail, pPassword
   local tEncryptedPassword
   
   if the backScripts is empty then
      send "revLoadLibrary" to stack "revLibURL"
   end if
   
   show image "loader"
   
   if pPassword is not empty then 
      put hexEncode(md5digest(pPassword)) into tEncryptedPassword
   end if
   
   local tPostData
   put "firstname="& urlencode(pFirstname) & "&" & \
         "email=" & urlencode(pEmail) & "&" & \
         "password=" & urlencode(tEncryptedPassword) into tPostData
   
   local tPostResult, tPostError
   post tPostData to "http://livecode.com/livecode/register/"
   put the result into tPostError
   put it into tPostResult  
   
   hide image "loader"
   
   if char 1 of tPostResult is 1 then      
      put pEmail into sActivateEmail
      put tEncryptedPassword into sActivateHashedPassword
      storeActivationDetails
      accountSignupFinish
   else  if char 1 of tPostResult is 0 then    
      set the itemDel to "|"
      accountSignupMessage item 2 of tPostResult
   else
      accountSignupMessage "There was a problem when attempting to create your account. Please try again."
   end if
end accountCreate

command accountSignupSkip
   local tLicense
   fetchLicenseForVersion sVersion, tLicense
   licenseUnlock tLicense
   close this stack
end accountSignupSkip

command accountSignupFinish
   local tLicense
   fetchLicenseForVersion sVersion, tLicense
   licenseUnlock tLicense
   
   libUrlResetAll
   repeat for each line tSocket in the openSockets
      close socket tSocket
   end repeat
   close this stack
end accountSignupFinish

on accountSignupMessage pMessage
   put pMessage into field "signup_message" of card "AccountSignup"
   show field "signup_message" of card "AccountSignup"
end accountSignupMessage

on accountSignupMessageClose
   hide field "signup_message" of card "AccountSignup"
end accountSignupMessageClose

function firstRun
   -- Create a blank old style activation file. This tells the updater that
   -- this version has been installed.
   --
   local tLicense
   put "name=LiveCode Community User" & return & "organization=" & return & "time=" & the seconds into tLicense
   storeLicenseForVersion sVersion, tLicense   
   
   local tFirstRunPath, tCommunityLCLD, tCommercialLCLD  
   put sLicensePath & slash & "livecode-firstrun.lcf" into tFirstRunPath
   put sLicensePath & slash & "livecode-community.lcld" into tCommunityLCLD
   put sLicensePath & slash & "livecode.lcld" into tCommercialLCLD   
   if there is a file tFirstRunPath or there is a file  tCommunityLCLD or there is a file tCommercialLCLD then
      return false
   else
      put empty into url ("file:" & tFirstRunPath)
      return true
   end if
end firstRun

command guessRepositoryPath
   -- Build and package directories
   local tPlatform
   local tProcessor
   local tBuildDir
   local tPackageDir
   local tAltPackageDir
   
   switch the platform
      case "win32"
         put "win" into tPlatform
         break
      case "macos"
         put "mac" into tPlatform
         break
      default
         put the platform into tPlatform
         break
   end switch
   
   switch the processor
      case "i386"
         put "x86" into tProcessor
         break
      default
         put the processor into tProcessor
         break
   end switch
   
   put "build-" & tPlatform & "-" & tProcessor into tBuildDir
   put tPlatform & "-" & tProcessor & "-bin" into tPackageDir
   put tPlatform & "-bin" into tAltPackageDir
   
   set the itemDelimiter to "/"
   if tBuildDir is among the items of sEnginePath then
      put item 1 to (itemOffset(tBuildDir,sEnginePath) - 1) of sEnginePath into sRepositoryPath
   else if tPackageDir is among the items of sEnginePath then
      put item 1 to (itemOffset(tPackageDir,sEnginePath) - 1) of sEnginePath into sRepositoryPath
   else if tAltPackageDir is among the items of sEnginePath then
      put item 1 to (itemOffset(tAltPackageDir,sEnginePath) -1) of sEnginePath into sRepositoryPath
   else if "_build" is among the items of sEnginePath then
      put item 1 to (itemOffset("_build",sEnginePath) - 1) of sEnginePath into sRepositoryPath
   end if
   
   return sRepositoryPath
end guessRepositoryPath
