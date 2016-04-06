# Compiling LiveCode for Mac OS X and iOS

![LiveCode Community Logo](http://livecode.com/wp-content/uploads/2015/02/livecode-logo.png)

Copyright © 2015 LiveCode Ltd., Edinburgh, UK

## Dependencies

### Required dependencies

You must install Xcode.  This will allow you to build LiveCode for:

* OS X desktop systems
* iPhone OS
* iPhoneSimulator

You will not be able to compile the "revvideograbber" extension.

### Optional dependencies

By default, LiveCode is compiled for a large number of versions of iPhoneSimulator, and requires quite a lot of Apple SDKs to be installed.

Create a directory on your hard disk (say, `/Applications/Xcode-Dev/`).

Download and install each of the following versions of Xcode, placing their app bundles into the specified paths:

  | Xcode version | App path |
  | ------------- | -------- |
  | 6.3.1         | /Applications/Xcode-Dev/Xcode_6_3_1.app |
  | 6.3           | /Applications/Xcode-Dev/Xcode_6_3.app |
  | 6.2           | /Applications/Xcode-Dev/Xcode_6_2.app |
  | 5.1.1         | /Applications/Xcode-Dev/Xcode_5_1_1.app |
  | 4.3.3         | /Applications/Xcode-Dev/Xcode_4_3_3.app |

Make sure you run and verify each of the versions of Xcode. Download and install any extra SDKs you need using the "Xcode → Preferences → Downloads" window.

Make `/Applications/Xcode-Dev/Xcode.app` a symlink to the latest version of Xcode available.  For example, run:

    cd /Applications/Xcode-Dev
    ln -s Xcode_6_3_1.app Xcode.app

After checking out the LiveCode git repository, you need to run a tool to finalize the Xcode setup and to make sure all of the necessary SDKs are installed.  If LiveCode is checked out to `~/git/livecode`, run:

    cd /Applications/Xcode-Dev/
    sh ~/git/livecode/tools/setup_xcode_sdks.sh

## Configuring LiveCode

### Build environment

If you have installed the `Xcode.app` to a non-standard location, or you wish to switch between multiple versions of Xcode, you will need to set the `XCODEBUILD` environment variable.  For example:

    export XCODEBUILD=/Applications/Xcode-Dev/Xcode.app

### Generating Xcode project files

To generate Xcode project files for OS X desktop builds, run:

    make config-mac

This will generate project files in the `build-mac` directory.  You can open and use these in Xcode.

To generate Xcode project files for iOS, run:

    make config-ios

This will generate several build directories with Xcode project files: one for each version of iPhoneOS or iPhoneSimulator.

If you want to just build for the newest supported version of the iPhoneOS SDK, you can simply run:

    make config-ios-iphoneos

To provide detailed configuration options, you can use the `config.sh` script.  For more information, run:

    ./config.sh --help

## Compiling LiveCode

You can open the generated project files in Xcode and compile from there using the normal Xcode build procedure.

You can also compile the engine from the command line using make, for example:

    make compile-mac

The same applies for the iPhoneOS and iPhoneSimulator builds.  For example, you can compile for the newest supported version of the iPhoneSimulator SDK using:

    make compile-ios-iphonesimulator
