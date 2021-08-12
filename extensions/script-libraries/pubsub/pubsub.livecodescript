script "com.livecode.script-library.pubsub"
/*

Name: Publish-Subscribe Library

Type: library

Version: 0.0.1

Summary:
This library implements a publis-subscribe model for messages in LiveCode

*/

-- Stores subscriptions in the following format
-- sSubscriptions[message][callback targets] = List of object ID's
local sSubscriptions

private command __AddSubscription pMessage, pCallingObject
   if sSubscriptions[pMessage]["callback targets"] is empty then
      put pCallingObject into sSubscriptions[pMessage]["callback targets"]
   else if pCallingObject is not among the lines of sSubscriptions[pMessage]["callback targets"] then
      put return & pCallingObject after sSubscriptions[pMessage]["callback targets"]
   else
      return "Already subscribed" for error
   end if
end __AddSubscription

private command __Subscribe pMessage, pObjectList
   # Get the log ID of the calling object
   local tCallingObjectID
   put the long ID of the target into tCallingObjectID
   
   # Special case propertyChanged where we have to register for listeners for obejcts
   if pMessage is "idePropertyChanged" then
      repeat for each line tObject in pObjectList
         revIDEPropertyRegisterListenerForObject tObject
         __ideAddSubscription pMessage & "," & tObject, tCallingObjectID
      end repeat
   else if pObjectList is not empty then
      repeat for each line tObject in pObjectList
         __ideAddSubscription pMessage & "," & tObject, tCallingObjectID
      end repeat
   else
      __ideAddSubscription pMessage, tCallingObjectID
   end if
   
   if the result is not empty then
      return the result
   end if
   
   return empty
end __Subscribe

private command __Unsubscribe pMessage, pObject, pObjectList
   # If not object is specified, get the long ID of the calling object
   if pObject is empty then
      put the long ID of the target into pObject
   end if
   
   # Special case propertyChanged where we have to register for listeners for obejcts
   if pMessage is "idePropertyChanged" then
      repeat for each line tObject in pObjectList
         delete line lineoffset(pObject, sSubscriptions[pMessage & "," & tObject]["callback targets"]) of sSubscriptions[pMessage & "," & tObject]["callback targets"]
         # If the object is no longer needed, unlisten.
         if sSubscriptions[pMessage & "," & tObject]["callback targets"] is empty then
            revIDEPropertyDeregisterListenerForObject tObject
         end if
      end repeat
   else if pMessage is "ideMoveStack" then
      repeat for each line tObject in pObjectList
         delete line lineoffset(pObject, sSubscriptions[pMessage & "," & tObject]["callback targets"]) of sSubscriptions[pMessage & "," & tObject]["callback targets"]
      end repeat
   else
      # Delete the calling object from the message: sSubscriptions[pMessage]["callback targets"]
      if lineoffset(pObject, sSubscriptions[pMessage]["callback targets"]) > 0 then
         delete line lineoffset(pObject, sSubscriptions[pMessage]["callback targets"]) of sSubscriptions[pMessage]["callback targets"]
      end if
   end if
end __Unsubscribe

private command __UnsubscribeAll pObject
   # If not object is specified, get the long ID of the calling object
   if pObject is empty then
      put the long ID of the target into pObject
   end if
   
   # Loop through messages and unsubscribe
   repeat for each key tMessage in sSubscriptions
      __Unsubscribe tMessage, pObject
   end repeat
end __UnsubscribeAll

private function __Subscriptions
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
end __Subscriptions

private function __SubscribedObjects pMessage
   return sSubscriptions[pMessage]["callback targets"]
end __SubscribedObjects


private command __PublishWithParameters pMessage, pTriggeringObjects, pParamsArray
   lock screen
   
   # If in run mode don't send any updates
   //if the tool is not "pointer tool" then exit ideMessageSendWithParameters
   
   local tSubscriptionList, tTargetsA, tSubscription
   # Add all universal subscribers to the subscription list
   put sSubscriptions[pMessage]["callback targets"] into tSubscriptionList
   
   # Add any per-object subscriptions
   repeat for each line tLine in pTriggeringObjects
      put pMessage & "," & tLine into tSubscription
      repeat for each line tTarget in sSubscriptions[tSubscription]["callback targets"]
         if tSubscriptionList is empty then
            put tTarget into tSubscriptionList
         else
            put return & tTarget after tSubscriptionList
         end if
      end repeat
   end repeat
   
   repeat for each line tObjectID in tSubscriptionList
      # if the callback object doesn't exist, don't send message
      if not exists(tObjectID) then
         revIDEUnsubscribeAll tObjectID
         next repeat
      end if
      
      # Send the message!
      set the itemdel to ":"
      local tMessage
      put item 1 of pMessage into tMessage
      
      # Construct a dispatch command with all the desired parameters
      local tDoString, tParams, tErrors
      put "dispatch tMessage to tObjectID" into tDoString
      
      # If this is a parameterised message, add the parameter as the first arg
      if the number of items of pMessage > 1 then
         put " with item 2 of pMessage" into tParams
      end if
      
      repeat with tParam = 1 to the number of elements in pParamsArray
         if tParams is empty then
            put " with pParamsArray[" & tParam & "]" into tParams
         else
            put ", pParamsArray[" & tParam & "]" after tParams
         end if
      end repeat
      put tParams after tDoString
      
      # Ensure any error doesn't prevent subsequent subscribers from receiving messages
      try
         do tDoString
      catch tError
         if tErrors is empty then
            put tError into tErrors
         else
            put return & tError after tErrors
         end if
      end try
   end repeat
   unlock screen
   
   if tErrors is not empty then
      return __revIDEError("Error when sending message" && pMessage & ":" & return & tErrors)
   end if
   
   return empty
