# Compiling LiveCode to JavaScript for HTML5

![LiveCode Community Logo](http://livecode.com/wp-content/uploads/2015/02/livecode-logo.png)

Copyright © 2015 LiveCode Ltd., Edinburgh, UK

**Warning**: Emscripten (HTML5) platform support for LiveCode is still under development.  This document is almost certainly already out of date.

## Dependencies

You will need a 64-bit Linux machine or VM with at least 4 GB of RAM
(8 GB is recommended).

### Emscripten SDK

Unsurprisingly, the Emscripten SDK must be installed in order to build
an Emscripten engine.

1. Download the portable Emscripten SDK from <https://kripken.github.io/emscripten/site>.  Put it in `/opt/emsdk_portable`, for example.

2. Check what SDKs are available by running:

       /opt/emsdk_portable/emsdk list

3. Install and activate the "incoming" SDK by running:

       /opt/emsdk_portable/emsdk install sdk-incoming-64bit
       /opt/emsdk_portable/emsdk activate sdk-incoming-64bit

   This will take a really long time and use an insane amount of RAM.

### Prebuild libicu

The LiveCode downloads server doesn't currently include prebuilt libraries for Emscripten.  You need to build **libicu** manually.  First you need to build a host library, and then use the host library to cross compile **libicu** for Emscripten.

    cd prebuilt
    ./build-libraries.sh linux x86_64 icu

    ( source /opt/emsdk_portable/emsdk_env.sh &&
      ./build-libraries.sh emscripten js icu )

## Build environment

Before building for Emscripten, source the Emscripten SDK script that sets up the environment correctly.  You need to source it with the `.` or `source` command rather than just running it.

    source /opt/emsdk_protable/emsdk_env.sh

## Configuring LiveCode

To configure LiveCode, run:

    make config-emscripten

This will generate make control files in the `build-emscripten` directory.  You can also run `config.sh` directly.

## Compiling LiveCode

LiveCode currently requires some special setup steps to enable building on Emscripten.

You need to construct a root filesystem for the standalone engine in the `engine/boot` directory.  This is a temporary setup step until a standalone builder is implemented for Emscripten engines. You need to provide two files there:

* `engine/boot/boot.livecode` is a stack to be loaded when the standalone engine starts.

* `engine/boot/fonts/basefont.ttf` is the default (currently only) font for text rendering

To compile LiveCode, run:

    make compile-emscripten

## Running LiveCode

Emscripten builds currently don't put all of the output files in the `emscripten-js-bin` directory.  To run the engine, load `build-emscripten/livecode/out/Debug/standalone-community.html` in a web browser.

Some web browsers (including Google Chrome) have JavaScript security policies that won't allow you to run the engine from a local filesystem.  For these browsers, you will need to run a local web server.  You can use the following steps to launch a local-only webserver listening on port 8080:

    cd build-emscripten/livecode/out
    python -m SimpleHTTPServer 8080
