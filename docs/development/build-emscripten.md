# Compiling LiveCode to JavaScript for HTML5

![LiveCode Community Logo](http://livecode.com/wp-content/uploads/2015/02/livecode-logo.png)

Copyright © 2020 LiveCode Ltd., Edinburgh, UK

**Warning**: Emscripten (HTML5) platform support for LiveCode is experimental and not recommended for production use.

## Dependencies

You will need a 64-bit Linux machine or VM with at least 4 GB of RAM
(8 GB is recommended).

## Emscripten build architectures

There are two emscripten architectures that can be targeted for building the Emscripten engine, asm.js and WebAssembly. asm.js is based on a subset of JavaScript, whereas WebAssembly produces binary byte-code.

## Building for asm.js

The emscripten-js build target produces output in asm.js format that is converted to bytecode in order to be run through an interpreter. The emterpreted code can then be halted and resumed. This method of producing interruptable code is now deprecated but the tools are still available from the Emscripten SDK.

### Emscripten SDK

1. Download the Emscripten SDK. The recommended method for obtaining the Emscripten SDK is to use git tools to clone the emsdk repository from github. Other methods are described in <https://emscripten.org/docs/getting_started/downloads.html>. Put it in `/opt/emsdk_js`, for example.

       `git clone https://github.com/emscripten-core/emsdk /opt/emsdk_js`

2. Check that the required tools are available by running:

       `/opt/emsdk_js/emsdk list --old`

3. Install and activate SDK 1.35.23 by running:

       `/opt/emsdk_js/emsdk install node-12.9.1-64bit`
       `/opt/emsdk_js/emsdk activate --embedded node-12.9.1-64bit`
       `/opt/emsdk_js/emsdk install emscripten-tag-1.35.23-64bit`
       `/opt/emsdk_js/emsdk activate --embedded emscripten-tag-1.35.23-64bit`
       `/opt/emsdk_js/emsdk install fastcomp-clang-tag-e1.35.23-64bit`
       `/opt/emsdk_js/emsdk activate --embedded fastcomp-clang-tag-e1.35.23-64bit`

   This will take a really long time and use an insane amount of RAM.

### Build environment

Before building for Emscripten, source the Emscripten SDK script that sets up the environment correctly.  You need to source it with the `.` or `source` command rather than just running it.

    `source /opt/emsdk_js/emsdk_env.sh`

### Configuring LiveCode

To configure LiveCode, run:

    `make config-emscripten-js`

This will generate make control files in the `build-emscripten-js` directory.  You can also run `config.sh` directly.

### Compiling LiveCode

To compile LiveCode, run:

    `make compile-emscripten-js`

This will generate outputs in the `emscripten-bin-js` directory.

## Building for WebAssembly

The emscripten-wasm build target produces output in WebAssembly format that is then processed to allow the call stack to be saved and restored, making it possible for the engine execution to be halted and resumed. This is now the preferred method of generating interruptable code and can be built with the most recent version of the Emscripten SDK tools (version 1.39.12 as of April 2020).

### Emscripten SDK

1. Download the Emscripten SDK as described above. Put it in `/opt/emsdk_wasm`, for example.

       `git clone https://github.com/emscripten-core/emsdk /opt/emsdk_wasm`

2. Check that the required tools are available by running:

       `/opt/emsdk_wasm/emsdk list`

3. Install and activate SDK 1.39.12 by running:

       `/opt/emsdk_wasm/emsdk install 1.39.12`
       `/opt/emsdk_wasm/emsdk activate --embedded 1.39.12`

### Build environment

Before building for Emscripten, source the Emscripten SDK script that sets up the environment correctly.  You need to source it with the `.` or `source` command rather than just running it.

    `source /opt/emsdk_wasm/emsdk_env.sh`

### Configuring LiveCode

To configure LiveCode, run:

    `make config-emscripten-wasm`

This will generate make control files in the `build-emscripten-wasm` directory.  You can also run `config.sh` directly.

### Compiling LiveCode

To compile LiveCode, run:

    `make compile-emscripten-wasm`

This will generate outputs in the `emscripten-bin-wasm` directory.

## Running LiveCode

**Note**: See also the "HTML5 Deployment" guide, available in the in-IDE dictionary.

A desktop IDE built and launched from the repository checkout folder can be used to run HTML5 standalones with the emscripten engines compiled using the steps above.

1. Enable emscripten output in the standalone settings for your stack. You can choose which architectures to include in the standalone output folder.

2. Select a browser from the "Development -> Test Target" menu

3. Click the "Test" button on the IDE toolbar.

Your stack will be compiled to a HTML5 standalone and launched in the selected browser. If "WebAssembly" output is enabled in the standalone settings of the stack then the WebAssembly engine will be used when compiling the test app, otherwise the asm.js engine will be used.

## Manual testing of HTML5 standalones. 

Use the desktop build of the LiveCode IDE to run the standalone builder and create an "HTML5" standalone.

Once you've created a standalone, you can open the HTML file in a web browser to try out the engine.

Some web browsers (including Google Chrome) have JavaScript security policies that won't allow you to run the engine from a local filesystem.  For these browsers, you will need to run a local web server.  You can use the following steps to launch a local-only webserver listening on port 8080:

    `cd /path/to/my/standalone`
    `python -m SimpleHTTPServer 8080`

You can then load http://localhost:8080/ in a web browser to view your standalone HTML5 engine.
