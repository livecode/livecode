#     Location Services Disabled with LC 6.6.4 (rc1)

A new function **mobileLocationAuthorizationStatus** (or **iphoneLocationAuthorizationStatus**) has been added. This returns the current location authorization status of the calling application. The status can be one of the following:

- **notDetermined**: User has not yet made a choice with regards to this application
- **restricted**: The application is not authorized to use location service
- **denied**: User has explicitly denied authorization for this application, or location services are disabled in Settings.
- **authorizedAlways**: User has granted authorization to use their location at any time, including monitoring for regions, visits, or significant location changes.
- **authorizedWhenInUse**: User has granted authorization to use their location only when the app is visible to them (it will be made visible to them if you continue to receive location updates while in the background). Authorization to use launch APIs has not been granted.

We have also changed the flow of the messages being sent to the user when using Location Services in iOS 8:

- In the standalone application settings tab, the developer can choose the type of the authorization request for their app.
 The two available options are either "always" or "when in use". Selecting "always" means that the app will prompt the user to grant authorization to use their location
 at *any* time, including monitoring for regions, visits, or significant location changes. The app then has access to the user's location even when the app is in the
 background. On the contrary, if "when in use" is selected, the app will prompt the user to grant authorization to use their location only when the app is visible on screen. You can choose only one type, not both. This means that if you go to Settings -> Privacy -> Location, you will see only two choices available ("Never" and either "Always" or "While using the app") for this app, keeping it consistent with other iOS apps.

- When the app is installed (on device or simulator) for the very first time, a dialog will pop up asking the user to authorize the app to use their location
 "always" or "when in use", depending on what was previously chosen in the standalone application settings.

- Every time the app is launched, it remembers the user's preference. No other popup dialogs will appear.

- The user can at any time change their preferences in Settings -> Privacy -> Location -> ..  

- In that way, you need not modify your existing scripts that used Location Services, in order to add iOS 8 support.
