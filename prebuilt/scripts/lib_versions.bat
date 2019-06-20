REM # This file sets the libraries versions defined in versions/

FOR %%L in (Thirdparty OpenSSL ICU CEF Curl CEFChromium) DO (
	SET PREBUILT_LIB=%%L
	SET /P !PREBUILT_LIB!_VERSION=<versions\!PREBUILT_LIB!
	IF EXIST "versions\!PREBUILT_LIB!_buildrevision" (
		SET /P !PREBUILT_LIB!_BUILDREVISION=<versions\!PREBUILT_LIB!_buildrevision
	)
)

FOR /F "tokens=*" %%x IN ('git -C ../thirdparty/ log -n 1 "--format=%%H"') DO SET Thirdparty_VERSION=%%x