end __PublishWithParameters

private command __Publish pMessage, pTriggeringObjects, pParam
   local tParam
   if pParam is not empty then
      put pParam into tParam[1]
   end if
   __PublishWithParameters pMessage, the long id of the target, tParam
end __Publish

----------------------------------------------------------------------------------
-- Public API

/**
Subscribes the calling object to receive the specified IDE message

pMessage (string): The message to subscribe to.

pObjectList (optional string): 
A list of objects, one per line, determining which calling objects the message should be restricted to.

Example:
on preOpenStack
   pusubSubscribe "ideSelectedObjectsChanged"
end preOpenStack

on ideSelectedObjectsChanged
   # Handle the notification
end ideSelectedObjectsChanged

Example:
-- When the selection changes, subscribe to the idePropertyChanged message for the newly selected objects
on ideSelectedObjectsChanged
   pubsubSubscribe "idePropertyChanged", revIDESelectedObjects()
end ideSelectedObjectsChanged

Description:
Subscribe to messages to have a notification sent for particular IDE actions.

**/

command pubsubSubscribe pMessage, pObjectList
   __Subscribe pMessage, pObjectList
end pubsubSubscribe

/**
Unsubscribes the calling object from the given message

pMessage (enum): The message to unsubscribe from. 
pObject (String): The long ID of the object you wish to unsubscribe from the message. This is an optional parameter, if not specified, the calling object will be used.
pObjectList (optional string): A list of objects, one per line.
**/

command pubsubUnsubscribe pMessage, pObject, pObjectList
   __Unsubscribe pMessage, pObject, pObjectList
end pubsubUnsubscribe

/**
Unsubscribes the object from all messages

pObject (String): The long ID of the object you wish to unsubscribe from all message. This is an optional parameter, if not specified, the calling object will be used.
**/
on pubsubUnsubscribeAll pObject
   __UnsubscribeAll pObject
end pubsubUnsubscribeAll


/**
Use to find out which messages the calling object is subscribed to.

returns (string): Return delimited list of messages, 1 per line.
**/
function pubsubSubscriptions
   return __Subscriptions()
end pubsubSubscriptions

/**
Use to find out which objects are subscribed to a given message

pMessage (String): The name of the message.

returns (string): Return delimited list of object ID's, 1 per line.
**/
function pubsubSubscribedObjects pMessage
   return __SubscribedObjects(pMessage)
end pubsubSubscribedObjects

/**
Sends the given message to all subscribed objects.

Parameters:
pMessage (string): The message to send

pTriggeringObjects (optional string):
A list, one per line, of the long ids of the objects triggering the message.
This is used for example with the <idePropertyChanged> message to
convey which objects had properties changed.

pParamsArray (array): 
A numerically keyed aray (starting at 1) of the parameters to pass along with the message.

Description:
Sends <pMessage> to all objects which have subscribed to it using
<revIDESubscribe>.

References: ideSubscribe (command), idePropertyChanged (message)
**/

command pubsubPublishWithParameters pMessage, pTriggeringObjects, pParamsArray
   __PublishWithParameters pMessage, pTriggeringObjects, pParamsArray
end pubsubPublishWithParameters

command commandPublish pMessage, pParam
   __Publish pMessage, the long id of the target, pParam
end commandPublish

command commandPublishWithTrigger pMessage, pTriggeringObjects, pParam
   __Publish pMessage, pTriggeringObjects, pParam
end commandPublishWithTrigger

----------------------------------------------------------------------------------
-- IDE Legacy shim

command ideSubscribe pMessage, pObjectList
   __Subscribe pMessage, pObjectList
end ideSubscribe

command ideMessageSendWithTrigger pMessage, pTriggeringObjects, pParam
   __Publish pMessage, pTriggeringObjects, pParam
end ideMessageSendWithTrigger

command ideMessageSend pMessage, pParam
   __Publish pMessage, the long id of the target, pParam
end ideMessageSend

on ideMessageSendWithParameters pMessage, pTriggeringObjects, pParamsArray
   __PublishWithParameters
end ideMessageSendWithParameters

command revIDEMessageSend pMessage, pTriggeringObjects
   local tParamsArray
   put pTriggeringObjects into tParamsArray[1]
   __Publish pMessage, pTriggeringObjects, tParamsArray
end revIDEMessageSend

command ideSubscribe pMessage, pObjectList
   __Subscribe pMessage, pObjectList
end ideSubscribe

command revIDESubscribe pMessage, pObjectList
   __Subscribe pMessage, pObjectList
end revIDESubscribe

command revIDEUnsubscribe pMessage, pObject, pObjectList
   __Unsubscribe pMessage, pObject, pObjectList
end revIDEUnsubscribe

command revIDEUnsubscribeAll pObject
   __UnsubscribeAll pObject
end revIDEUnsubscribeAll

function revIDESubscriptions
   __Subscriptions
end revIDESubscriptions

function revIDESubscribedObjects pMessage
   return __SubscribedObjects(pMessage)
end revIDESubscribedObjects
