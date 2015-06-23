@echo off
REM Configures the Windows build

SETLOCAL EnableExtensions

REM Attempt to locate a copy of Perl
WHERE /Q perl 1>NUL 2>NUL
IF %ERRORLEVEL% NEQ 0 (
  IF EXIST C:\perl64\bin\perl.exe (
    SET extra_options=%extra_options% -Dperl="C:/perl64/bin/perl.exe"
  ) ELSE IF EXIST C:\perl\bin\perl.exe (
    SET extra_options=%extra_options% -Dperl="C:/perl/bin/perl.exe"
  ) ELSE (
    ECHO >&2 Error: could not locate a copy of perl
    PAUSE
    EXIT 1
  )
)

REM Attempt to locate a copy of Python
WHERE /Q python 1>NUL 2>NUL
IF %ERRORLEVEL% NEQ 0 (
  IF EXIST C:\Python27\python.exe (
    SET python=C:\Python27\python.exe
  ) ELSE (
    ECHO >&2 Error: could not locate a copy of python
    PAUSE
    EXIT 1
  )
) ELSE (
  SET python=python
)

REM Run the configure step
%python% gyp\gyp_main.py --format msvs --depth . --generator-output build-win-x86/livecode -Gmsvs_version=2010 %extra_options%

REM Pause if there was an error so that the user gets a chance to see it
IF %ERRORLEVEL% NEQ 0 PAUSE
EXIT %ERRORLEVEL%

