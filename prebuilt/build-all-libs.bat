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
	SET LIBS=openssl,curl,icu
) ELSE (
	SET LIBS=%1
)

SET TOOL=14
FOR /f "tokens=1* delims=," %%L IN ("%LIBS%") DO (
	SET LIB=%%L

	ECHO Building !LIB! for four configurations

	MKDIR %LOCKPATH%
	FOR %%M IN (debug,release) DO (
		FOR %%A IN (x86,x86_64) DO (
			SET MODE=%%M
			SET ARCH=%%A

			REM The prepare step is done synchronously
			ECHO Preparing %%L-%%M-%%A
			CALL build-libs.bat %%L prepare

			REM The build step is done asynchronously
			ECHO Starting %%L-%%M-%%A
			START "%%L-%%M-%%A" 9>"%LOCKPATH%\%%L-%%M-%%A.lock" CMD /C build-libs.bat %%L
		)
	)

	ECHO Waiting for all !LIB! builds to finish

	:WaitForEnd
	1>NUL 2>NUL PING /n 2 ::1
	FOR %%K IN (!LIB!-debug-x86,!LIB!-release-x86,!LIB!-debug-x86_64,!LIB!-release-x86_64) DO (
		(CALL ) 9>"%LOCKPATH%\%%K.lock" || GOTO :WaitForEnd
	) 2>NUL

	RMDIR /S /Q %LOCKPATH%
	
	ECHO Finished building !LIB! for four configurations
 )
