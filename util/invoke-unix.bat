@echo off

REM This batch file forwards commands to a Unix-like shell

IF EXIST C:\Cygwin\bin (
  C:\Cygwin\bin\bash.exe -c '%*'
) ELSE (
  invoke-unix-wine.exe %*
)

EXIT %ERRORLEVEL%

