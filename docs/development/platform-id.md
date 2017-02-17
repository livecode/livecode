# Platform Identification Triples

LiveCode is built for and supports multiple different platforms with a variety of
architecture and toolchain/sdk versions. In order to identify specific builds
uniquely a standard triple format is used:
```
    <architecture>-<platform>[-<options>]
```
For example, the triple describing 32-bit linux would be x86-linux; and the
triple describing 64-bit macOS built against the 10.9 SDK would be
x86_64-mac-macos10.9

*Note:* All triples should be written, generated and manipulated as lowercase as
they will be used as parts of names of files and folders on disk on platforms
with case-sensitive filenames.

The specific architectures, platforms and options that are supported are
outlined in this document.

## Architectures

A universal set of names for architectures are used, however only a subset are
valid for any specific platform/options combination and for each platform there
is a mapping to a specific ABI/processor configuration for it.

The following architecture identifiers are available:

- x86
- x86_64
- armv6
- armv7
- arm64
- universal
- js

The x86 architecture is currently supported for linux, mac, win32 and
ios-iphonesimulator. It maps to the default 32-bit x86 ABI and processor
configuration for the target.

The x86_64 architecture is currently supported for linux, mac and
ios-iphonesimulator targets. It maps to the default 64-bit x86-64 ABI and
processor configuration for the target.

The armv6 architecture is currently supported for android. It maps to
the (linux) armeabi ABI and processor configuration.

The armv7 architecture is currently supported for android and ios-iphoneos. It
maps to the (linux) armeabi_v7a ABI and processor configuration on android, and
to the armv7 ABI and processor configuration on ios-iphoneos.

The armv64 architecture is currently supported for ios-iphoneos. It maps to
the arm64 ABI and processor configuration.

The universal architecture is currently supported for mac and ios. It describes
'fat' builds containing multiple architecture slices. It maps as follows:

- mac: x86 and x86_64
- ios-iphonesimulator: x86 and x86_64
- ios-iphoneos: armv7 and arm64

The js architecture is only supported for emscripten.

## Platforms

The following platform identifiers are available:

- linux
- mac
- win32
- android
- ios
- emscripten

## Options

For some platforms the architecture and platform pair is not enough to
describe the specific target a native code component has been built for.
In this case a third options section will be present. The options available are
specific to a particular platform.

### Windows (win32)

Components built using different versions of msvc or different crt modes are
incompatible and thus must be explictly expressed.

The options section has the form:
```
    msvc<X>_<Y>
```
Where X is the 100 * the compiler version, and Y is the CRT mode which is one of:

- mtd: built against the static debug CRT
- mt: built against the static release CRT
- mdd: built against the dynamic debug CRT
- md: built against the dynamic release CRT

For example, a build using the X86 VC2015 compiler for static release CRT would be
`x86-win32-msvc140_mt`.

### Mac (mac)

Components built using different SDK versions are incompatible and thus must
be explicitly expressed.

The options section is the lowercase version of the SDK identifier used to build
the component:
```
  macosx<N>.<M> (for SDK versions < 10.12)
  macos<N>.<M> (for SDK versions >= 10.12)
```
Where N is the major version of the SDK and M is the minor version of the SDK.

For example, a build using the x86 compiler in the 10.9 SDK would be
`x86-mac-macosx10.9`.

### iOS (ios)

Components built using different iOS SDKs are incompatible and thus must be
explicitly expressed. Indeed, the difference between the iOS simulator builds
and iOS device builds are expressed in terms of the SDK.

The options section is the lowercase version of the SDK identifier used to
build the component:
```
   iphonesimulator<N>.<M>
   iphoneos<N>.<M>
```
Where N is the major version of the SDK and M is the minor version of the SDK.

For example, a universal build for the iOS simulator against the 8.3 SDK would
be `universal-ios-iphonesimulator8.3`.
