script "InstallerStackBehavior"
################################################################################
#
#    STARTUP METHOD
#

on startup
   local tAction
   
   if $1 is "install" then
      put "install" into tAction
   else if $1 is "doinstall" then
      put "doinstall" into tAction
   else if $1 is "uninstall" then
      put "uninstall" into tAction
   else if $1 is "douninstall" then
      put "douninstall" into tAction
   else
      set the itemDelimiter to slash
      if the last item of $0 contains "setup" then
         put "uninstall" into tAction
      else
         put "install" into tAction
      end if
   end if
   
   switch tAction
      case "install"
         if the environment is "installer command line" then
            runFacelessInstall
         else
            hide me
            send "runInstallerUI" to me in 0 millisecs
         end if
         break
      case "uninstall"
         if the environment is "installer command line" then
            runFacelessUninstall
         else
            hide me
            send "runUninstallerUI" to me in 0 millisecs
         end if
         break
      case "doinstall"
         runInstallerActions
         break
      case "douninstall"
         runUninstallerActions
         break
      default
         quit 1
   end switch
end startup

-- MM-2011-04-01: Catch the relaunch message to make sure only a single installer is ever running
-- Ensures only a single updater is ever run, even if launched by multiple browsers.
on relaunch pArgs
   if pArgs is empty then
      return empty
   else
      pass relaunch
   end if
end relaunch

////////////////////////////////////////////////////////////////////////////////

// Utility functions removed from the "Tools Installer" stack.

function resolveImageId pImage
   return the id of image (pImage & ".png") of stack "Resources"
end resolveImageId

function resolveImageLongId pImage
   return the long id of image (pImage & ".png") of stack "Resources"
end resolveImageLongId

function getInstallerType
   return the cInstallerType of card "Resources" of stack "Resources"
end getInstallerType

command configureFonts pStack
    set the textFont of stack pStack to empty
end configureFonts

command setTitleField pField, pTitle, pSubTitle
   if pSubTitle is empty then
      set the text of pField to pTitle
      set the textStyle of word 1 to -1 of pField to "bold"
      set the textColor of word 1 to -1 of pField to "#444444"
   else
      set the text of pField to pSubTitle && "//" && pTitle
      set the textColor of word 1 to -1 of pField to "#555555"
      set the textStyle of word (-the number of words of pTitle) to -1 of pField to "bold"
      set the textColor of word (-the number of words of pTitle) to -1 of pField to "#444444"
   end if
end setTitleField

on errorDialog pError
   if the environment is "installer command line" then
      write "[" && the internet date && "]" && ": ERROR " && pError & return to stdout
      quit 1
   else
      answer error "An error occurred when running the installer. Please submit a screenshot of this dialog to http://quality.livecode.com" & return & pError
      quit 1
   end if
end errorDialog
