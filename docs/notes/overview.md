# Overview
LiveCode 8.0 is the most exciting release in the history of the technology. It provides a simple way to extend the functionality or control set of LiveCode.

Our focus in LiveCode 8.0 is extensibility. You can now build and share widgets (custom controls) and libraries that are treated by LiveCode as engine level elements.

LiveCode 8.0 can be thought of as a version 7.0 with a new module allowing extensions to be plugged into the engine. As a result, 8.0 should be as functional and stable as LiveCode 7.0.

## I don’t want to build extensions. What’s in it for me?
Many love LiveCode because of the productivity benefits and don’t have time to build extensions. If that is the case just kick back and start using LiveCode 8 and keep an eye on the extensions portal. You can start using new controls and libraries as they are built by other community members.

## LiveCode Script vs LiveCode Builder
To make it possible to create extensions and plug them into the LiveCode engine we've created a new flavour of our language called ***LiveCode Builder***. LiveCode Builder looks a lot like LiveCode Script so should feel familiar for any seasoned LiveCode developer. There is lots of new syntax which exposes parts of the LiveCode engine that were only previously available to those who were skilled c/c++ developers.

LiveCode Builder is a new language and is therefore highly experimental and should be considered an early prototype. It will take some getting used to but we know you’ll love it once you see how powerful it is. The best way to get started is to read the "Extending LiveCode" guide which can be found in the dictionary under the "Guide" tab.

## Warning
It is important to stress that ***no aspect of this release should be considered final***. ***Every piece of syntax in LiveCode Builder is subject to change***.

## IDE
A number of palettes have been replaced and new ones added in order to support extensions. The tools palette, message box and property inspector have been rewritten. An extension manager stack has been added to keep track of installed extensions, and a plugin added to help users create their own extensions. For full details see the "Extending LiveCode" guide which can be found in the dictionary under the "Guide" tab.