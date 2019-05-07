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

SET PREBUILT_LIBS=OpenSSL Curl ICU CEF thirdparty

ECHO Building Libs %PREBUILT_LIBS%

SET TOOL=15
FOR %%L IN (%PREBUILT_LIBS%) DO (
	SET PREBUILT_LIB=%%L

	ECHO Building !PREBUILT_LIB! for all configurations

	ECHO Preparing !PREBUILT_LIB!
	CALL build-libs.bat !PREBUILT_LIB! prepare

	FOR %%M IN (debug,release) DO (
		SET MODE=%%M
		SET TRIPLE=!PREBUILT_LIB!-!MODE!-%ARCH%
		ECHO Starting !TRIPLE!
		CALL build-libs.bat !PREBUILT_LIB!
	)

	ECHO Finished building !PREBUILT_LIB! for all configurations
)
