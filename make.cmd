@REM Make sure ProgramFiles(x86) variable is defined
@SET "ProgramFilesBase=%ProgramFiles% (x86)"
@IF NOT EXIST "%ProgramFilesBase%" (
  SET "ProgramFilesBase=%ProgramFiles%"
)

ECHO %ProgramFilesBase%

@REM Set up environment so that we can run Visual Studio.
@REM More-or-less the same effect as running vcvars32.bat
@REM from the Visual Studio source tree.
@REM @call vcvars32
@set "PATH=%ProgramFilesBase%\Microsoft Visual Studio 10.0\VSTSDB\Deploy;%ProgramFilesBase%\Microsoft Visual Studio 10.0\Common7\IDE\;%ProgramFilesBase%\Microsoft Visual Studio 10.0\VC\BIN;%ProgramFilesBase%\Microsoft Visual Studio 10.0\Common7\Tools;C:\Windows\Microsoft.NET\Framework\v4.0.30319;C:\Windows\Microsoft.NET\Framework\v3.5;%ProgramFilesBase%\Microsoft Visual Studio 10.0\VC\VCPackages;%ProgramFilesBase%\HTML Help Workshop;%ProgramFilesBase%\Microsoft SDKs\Windows\7.0A\bin\NETFX 4.0 Tools;%ProgramFilesBase%\Microsoft SDKs\Windows\7.0A\bin;C:\Perl64\site\bin;C:\Perl64\bin;C:\Perl\site\bin;C:\Perl\bin;C:\windows\system32;C:\windows;C:\windows\system32\wbem"

@REM Works around hangs when generating .pdb files
@REM Needs to run in the background as never terminates
@REM
@REM Run this with its CWD outside the build tree so that
@REM the fact it hangs around does not interfere with
@REM cleaning up the build tree.
@pushd \
@REM @start /min mspdbsrv -start -spawn -shutdowntime -1
@popd

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

IF -%1-==-- (
  @msbuild %BUILD_PROJECT% /fl /flp:Verbosity=normal /nologo /p:Configuration=%BUILDTYPE% /m:1
) ELSE (
  @msbuild %BUILD_PROJECT% /fl /flp:Verbosity=normal /nologo /p:Configuration=%BUILDTYPE% /m:1 /t:%TARGET%
)

@exit %ERRORLEVEL%
