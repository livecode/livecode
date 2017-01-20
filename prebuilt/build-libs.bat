@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

REM ############################################################################
REM #
REM #   CONFIGURE VERSIONS AND FOLDERS
REM #

SET _ROOT_DIR=C:\LiveCode\Prebuilt\libraries
SET _INSTALL_DIR=%_ROOT_DIR%\prefix
SET _WINSDK_ROOT=C:\Program Files\Microsoft SDKs\Windows\v7.1

echo "Will build into %_ROOT_DIR%"
MKDIR %_ROOT_DIR%

echo "Will install into %_INSTALL_DIR%"
MKDIR %_INSTALL_DIR%

REM # get the drive & path of the folder this script lives in
REM # (note: ends with \ path delimiter)
FOR /F "delims=" %%A IN ("%0") DO SET _TOOLS_DIR=%%~dpA

REM Get the libraries version variables set from scripts/lib_versions.bat
CALL "scripts\lib_versions.bat"

REM Architecture defaults to x86
IF NOT DEFINED ARCH (
  SET ARCH=x86
)

REM Set up the compilation environment
if %ARCH%==x86_64 (
  CALL "%_WINSDK_ROOT%\bin\setenv.cmd" /x64 /release /xp
) ELSE (
  CALL "%_WINSDK_ROOT%\bin\setenv.cmd" /x86 /release /xp
)

REM Ensure Cygwin and NASM are in the path
WHERE /Q bash 1>NUL 2>NUL
IF %ERRORLEVEL% NEQ 0 (
  SET "PATH=%PATH%;C:\Cygwin64\bin"
)
WHERE /Q nasm 1>NUL 2>NUL
IF %ERRORLEVEL% NEQ 0 (
  SET "PATH=%PATH%;C:\Program Files (x86)\NASM"
)


REM Build OpenSSL
CALL "scripts\build-openssl.bat"
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

REM Build Curl
CALL "scripts\build-curl.bat"
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

REM Build ICU
CALL "scripts\build-icu.bat"
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%
