# iOS Remote Control Support
Support has been added allowing access to the 'remote control' feature on iOS. This feature allows access to the audio controls both on an external device, and also on the device itself when in lock screen (via double-tapping on the Home button).

To use the new features, remote control access must be first enabled (**iphoneEnableRemoteControl**) and then the app must play audio. You also need to ensure that the audio category is set to 'playback' (**iphoneSetAudioCategory "playback"**).

The remote controls will be delivered via an **remoteControlReceived** message.

The information to display can be configured using **iphoneSetRemoteControlDisplay**.

## Enabling and Disabling Remote Control

	**iphoneEnableRemoteControl**
	**iphoneDisableRemoteControl**
	**iphoneRemoteControlEnabled()**

Use the command **iphoneEnableRemoteControl** to start receiving remote control events after you start playing audio and the command **iphoneDisableRemoteControl** to stop receiving them. The **iphoneRemoteControlEnabled()** function returns true if they are currently enabled.

## Receiving Remote Control Events

If remote control is enabled, and a remote control operation occurs then a **remoteControlReceived** message will be sent to the current card.

	**remoteControlReceived** *type*

Here *type* is one of: *play*, *pause*, *stop*, *toggle play pause*, *next track*, *previous track*, *begin seeking forward*, *begin seeking backward*, *end seeking forward*, *end seeking backward*.

## Configuring Remote Control Display

When remote controls are enabled, information can be set that may be displayed on the remote control device, or on the (locked) home screen controls.

To configure this information use the **iphoneSetRemoteControlDisplay** command:

	**iphoneSetRemoteControlDisplay** *metadata*

Here, *metadata* is an array with one or more of the following keys:

*title - string
*artist - string
*artwork - either the text of an image or an image filename
*composer - string
*genre - string
*album title - string
*album track count - number
*album track number - number
*disc count - number
*disc number - number
*chapter count - number
*chapter number - number
*playback duration - number
*elapsed playback time - number
*playback rate - number
*playback queue index - number
*playback queue count - number

**Note**: There is no guarantee that any of this information will be used, it is up to the remote control device / iOS to decide.

**Note**: This functionality is only available on iOS 5.x and above.
