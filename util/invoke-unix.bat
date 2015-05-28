@echo off

REM This batch file forwards commands to a Unix-like shell
REM
REM We need to work around what looks to be a MCVS bug: the final parameter is
REM missing the terminating '"' when quoted

@echo Invoking Unix command '%*"'

IF EXIST C:\Cygwin\bin (
  REM Obscure way to get the cmd.exe equivalent to `...` substitution
  FOR /F "usebackq tokens=*" %%x IN (`C:\Cygwin\bin\cygpath.exe %CD%`) DO SET cygwin_cd=%%x
  FOR /F "usebackq tokens=*" %%x IN (`C:\Cygwin\bin\cygpath.exe %*"`)  DO SET cygwin_cmd=!cygwin_cmd! %%x

  C:\Cygwin\bin\bash.exe -c 'cd !cygwin_cd! ^&^& !cygwin_cmd!'
) ELSE (
  invoke-unix-wine.exe %*"
)

EXIT %ERRORLEVEL%

