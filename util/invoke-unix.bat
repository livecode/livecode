@echo off

REM This batch file forwards commands to a Unix-like shell

REM We use the delayed-expansion feature of the command processor
SETLOCAL EnableDelayedExpansion

SET commands=%*

REM Does this look like a wine environment?
REM We guess this by looking for a common-ish Wine env var
IF DEFINED WINEDEBUG (
  @echo Invoking Unix command '%commands%' via Wine
  %~dp0\invoke-unix-wine.exe %commands%
  EXIT %ERRORLEVEL%
)


IF EXIST C:\Cygwin\bin (
  REM We need to work around what looks to be a MCVS bug: the final parameter is
  REM missing the terminating '"' in come circumstances
  SET commands=%commands%
  
  @echo Invoking Unix command '!commands!' via Cygwin
  
  REM Obscure way to get the cmd.exe equivalent to `...` substitution
  FOR /F "usebackq tokens=*" %%x IN (`C:\Cygwin\bin\cygpath.exe %CD%`) DO SET cygwin_cd=%%x
  FOR %%x IN (!commands!) DO (
    FOR /F "usebackq tokens=*" %%y IN (`C:\Cygwin\bin\bash.exe -c 'if [[ %%x ^=^= -* ]] ^; then echo %%x ^; else /bin/cygpath %%x ^; fi'`) DO SET cygwin_cmd=!cygwin_cmd! %%y
  )

  SET PATH=C:\Cygwin\bin:%PATH%
  C:\Cygwin\bin\bash.exe -c 'cd !cygwin_cd! ^&^& !cygwin_cmd!'
  EXIT %ERRORLEVEL%
)

@echo Could not find Cygwin or wine
EXIT 1

