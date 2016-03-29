# Overview

LiveCode 8.0 brings important new capabilities to all LiveCode developers:

* Use new custom controls and libraries in your applications, including a new browser widget.

* Extend LiveCode with the new LiveCode Builder language

* Deploy to HTML5 and run your application in a web browser

* Many other improvements!

## Simplify design with widgets

With LiveCode 8.0, your apps are set free from using the small range of user interface controls that were previously available in LiveCode.  The LiveCode engine now lets you load custom controls, called widgets, and use them in your apps just like any other control.

LiveCode comes with a selection of widgets that simplify creating many commonly-needed sets of controls in mobile apps, and if they aren't enough, you can download and install widgets created by members of the LiveCode development community and third-party vendors.

One of the most exciting new widgets introduced in LiveCode 8.0 is the browser widget.  It replaces the revBrowser external with a much more powerful and flexible browser control that's easier to use and more reliable.

The LiveCode IDE has also been extended and revised in order to support widgets and other extensions.  Widgets are now available in the "Tools" palette, and installed extensions can be viewed in the "Extension Manager".  The dictionary has been extended to include extension documentation.

## Extend Livecode with LiveCode Builder

In LiveCode 8.0, the well-known LiveCode scripting language is joined by a brand new programming language called LiveCode Builder.  LiveCode Builder looks a lot like LiveCode script, and should be easy to learn for any experienced LiveCode developer.

Using LiveCode Builder, it is now possible to extend LiveCode with new controls and libraries without any need to program in C or C++.  The IDE has a new "Extension Builder" tool that helps developers test, debug and package their extensions.

For more information, please refer to the "Extending LiveCode" guide and the "LiveCode Builder" section of the dictionary.

**Note:** LiveCode Builder is a new and experimental language.  There is no stability guarantee for the language or its standard libraries.  Be aware that the language syntax or features may change incompatibly in future versions of LiveCode!

## Deploy to the browser with HTML5

The LiveCode 8.0 engine now runs on a new platform: the web browser.  The LiveCode engine now runs as a JavaScript library in an HTML page, allowing users to run your application without having to install anything.

For more information, please refer to the "HTML5 Deployment" guide.

**Note:** The HTML5 platform is very different to the other platforms that LiveCode supports, and many engine features are either unsupported or work differently.

## More!

LiveCode 8.0 includes many other enhancements, including:

* more powerful and complete clipboard access, sponsored by [FMProMigrator](https://www.fmpromigrator.com)

* 64-bit Mac standalone deployment, SSL support for PostgreSQL
  connections, and "find and replace" that preserves text style, all
  sponsored by the community Feature Exchange

* optimised Unicode text processing

* Unicode printing on Linux

* a new JSON library extension

* greatly improved native theming on desktop platforms

* a new IDE Start Center and interactive tutorial
