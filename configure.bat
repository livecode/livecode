@echo off
REM Configures the Windows build

SETLOCAL EnableExtensions

REM Set this variable if any warnings have been reported
SET warnings=0

REM Not all versions of windows the the %programfiles(x86)% variable
IF NOT DEFINED programfiles(x86) SET programfiles(x86)=%programfiles%

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
    EXIT 1
  )
) ELSE (
  SET python=python
)

REM Attempt to locate the QuickTime SDK
IF EXIST "%programfiles(x86)%\QuickTime SDK\*" (
  SET extra_options=%extra_options% -Dquicktime_sdk="%programfiles(x86)%/QuickTime SDK"
) ELSE (
  ECHO >&2 Error: could not locate the QuickTime SDK
  PAUSE
  EXIT 1
)

REM Attempt to locate the Microsoft Speech SDK v5.1
IF EXIST "%programfiles(x86)%\Microsoft Speech SDK 5.1\*" (
  SET extra_options=%extra_options% -Dms_speech_sdk5="%programfiles(x86)%/Microsoft Speech SDK 5.1"
) ELSE (
  ECHO >&2 Warning: could not locate the Microsoft Speech SDK v5.1; revSpeech will not build
  SET warnings=1
)

REM Attempt to locate the Microsoft Speech SDK v4
IF EXIST "%programfiles(x86)%\Microsoft Speech SDK\*" (
  SET extra_options=%extra_options% -Dms_speech_sdk4="%programfiles(x86)%/Microsoft Speech SDK"
) ELSE (
  ECHO >&2 Warning: could not locate the Microsoft Speech SDK v4; revSpeech will not build
  SET warnings=1
)

REM Pause so any warnings can be seen
IF %warnings% NEQ 0 PAUSE

REM Run the configure step
%python% config.py --platform win-x86 %extra_options% %gypfile%
PAUSE

REM Pause if there was an error so that the user gets a chance to see it
IF %ERRORLEVEL% NEQ 0 PAUSE
EXIT %ERRORLEVEL%
