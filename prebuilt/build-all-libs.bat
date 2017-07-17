@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

SET "LOCKPATH=%TEMP%\build-all-libs-locks"
IF EXIST "%LOCKPATH%" (
	RMDIR /S /Q %LOCKPATH%
	IF %ERRORLEVEL% NEQ 0 (
		ECHO "Build all libs already running"
		EXIT /B 1
	)
)

IF "%1"=="" (
	SET PLATFORM=win32
) ELSE (
	SET PLATFORM=%1
)

IF "%2"=="" (
	SET ARCH=x86
) ELSE (
	SET ARCH=%2
)

SET PREBUILT_LIBS=openssl curl icu
REM IF "%3"=="" (
REM 	SET PREBUILT_LIBS=openssl curl icu
REM ) ELSE (
REM 	SET PREBUILT_LIBS=%3
REM )

ECHO Building Libs %PREBUILT_LIBS%

SET TOOL=14

SET LOCKS=
MKDIR %LOCKPATH%

FOR %%L IN (%PREBUILT_LIBS%) DO (
	SET PREBUILT_LIB=%%L

	ECHO Building !PREBUILT_LIB! for four configurations

	REM The prepare step is done synchronously
	ECHO Preparing !PREBUILT_LIB!
	CALL build-libs.bat !PREBUILT_LIB! prepare

	FOR %%M IN (debug,release) DO (
		SET MODE=%%M
		SET TRIPLE=!PREBUILT_LIB!-!MODE!-%ARCH%
		SET LOCKS=!TRIPLE! !LOCKS!
		REM The build step is done asynchronously
		ECHO Starting !TRIPLE!
		START "!TRIPLE!" 9>"%LOCKPATH%\!TRIPLE!.lock" CMD /C build-libs.bat !PREBUILT_LIB!
	)

	REM ECHO Finished building !PREBUILT_LIB! for four configurations
)

ECHO Waiting for all builds to finish
:WaitForEnd
1>NUL 2>NUL PING /n 2 ::1
FOR %%K IN (!LOCKS!) DO (
	(CALL ) 9>"%LOCKPATH%\%%K.lock" || GOTO :WaitForEnd
) 2>NUL

RMDIR /S /Q %LOCKPATH%
