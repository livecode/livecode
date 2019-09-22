@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

REM ############################################################################
REM #
REM #   BUILD Externals
REM #

IF %MODE%==debug (
	echo Skipping debug builds of externals
	GOTO :EOF
)

ECHO Building Externals

CALL "%~dp0\build-external.bat" mergJSON "https://github.com/montegoulding/mergjson.git"
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

CALL "%~dp0\build-external.bat" mergMarkdown "https://github.com/montegoulding/mergmarkdown.git"
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

CALL "%~dp0\build-external.bat" blur "https://github.com/montegoulding/blur.git"
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

ECHO Finished building externals for all configurations
