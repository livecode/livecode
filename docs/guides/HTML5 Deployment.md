---
group: deployment
---

# HTML5 Deployment

## Introduction

**Note: This is BETA release of HTML5 deployment support. If you intend to use it in production, please test thoroughly that it supports all the functionality you are looking for.**

Almost every Internet-connected device has a web browser.  If your application can run in a browser, your app can be used anywhere and by anyone, without any need to download or install it.

With LiveCode 8's HTML5 deployment capability, you can now run applications written in LiveCode in any web browser that supports JavaScript and HTML5.

## Supported browsers

Only a limited range of browsers are supported for HTML5 deployment in this release of LiveCode.

* [Mozilla Firefox](https://www.mozilla.org/firefox/new/) 40.0 (or newer)
* [Google Chrome](https://www.google.com/chrome/) 44 (or newer)
* [Safari for Mac](https://support.apple.com/HT204416) (latest version)

We hope to broaden the range of supported browsers in the future.

## HTML5 engine features

The HTML5 engine in this release of LiveCode has a limited range of features.  You can:

* deploy single-stack applications with embedded resources
* use most of the engine's built-in controls and graphics capabilities.
* read and write temporary files in a special virtual filesystem (which is erased when the user navigates away from the page)
* use LiveCode Builder widgets and extensions
* interact with JavaScript code in the web page using `do <script> as "JavaScript"`
* perform basic networking operations using the **load** command

Several important features are not yet supported:

* some `ask` and `answer` message boxes
* multimedia (the "player" control)
* JavaScript in LiveCode Builder extensions

Two important unsupported features are unlikely to be added in the near future:

* operations that need to pause the script while something happens
  (e.g. `wait 10`)
* externals (including revdb)

## How to deploy an app to HTML5

### Step by step

Deploying an app to an HTML5 standalone is straightforward:

1) Open your stack in the LiveCode IDE

2) Select **File → Standalone Application Settings...** from the menu bar

3) Browse to the **HTML5** tab of the standalone settings window

4) Make sure that the **Build for HTML5** checkbox is enabled

5) Close the standalone settings window

6) Select **File → Save as Standalone Application...** from the menu bar

Your application will be packaged up and placed in the selected output folder.

### Contents of the HTML5 standalone

The HTML5 standalone contains four files:

* A standalone archive, named `standalone.zip` by default.  This file contains your application and all of the resources that it depends on.  When the engine runs, the filesystem that's visible to the engine (e.g. via the `open file` syntax) is based on the contents of the standalone archive.

* The engine itself, which consists of two files.  The `.js` file contains the engine's executable code, and the `.html.mem` file contains essential data that's needed for the engine to run.  These files are always the same, and only change when LiveCode is upgraded.

* A test HTML page.  This can be opened in a browser and will correctly prepare, download and start your HTML5 app in a convenient test environment.

### Testing your HTML5 app with a local web server

Some browsers, such as Google Chrome, do not permit pages to download resources from `file://` URLs.  You won't be able to test your application in these browsers unless you run a local HTTP server.

A quick and easy way to run a simple local HTTP server is to use Python.  Open a terminal window, change directory to your standalone's directory, and run:

    python -m SimpleHTTPServer 8080

This will let you access your standalone by opening your web browser and visiting <http://localhost:8080>.

## Reporting bugs

