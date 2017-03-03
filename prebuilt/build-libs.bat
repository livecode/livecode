@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

REM ############################################################################
REM #
REM #   CONFIGURE VERSIONS AND FOLDERS
REM #

REM If called with no arguments ICU, OpenSSL and Curl will be built.
REM Otherwise it can be called with one or two arguments. The first argument is
REM the library to build, the second is either not present or 'prepare'. If
REM 'prepare', then the source will be downloaded and unpacked into a unique
REM library/toolset/crt folder reading for building.

REM All libs can be built with three distinct options define in env vars:
REM   ARCH := x86 or x86_64 -- default x86
REM   MODE := debug or release -- default debug
REM   TOOL := any valid VS tool version
REM
REM The defaults for these variables if they are unset are x86, debug, 14.
REM
REM The resulting built libraries are packaged into archives containing a lib
REM and include folder. The naming of the archives follows the GNU triple
REM convention:
REM   ARCH-win32-SDK
REM Where:
REM   ARCH := x86 or x86_64
REM   SDK := TOOLSET_CRT
REM Here TOOLSET is the VS PlatformToolset string - e.g. v140 for VS2015; and
REM CRT is the one of MT, MTd, MD, MDd. This corresponds to static, static debug
REM dynamic, and dynamic debug variants.

REM This script checks for bash in the path. If it is not present it assumes it
REM is installed as part of 64-bit cygwin at C:\Cygwin\bin

REM This script checks for nasm in the path. If it is not present it assumes it
REM is installed at C:\Program Files (x86)\NASM

SET _ROOT_DIR=C:\LiveCode\Prebuilt\libraries

echo Will build into %_ROOT_DIR%
IF NOT EXIST %_ROOT_DIR% MKDIR %_ROOT_DIR%

REM # get the drive & path of the folder this script lives in
REM # (note: ends with \ path delimiter)
FOR /F "delims=" %%A IN ("%0") DO SET _TOOLS_DIR=%%~dpA

REM Get the libraries version variables set from scripts/lib_versions.bat
CALL "scripts\lib_versions.bat"

REM Architecture defaults to x86
IF NOT DEFINED ARCH (
  SET ARCH=x86
)

IF NOT DEFINED MODE (
  SET MODE=debug
)

IF NOT DEFINED TOOL (
	SET TOOL=14
)

REM Check variable values
IF %ARCH%==x86 (
	REM
) ELSE (
	IF %ARCH%==x86_64 (
		REM
	) ELSE (
		ECHO ARCH variable must be x86 or x86_64
		EXIT /B 1
	)
)

IF %MODE%==debug (
	REM
) ELSE (
	IF %MODE%==release (
		REM
	) ELSE (
		ECHO MODE variable must be debug or release
		EXIT /B 1
	)
)

REM Compute the mode suffix - MD, MDd, MT, MTd - these correspond
REM to the CRT options available: DLL, DLL debug, Static, Static debug.
IF %MODE%==debug (
	SET CRT=static_debug
) ELSE (
	SET CRT=static_release
)

REM Set the suffix that should be used by all libraries
SET BUILDTRIPLE=%ARCH%-win32-v%TOOL%0_%CRT%

REM Compute path to VS version
IF %ARCH%==x86 (
	SET ARCH_STRING=x86
) ELSE (
	SET ARCH_STRING=amd64
)

SET VSCONFIGTOOL="C:\Program Files (x86)\Microsoft Visual Studio %TOOL%.0\VC\vcvarsall.bat"

REM Ensure the desired vsvarsall.bat file exists for the chosen options
IF NOT EXIST %VSCONFIGTOOL% (
	ECHO Cannot find Visual Studio configuration tool config batch file at %VSCONFIGTOOL%
	EXIT /B 1
)

REM Configure the visual studio tools
CALL %VSCONFIGTOOL% %ARCH_STRING%
WHERE /Q cl 1>NUL 2>NUL
IF %ERRORLEVEL% NEQ 0 (
	ECHO Configuration of Visual Studio tools failed
	EXIT /B 1
)

REM Ensure Cygwin and NASM are in the path
WHERE /Q bash 1>NUL 2>NUL
IF %ERRORLEVEL% NEQ 0 (
  SET "PATH=%PATH%;C:\Cygwin64\bin"
)
WHERE /Q bash 1>NUL 2>NUL
IF %ERRORLEVEL% NEQ 0 (
	ECHO Cannot find 'bash'. Make sure Cygwin64 is installed with root C:\Cygwin64 and bash is present.
	EXIT /B 1
)

WHERE /Q nasm 1>NUL 2>NUL
IF %ERRORLEVEL% NEQ 0 (
   SET "PATH=%PATH%;C:\Program Files (x86)\NASM"
)
WHERE /Q nasm 1>NUL 2>NUL
IF %ERRORLEVEL% NEQ 0 (
	ECHO Cannot find 'nasm'. Make sure nasm is installed with root "C:\Program Files (x86)\NASM".
	EXIT /B 1
)

IF %1=="" (
	REM Build OpenSSL
	CALL "scripts\build-openssl.bat"
	IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

	REM Build Curl
	CALL "scripts\build-curl.bat"
	IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

	REM Build ICU
	CALL "scripts\build-icu.bat"
	IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%
) ELSE (
	CALL "scripts\build-%1.bat" %2
	IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%
)
