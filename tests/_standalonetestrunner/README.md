# Standalone Test Runner Builder

Prior to using the standalone test runner builder either:

- build the host development engine and if the target is another
platform then build standalones and externals for that.
- use with a packaged build

## Usage in IDE

- open the `StandaloneTestRunnerBuilder.livecodescript` stack or
otehrwise load it into memory
- open the message box and use the following command where:
  - `<buildplatform>` is a standalone builder build platform
  - `<options>` is an array of extra build options:
    - `noui` - boolean
    - `target` - a deploy target
    - `profile` - a provisioning profile uuid for iOS

```
dispatch "BuildAndRun" to stack "StandaloneTestRunnerBuilder" with <buildplatform>, <options>; put the result
```

## Usage on command line

- `-h,--help`       Basically the same as this
- `-p,--platform`   Specify the build platform to build and run e.g --platform "MacOSX x86-64"
- `-a,--all`        Build and run all platforms
- `-t,--target`     Specify the build target name for mobile builds. If not specified and the all flag is set" & return & \
  then all targets will be run. e.g --target" && quote & "iPad Simulator 10.3" & quote & return & \
- `--profile`       Specify the iOS provisioning profile. Only necessary for device builds." & return & \         
- `-v,--verbose`    Output all results instead of just failures

In the following command examples `lcide` is an alias to the community
development engine.

```
lcide tests/_standalonetestrunner/StandaloneTestRunnerBuilder.livecodescript --verbose --platform "Emscripten"
lcide tests/_standalonetestrunner/StandaloneTestRunnerBuilder.livecodescript -va
```
