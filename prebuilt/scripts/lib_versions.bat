REM # This file sets the libraries versions defined in versions/

FOR %%L in (OpenSSL ICU CEF Curl) DO (
	SET PREBUILT_LIB=%%L
	SET /P !PREBUILT_LIB!_VERSION=<versions\!PREBUILT_LIB!
	IF EXIST "versions\!PREBUILT_LIB!_buildrevision" (
		SET /P !PREBUILT_LIB!_BUILDREVISION=<versions\!PREBUILT_LIB!_buildrevision
	)
)
