---
group: deployment
---

# Mac Deployment

## Introduction

LiveCode 8.0 DP15 introduced the ability to build 64-bit standalones for 
MacOSX after a successful sponsorship campaign on the LiveCode Feature 
Exchange.

## 64-bit engine features

The 64-bit Mac engines should work exactly the same as the 32-bit 
engines, with the following exceptions:

* QuickTime is not available as it has been deprecated by Apple
* Sound recording is not available as it depends on QuickTime
* The revvideograbber external is not available as it depends on QuickTime

Some other features may differ in minor ways not affecting 
functionality, for example:

* Printing progress dialogs are not shown as the necessary API was removed by Apple

## The IDE

The LiveCode IDE runs in 32-bit mode by default for maximum 
compatibility. If you'd like to run it in 64-bit mode, right-click on 
the app bundle, select "Get Info" and deselect the "Open in 32-bit mode" 
option. In future, we will add a menu item in the IDE that re-launches 
the IDE in either 32-bit or 64-bit mode, as appropriate.

## How to deploy a 64-bit Mac standalone

### Building

The MacOS pane of the Standalone Settings includes two checkboxes to 
indicate what engines you'd like to include in the standalone. The 
current default is 32-bit only but this can be changed to 64-bit only or 
32-bit/64-bit Universal binary. If both engines are included, only one 
app bundle is produced containing both engines. The app will then run in 
64-bit mode on 64-bit machines or 32-bit mode otherwise.

### Testing

If you have built a 32-bit only or 64-bit only standalone, the app will 
always run in that mode. If you have a Universal standalone containing 
both, you can force the app to run in 32-bit mode on 64-bit machines by 
right-clicking the app bundle, selecting "Get Info" and then enabling 
the "Open in 32-bit mode" option.

## Reporting bugs

Please report bugs to the 
[LiveCode Quality Centre](http://quality.livecode.com/).  Make sure you 
select the correct engine type when prompted - 32-bit or 64-bit.