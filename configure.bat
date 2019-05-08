@echo off
REM Configures the Windows build

SETLOCAL EnableExtensions

REM Set this variable if any warnings have been reported
SET warnings=0

REM Not all versions of windows the the %programfiles(x86)% variable
IF NOT DEFINED programfiles(x86) SET programfiles(x86)=%programfiles%

REM When calling configure.bat from the command line, BUILD_EDITION is not defined
IF NOT DEFINED BUILD_EDITION SET BUILD_EDITION="community"

REM Target architecture currently defaults to 32-bit x86
IF NOT DEFINED TARGET_ARCH SET TARGET_ARCH=x64

REM Make sure TARGET_ARCH is always x86 or x86_64

IF %TARGET_ARCH%==x64 (
  SET TARGET_ARCH=x86_64
) ELSE IF %TARGET_ARCH%==i386 (
  SET TARGET_ARCH=x86
) ELSE IF %TARGET_ARCH% ==x86 (
  REM Valid
) ELSE IF %TARGET_ARCH% == x86_64 (
  REM Valid
) ELSE (
  ECHO >&2 Error: invalid target arch %TARGET_ARCH%
  EXIT /B 1
)

REM Note: to test whether a directory exists in batch script, you need to check
REM whether a file within that directory exists. Easiest way to do this is to
REM add the "*" wildcard after the directory

REM Attempt to locate a copy of Python
WHERE /Q python 1>NUL 2>NUL
IF %ERRORLEVEL% NEQ 0 (
  IF EXIST C:\Python27\python.exe (
    SET python=C:\Python27\python.exe
  ) ELSE IF EXIST C:\Python26\python.exe (
    SET python=C:\Python26\python.exe
  ) ELSE (
    ECHO >&2 Error: could not locate a copy of python
    PAUSE
    EXIT /B 1
  )
) ELSE (
  SET python=python
)

REM Attempt to locate the Microsoft Speech SDK v5.1
IF EXIST "%programfiles(x86)%\Microsoft Speech SDK 5.1\*" (
  SET extra_options=%extra_options% -Dms_speech_sdk5="%programfiles(x86)%/Microsoft Speech SDK 5.1"
) ELSE (
  ECHO >&2 Warning: could not locate the Microsoft Speech SDK v5.1; revSpeech will not build
  SET warnings=1
)

REM Pause so any warnings can be seen
IF %warnings% NEQ 0 PAUSE

REM Run the configure step
%python% config.py --platform win-%TARGET_ARCH% %extra_options% %gypfile%
PAUSE

REM Pause if there was an error so that the user gets a chance to see it
IF %ERRORLEVEL% NEQ 0 PAUSE
EXIT /B %ERRORLEVEL%
