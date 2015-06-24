@REM Set up environment so that we can run Visual Studio.
@REM More-or-less the same effect as running vcvars32.bat
@REM from the Visual Studio source tree.
@REM @call vcvars32
@set "PATH=C:\Program Files\Microsoft Visual Studio 10.0\VSTSDB\Deploy;C:\Program Files\Microsoft Visual Studio 10.0\Common7\IDE\;C:\Program Files\Microsoft Visual Studio 10.0\VC\BIN;C:\Program Files\Microsoft Visual Studio 10.0\Common7\Tools;C:\Windows\Microsoft.NET\Framework\v4.0.30319;C:\Windows\Microsoft.NET\Framework\v3.5;C:\Program Files\Microsoft Visual Studio 10.0\VC\VCPackages;C:\Program Files\HTML Help Workshop;C:\Program Files\Microsoft SDKs\Windows\7.0A\bin\NETFX 4.0 Tools;C:\Program Files\Microsoft SDKs\Windows\7.0A\bin;C:\Perl\site\bin;C:\Perl\bin;C:\windows\system32;C:\windows;C:\windows\system32\wbem"
 
@REM Works around hangs when generating .pdb files
@REM Needs to run in the background as never terminates
@REM
@start /min /b mspdbsrv -start -spawn -shutdowntime -1

@REM Select the correct build mode.
@REM 
@IF NOT DEFINED BUILDTYPE SET BUILDTYPE=Debug

@REM Select the correct build project file
@REM
IF NOT DEFINED BUILD_EDITION SET BUILD_EDITION=community
IF %BUILD_EDITION%==commercial (
  SET BUILD_PROJECT=livecode-commercial.sln
) ELSE (
  SET BUILD_PROJECT=livecode\livecode.sln
)
 
@msbuild %BUILD_PROJECT% /fl /flp:Verbosity=normal /nologo /p:Configuration=%BUILDTYPE% /m:1
 
@exit %ERRORLEVEL%

