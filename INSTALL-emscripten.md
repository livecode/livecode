# Compiling LiveCode to JavaScript for HTML5

![LiveCode Community Logo](http://livecode.com/wp-content/uploads/2015/02/livecode-logo.png)

Copyright © 2015 LiveCode Ltd., Edinburgh, UK

**Warning**: Emscripten (HTML5) platform support for LiveCode is experimental and not recommended for production use.

## Dependencies

You will need a 64-bit Linux machine or VM with at least 4 GB of RAM
(8 GB is recommended).

### Emscripten SDK

Unsurprisingly, the Emscripten SDK must be installed in order to build
an Emscripten engine.

1. Download the portable Emscripten SDK from <https://kripken.github.io/emscripten-site>.  Put it in `/opt/emsdk_portable`, for example.

2. Check which SDKs are available by running:

       /opt/emsdk_portable/emsdk list

3. Install and activate the "incoming" SDK by running:

       /opt/emsdk_portable/emsdk install sdk-incoming-64bit
       /opt/emsdk_portable/emsdk activate sdk-incoming-64bit

   This will take a really long time and use an insane amount of RAM.

## Build environment

Before building for Emscripten, source the Emscripten SDK script that sets up the environment correctly.  You need to source it with the `.` or `source` command rather than just running it.

    source /opt/emsdk_portable/emsdk_env.sh

## Configuring LiveCode

To configure LiveCode, run:

    make config-emscripten

This will generate make control files in the `build-emscripten` directory.  You can also run `config.sh` directly.

## Compiling LiveCode

To compile LiveCode, run:

    make compile-emscripten

This will generate outputs in the `emscripten-bin` directory.

## Running LiveCode

**Note**: See also the "HTML5 Deployment" guide, available in the in-IDE dictionary.

Use the desktop build of the LiveCode IDE to run the standalone builder and create an "HTML5" standalone.

Once you've created a standalone, you can open the HTML file in a web browser to try out the engine.

Some web browsers (including Google Chrome) have JavaScript security policies that won't allow you to run the engine from a local filesystem.  For these browsers, you will need to run a local web server.  You can use the following steps to launch a local-only webserver listening on port 8080:

    cd /path/to/my/standalone
    python -m SimpleHTTPServer 8080

You can then load http://localhost:8080/ in a web browser to view your standalone HTML5 engine.

