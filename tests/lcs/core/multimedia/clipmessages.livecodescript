script "CoreMultimediaClipMessages"

local sNewObject, sDeletedObject

on newAudioclip
   put the long id of the target into sNewObject
end newAudioclip

on newVideoclip
   put the long id of the target into sNewObject
end newVideoclip

on deleteAudioclip
   put the long id of the target into sDeletedObject
end deleteAudioclip

on deleteVideoclip
   put the long id of the target into sDeletedObject
end deleteVideoclip

on TestAudioclipMessages
   local tObject
   import audioclip from file (TestGetInputFile("testBlankAudio.wav"))
   put the long id of the last audioclip into tObject
   TestAssert "newAudioclip message", sNewObject is tObject
   delete tObject
   TestAssert "deleteAudioclip message", sDeletedObject is tObject
end TestAudioclipMessages

on TestVideoclipMessages
   local tObject
   import videoclip from file (TestGetInputFile("/testBlankVideo.mpeg"))
   put the long id of the last videoclip into tObject
   TestAssert "newVideoclip message", sNewObject is tObject
   delete tObject
   TestAssert "deleteVideoclip message", sDeletedObject is tObject
end TestVideoclipMessages
