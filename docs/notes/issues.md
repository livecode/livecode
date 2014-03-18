#Known issues

Every effort has been made to ensure that externally, the engine behaviour is identical to the current unrefactored release. In other words, users should not notice any difference in functionality in their existing stacks. However, users will notice a general slow-down caused by lack of optimisation in this release - this will be addressed for DP 2.

*The installer will currently fail if you run it from a network share on Windows. Please copy the installer to a local disk before launching on this platform.
*The engine files are much larger than previous versions due to inclusion of ICU data
*LiveCode does not run correctly when installed to Unicode paths on OSX
*On Windows, executing LiveCode from the installer fails as it cannot find the IDE
*Android app label is not yet Unicode compatible
