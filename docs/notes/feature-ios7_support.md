# iOS 7.0/Xcode 5.0 Support

Support has been added to the engine and IDE to allow for iOS builds using Xcode 5.0. This means that OS 10.8 users (required by Xcode 5.0)  will need to have Xcode 5.0 installed and set up in LiveCode's preferences in order to produce Arm v7 builds. Arm v6 builds will still be produced using Xcode 4.4 (the last version to support Arm v6).

The table below details the versions of Xcode LiveCode requires on each platfrom to produce the given build type. In order to produce universal builds both the Arm v6 and Arm v7 Xcode SDKs will be required.


|Platform|Arm v6|Arm v7|
-
|10.6|Xcode 4.2 (iOS 5.0)|Xcode 4.2 (iOS 5.0)|
|10.7|Xcode 4.3 (iOS 5.1)|Xcode 4.6 (iOS 6.1)|
|10.8|Xcode 4.3 (iOS 5.1)|Xcode 5.0 (iOS 7.0)|


In addition to the above, the new iOS 7 icon sizes can be specified in the standalone builder. They are sized as follows:

* Retina iPhone: 120x120
* iPad: 76x76
* Retina iPad: 152x152