Please report bugs to the [LiveCode Quality Centre](http://quality.livecode.com/).  Make sure to select "HTML5 Standalone" when you're creating your bug report!

## Advanced: HTML5 standalone filesystem

JavaScript applications running in a browser don't have access to the host system's filesystem.  Instead, the filesystem-related features of LiveCode, such as `open file`, use a virtual filesystem (VFS) that exists only in memory.  This filesystem is initialised before the engine starts, and is reset and its content discarded when the engine stops (when the user closes the browser view or navigates to a different page).

During engine startup, the VFS is populated from the contents of the `standalone.zip` file that's created by the HTML5 deployment process.  All of the initial files are stored in `/boot/` in the VFS.

There are several special files & directories in the `/boot/` directory:

* `/boot/__startup.livecode`: a stack that performs loading & initialisation operations during engine startup.
* `/boot/auxiliary_stackfiles/`: during startup, each file in this directory is loaded and is receives the `revLoadLibrary` message
* `/boot/fonts/basefont.ttf`: the font used by the engine
* `/boot/standalone/__boot.livecode`: this is the initial stack of the application, which is the main stack that receives the `startup` message when the engine has been initialized
* `/boot/standalone/`: the `defaultFolder` when the engine starts, and the location where additional assets selected using the "Copy files" page of the standalone builder are placed
* `/boot/extensions/extensions.txt`: list of extensions included in the standalone, in the order in which they should be autoloaded
* `/boot/extensions/`: the directory where all autoloaded extensions are stored

In general, if you wish to add new files or directories to the `standalone.zip` archive, it is best to add them outside the `/boot/` directory tree.

## Advanced: Embedding an HTML5 standalone in a web page

The default HTML5 page provided by the HTML5 standalone builder is designed for testing and debugging purposes.  However, you may want to embed the standalone engine in a more visually appealing page.  To do this, you require three elements: 1) a canvas, 2) a JavaScript `Module` object, and 3) an HTML `<script>` element that downloads the engine.

### The canvas

The engine renders into a HTML5 `<canvas>` element.  There are three important considerations when creating the canvas:

* the canvas must have absolutely no border, or mouse coordinate calculations will be incorrect

* it will be automatically resized by the engine to match the size of your stack, so don't attempt to set its size using HTML or CSS

* it needs to be easily uniquely identifiable, so that the engine can find it.

The absolute minimum canvas element would look something like this:

    <canvas style="border: 0px none;" id="canvas" oncontextmenu="event.preventDefault();"></canvas>

By default, most web browsers will indicate when the canvas has focus by displaying a highlighted outline.  This helps users identify which part of the web page is capturing their key presses.  You can usually disable this outline by adding `outline: none;` to the canvas's CSS styles.

### The Module object

The top-level JavaScript `Module` object contains the parameters that control how the engine runs.  At minimum, you need only specify the `Module.canvas`, which should be your canvas element.

The absolute minimum `Module` object declaration would look something like:

    <script type="text/javascript">
    var Module = {
      canvas: document.getElementById('canvas'),
    };
    </script>

### Engine download

The engine is quite a large JavaScript file, so it's downloaded asynchronously in order to let the rest of the page finish loading and start being displayed.

Quite straightforwardly:

    <script async type="text/javascript" src="standalone-<version>.js"></script>

Make sure to replace `<version>` as appropriate.

### Bringing it all together

Here's the complete skeleton web page for an HTML5 standalone:

    <html>
       <body>
        <canvas style="border: 0px none;" id="canvas" oncontextmenu="event.preventDefault()"></canvas>

         <script type="text/javascript">
           var Module = { canvas: document.getElementById('canvas')  };
         </script>
        <script async type="text/javascript" src="standalone-community.js"></script>
      </body>
    </html>

## Advanced: Speeding up engine download

Currently, the engine files are almost 30 MB, which is a lot to download before the engine can start.  It is possible to speed up the download by enabling deflate compression in the web server configuration.

Enabling deflate compression reduces the total download size to around 6.3 MB.  It's recommended to pre-compress the engine with `gzip`, and then configure your web server to serve the pre-compressed files.

* For the Apache web server, configure `mod_deflate` to serve [pre-compressed content](https://httpd.apache.org/docs/2.4/mod/mod_deflate.html#precompressed)
* For the NGINX web server, add [`gzip_static on;`](https://www.nginx.com/resources/admin-guide/compression-and-decompression/#send) to your configuration.

## Advanced: Customizing the Module object

There are a number of LiveCode-specific `Module` attributes that you can modify to affect how the engine behaves:

* `Module.livecodeStandalone`: the filename of the standalone archive (default `standalone.zip`)
* `Module.livecodeStandalonePrefixURL`: Prepended to the standalone archive filename to construct its full URL (default empty)
* `Module.livecodeStandaloneRequest`: If you assign a network request to this attribute (before the engine runs), then it will use that request for the standalone archive instead of automatically starting a download for you. This means that you can, in your HTML, fire off a request for the standalone before the engine script actually arrives.  For this to work, the network request should be an `XMLHttpRequest` with its `responseType` set to `arraybuffer`.

See also Emscripten's [Module object documentation](https://kripken.github.io/emscripten-site/docs/api_reference/module.html).
