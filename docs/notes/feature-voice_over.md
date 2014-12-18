# Voice Over support

**What has changed?**

Prior to LC 6.6.4 RC3 one could not interact with LC stacks when Voice Over was turned on, for example, a LC button would not respond to touch events. This changed in LC 6.6.4 RC3, but broke interaction with native iOS controls, for example, links inside a mobile browser window did not work as expected with Voice Over. To address this issue, we added a new property to iOS native controls, **ignoreVoiceOverSensitivity**, that allow turning off Voice Over sensitivity for those objects. 

Example:

on ShowVOBrowser
   global gVOBrowserID
   if the platform = "iPhone" then
      if gVOBrowserID = empty then
         MobileControlCreate "browser"
         put the result into gVOBrowserID
      end if
      mobilecontrolset gVOBrowserID,"dataDetectorTypes","link"
      mobilecontrolset gVOBrowserID, "ignoreVoiceOverSensitivity","true"      
      mobilecontrolset gVOBrowserID,"rect”,”0,0,300,300”
      mobilecontrolset gVOBrowserID,"visible",true    
   end if
end ShowVOBrowser

on hideVOBrowser
   global gVOBrowserID
   if the platform = "iPhone" then
      mobilecontrolset gVOBrowserID,"visible",false
      mobilecontrolset gVOBrowserID, "ignoreVoiceOverSensitivity","false"
   end if
end hideVOBrowser

Moreover, we added a new LiveCode function, **mobileIsVoiceOverRunning**, to detect if Voice Over is turned on.
