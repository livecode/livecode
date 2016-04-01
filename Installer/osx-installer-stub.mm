/* Copyright (C) 2016 LiveCode Ltd.
 
 This file is part of LiveCode.
 
 LiveCode is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License v3 as published by the Free
 Software Foundation.
 
 LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 for more details.
 
 You should have received a copy of the GNU General Public License
 along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */



#import <Foundation/Foundation.h>


// This file is a simple stub executable for launching the AppleScript that
// opens a Finder window showing the contents of the DMG. It is used by the
// auto-updater to display the contents of the DMG so the user can copy the new
// app bundle to their Applications folder.
//
// Previously, we used a shell script to do the launching. However, as of OSX
// 10.11.4, the `codesign` tool generates signatures that cannot be verified on
// earlier versions of OSX when asked to sign a non-MachO executable as code.
//
// Therefore, we use this executable in place of the shell script so that the
// object being signed is one that doesn't cause problems.


int main(int argc, char* argv[])
{
    // Get the main bundle for the application
    NSBundle* mainBundle = [NSBundle mainBundle];
    
    // Path to the subdirectory containing the bundle's resources
    NSString* resourcePath = [mainBundle resourcePath];
    
    // Append the path to the AppleScript that opens a Finder window
    NSString* scriptPath = [resourcePath stringByAppendingPathComponent:@"Installer/ShowDmgWindow.scpt"];
    
    // Turn the path into a URL and load it into an AppleScript object
    NSURL* scriptURL = [[NSURL alloc] initFileURLWithPath:scriptPath];
    NSAppleScript* appleScript = [[NSAppleScript alloc] initWithContentsOfURL:scriptURL error:nil];
    
    // Generate the "run" AppleEvent to send to the script. It needs to be given
    // an array containing 1 item which is the POSIX path to the DMG folder.
    NSString* dmgPath = [[mainBundle bundlePath] stringByDeletingLastPathComponent];
    NSAppleEventDescriptor* pathDescriptor = [NSAppleEventDescriptor descriptorWithString:dmgPath];
    NSAppleEventDescriptor* argvList = [NSAppleEventDescriptor listDescriptor];
    NSAppleEventDescriptor* appleEvent = [NSAppleEventDescriptor appleEventWithEventClass:kCoreEventClass
                                                                                  eventID:kAEOpenApplication
                                                                         targetDescriptor:nil
                                                                                 returnID:kAutoGenerateReturnID
                                                                            transactionID:kAnyTransactionID ];
    [argvList insertDescriptor:pathDescriptor atIndex:1];
    [appleEvent setParamDescriptor:argvList forKeyword:keyDirectObject];
    
    // Execute the script
    [appleScript executeAppleEvent:appleEvent error:nil];
    
    // Cleanup
    [appleEvent release];
    [argvList release];
    [pathDescriptor release];
    [dmgPath release];
    [appleScript release];
    [scriptURL release];
    [scriptPath release];
    [resourcePath release];
    [mainBundle release];
    
    // All done
    return 0;
}
