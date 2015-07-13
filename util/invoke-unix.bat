@echo off

REM This batch file forwards commands to a Unix-like shell

REM We use the delayed-expansion feature of the command processor
SETLOCAL EnableDelayedExpansion
SETLOCAL EnableExtensions

SET commands=%*

REM Does this look like a wine environment?
REM We guess this by looking for a common-ish Wine env var
IF DEFINED WINEDEBUG (
  @echo Invoking Unix command '%commands%' via Wine
  %~dp0\invoke-unix-wine.exe %commands%
  EXIT %ERRORLEVEL%
)

REM Is Cygwin already in the path?
REM If not, look in a couple of likely locations for it
WHERE /Q cygpath.exe 1>NUL 2>NUL
IF %ERRORLEVEL% EQU 0 (
  SET cygwin_path=
) ELSE IF DEFINED CYGPATH (
  SET cygwin_path=%CYGPATH%\bin\
) ELSE IF EXIST C:\Cygwin64\bin (
  SET cygwin_path=C:\Cygwin64\bin\
) ELSE IF EXIST C:\Cygwin\bin (
  SET cygwin_path=C:\Cygwin\bin\
) ELSE (
  @ECHO >&2 Cannot locate a Cygwin installation
  EXIT 1
)

REM We need to work around what looks to be a MCVS bug: the final parameter is
REM missing the terminating '"' in come circumstances
SET commands=%commands%

@echo Invoking Unix command '!commands!' via Cygwin

REM Obscure way to get the cmd.exe equivalent to `...` substitution
FOR /F "usebackq tokens=*" %%x IN (`%cygwin_path%cygpath.exe %CD%`) DO SET cygwin_cd=%%x
FOR %%x IN (!commands!) DO (
  FOR /F "usebackq tokens=*" %%y IN (`%cygwin_path%bash.exe -c 'if [[ \'%%x\' ^=^= -* ]] ^; then echo \'%%x\' ^; else /bin/cygpath \'%%x\' ^; fi'`) DO SET cygwin_cmd=!cygwin_cmd! %%y
)

REM All the parameters have been Unix-ified; run the command
SET PATH=%cygwin_path%:%PATH%
%cygwin_path%bash.exe -c 'cd !cygwin_cd! ^&^& !cygwin_cmd!'
EXIT %ERRORLEVEL%

