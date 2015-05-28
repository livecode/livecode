@echo off

REM This batch file forwards commands to a Unix-like shell
REM
REM We need to work around what looks to be a MCVS bug: the final parameter is
REM missing the terminating '"' when quoted
SET commands=%*"

@echo Invoking Unix command '%commands%'

REM Does this look like a wine environment?
REM We guess this by looking for a common-ish Wine env var
IF DEFINED WINEDEBUG (
  START /wait %~dp0\invoke-unix-wine.exe %commands%
  EXIT %ERRORLEVEL%
)


IF EXIST C:\Cygwin\bin (
  REM Obscure way to get the cmd.exe equivalent to `...` substitution
  FOR /F "usebackq tokens=*" %%x IN (`C:\Cygwin\bin\cygpath.exe %CD%`) DO SET cygwin_cd=%%x
  FOR /F "usebackq tokens=*" %%x IN (`C:\Cygwin\bin\cygpath.exe %*"`)  DO SET cygwin_cmd=!cygwin_cmd! %%x

  C:\Cygwin\bin\bash.exe -c 'cd !cygwin_cd! ^&^& !cygwin_cmd!'
  EXIT %ERRORLEVEL%
)

@echo Could not find Cygwin or wine
EXIT 1

