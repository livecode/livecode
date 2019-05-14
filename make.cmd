@rem Try to detect if running on 64-bit or 32-bit windows
@rem Choose some sensible defaults
set ProgramFilesBase=%ProgramFiles(x86)%
if defined ProgramFiles(x86) (
    if not defined BUILD_PLATFORM set BUILD_PLATFORM=win-x86_64
) else (
    set ProgramFilesBase=%ProgramFiles%
    if not defined BUILD_PLATFORM set BUILD_PLATFORM=win-x86
)

@rem Guess build mode
if not defined BUILDTYPE set BUILDTYPE=Debug

@rem Guess build project
if not defined BUILD_EDITION set BUILD_EDITION=community
if /I "%BUILD_EDITION%"=="commercial" (
    set BUILD_PROJECT=livecode-commercial.sln
    set DEFAULT_TARGET=____\default
) else (
    set BUILD_PROJECT=livecode\livecode.sln
    set DEFAULT_TARGET=default
)

@rem Guess target architecture based on build platform
if /I "%BUILD_PLATFORM%"=="win-x86_64" (
    @set VSCMD_ARG_TGT_ARCH=x64
    @set MSBUILD_PLATFORM=x64
)
if /I "%BUILD_PLATFORM%"=="win-x86" (
    @set VSCMD_ARG_TGT_ARCH=x86
    @set MSBUILD_PLATFORM=Win32
)

@rem Try to build with VS 2017 build tools by default
if not defined VSINSTALLDIR set VSINSTALLDIR=%ProgramFilesBase%\Microsoft Visual Studio\2017\BuildTools\
call "%VSINSTALLDIR%VC\Auxiliary\Build\vcvarsall.bat" %VSCMD_ARG_TGT_ARCH%

@if "%1" NEQ "" (
	set MSBUILD_TARGET_ARG=/t:%1
) else (
	set MSBUILD_TARGET_ARG=/t:%DEFAULT_TARGET%
)

msbuild %BUILD_PROJECT% /fl /flp:Verbosity=normal /nologo /m:1 %MSBUILD_TARGET_ARG% /p:Configuration=%BUILDTYPE% /p:Platform=%MSBUILD_PLATFORM%

@exit %ERRORLEVEL%
