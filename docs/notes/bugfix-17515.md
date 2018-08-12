# Add support for custom entitlements for iOS

Custom entitlements can now be added to an iOS app by including one or more 
`.xcent` files in the copy files section of the standalone builder containing an
XML snippet of key/value pairs. For example, if you wanted to add the 
entitlement for HomeKit to your app you might create a file named 
`HomeKit.xcent` with the following content:

````
<key>com.apple.developer.homekit</key>
<true/>
````
