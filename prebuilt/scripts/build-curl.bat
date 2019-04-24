@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

REM ############################################################################
REM #
REM #   BUILD CURL
REM #

ECHO Build Curl for %BUILDTRIPLE%

SET CURL_TGZ=%_ROOT_DIR%\curl-%CURL_VERSION%.tar.gz
SET CURL_SRC=%_ROOT_DIR%\curl-%CURL_VERSION%-%BUILDTRIPLE%-src
SET CURL_BIN=%_ROOT_DIR%\curl-%CURL_VERSION%-%BUILDTRIPLE%-bin
SET CURL_BUILD_LOG=%_ROOT_DIR%\curl-%CURL_VERSION%-%BUILDTRIPLE%.log

IF DEFINED CURL_BUILDREVISION (
	SET CURL_TAR=%_PACKAGE_DIR%\Curl-%CURL_VERSION%-%BUILDTRIPLE%-%CURL_BUILDREVISION%.tar
) ELSE (
	SET CURL_TAR=%_PACKAGE_DIR%\Curl-%CURL_VERSION%-%BUILDTRIPLE%.tar
)

REM The files (relative to BIN) to include in the tar archive (UNIX path format)
SET CURL_FILES=include lib/libcurl_a.lib lib/libcurl_a.pdb

cd "%_ROOT_DIR%"

if not exist %CURL_TGZ% (
	echo Fetching curl-%CURL_VERSION% for %BUILDTRIPLE%
	perl -MLWP::Simple -e "getstore('http://curl.haxx.se/download/curl-%CURL_VERSION%.tar.gz', '%CURL_TGZ%')"
)

if not exist %CURL_SRC% (
	echo Unpacking curl-%CURL_VERSION% for %BUILDTRIPLE%
	perl -MArchive::Tar -e "$Archive::Tar::FOLLOW_SYMLINK=1;Archive::Tar->extract_archive('%CURL_TGZ%', 1);"
	ren curl-%CURL_VERSION% curl-%CURL_VERSION%-%BUILDTRIPLE%-src
)

IF "%1"=="prepare" (
	EXIT /B 0
)

cd "%CURL_SRC%"

cd winbuild

ECHO Preparing Curl for %BUILDTRIPLE%
IF DEFINED OpenSSL_BUILDREVISION (
	SET OPENSSL_TBZ=%_PACKAGE_DIR%\OpenSSL-%OpenSSL_VERSION%-%BUILDTRIPLE%-%OpenSSL_BUILDREVISION%.tar.bz2
) ELSE (
	SET OPENSSL_TBZ=%_PACKAGE_DIR%\OpenSSL-%OpenSSL_VERSION%-%BUILDTRIPLE%.tar.bz2
)

IF NOT EXIST %OPENSSL_TBZ% (
	ECHO OpenSSL must be built before Curl
	EXIT /B 1
)

ECHO ========== PREPARING ==========  >%CURL_BUILD_LOG%
FOR /F "usebackq tokens=*" %%x IN (`cygpath.exe -u %OPENSSL_TBZ%`) DO SET OPENSSL_TBZ_CYG=%%x
FOR /F "usebackq tokens=*" %%x IN (`cygpath.exe -u %CURL_BIN%`) DO SET CURL_BIN_CYG=%%x

IF NOT EXIST "%CURL_BIN%" MKDIR "%CURL_BIN%"
bash -c "tar --extract --bzip2 --file=%OPENSSL_TBZ_CYG% --directory=%CURL_BIN_CYG% --strip-components=1" >>%CURL_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

ECHO Configuring Curl for %BUILDTRIPLE%
ECHO ========== CONFIGURING ==========  >>%CURL_BUILD_LOG%
SET CURL_CONFIG=VC=%TOOL% WITH_DEVEL="%CURL_BIN%" WITH_SSL=static GEN_PDB=yes RTLIBCFG=static ENABLE_IDN=no

IF %ARCH%==x86_64 (
  SET SHORT_ARCH=x64
  SET CURL_CONFIG=%CURL_CONFIG% MACHINE=x64
) ELSE (
  SET SHORT_ARCH=x86
  SET CURL_CONFIG=%CURL_CONFIG% MACHINE=x86
)

IF %MODE%==debug (
	SET CURL_CONFIG=%CURL_CONFIG% DEBUG=yes
	SET LIBSUFFIX=_debug
) ELSE (
	SET CURL_CONFIG=%CURL_CONFIG% DEBUG=no
	SET LIBSUFFIX=
)

ECHO Building Curl for %BUILDTRIPLE%
ECHO ========== BUILDING ==========  >>%CURL_BUILD_LOG%
nmake /F Makefile.vc MODE=static %CURL_CONFIG% >>%CURL_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

ECHO Packaging Curl for %BUILDTRIPLE%
ECHO ========== PACKAGING ==========  >>%CURL_BUILD_LOG%
IF NOT EXIST "%CURL_BIN%\include" MKDIR "%CURL_BIN%\include"
IF NOT EXIST "%CURL_BIN%\lib"     MKDIR "%CURL_BIN%\lib"
XCOPY /E /Y ..\builds\libcurl-vc%TOOL%-%SHORT_ARCH%-%MODE%-static-ssl-static-ipv6-sspi\include\*     "%CURL_BIN%\include" >>%CURL_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%
COPY /Y ..\builds\libcurl-vc%TOOL%-%SHORT_ARCH%-%MODE%-static-ssl-static-ipv6-sspi\lib\libcurl_a%LIBSUFFIX%.lib "%CURL_BIN%\lib\libcurl_a.lib" >>%CURL_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%
COPY /Y ..\builds\libcurl-vc%TOOL%-%SHORT_ARCH%-%MODE%-static-ssl-static-ipv6-sspi\lib\libcurl_a%LIBSUFFIX%.pdb "%CURL_BIN%\lib\libcurl_a.pdb" >>%CURL_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

REM Build the prebuilt archives

cd "%CURL_BIN%"
FOR /F "usebackq tokens=*" %%x IN (`cygpath.exe -u %CURL_TAR%`) DO SET CURL_TAR_CYG=%%x

ECHO Archiving Curl for %BUILDTRIPLE%
ECHO ========== ARCHIVING ==========  >>%CURL_BUILD_LOG%
bash -c "tar --create --file=%CURL_TAR_CYG% --transform='flags=r;s|^|%BUILDTRIPLE%/|' %CURL_FILES%" >>%CURL_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

ECHO Compressing Curl for %BUILDTRIPLE%
ECHO ========== COMPRESSING ==========  >>%CURL_BUILD_LOG%
bash -c "bzip2 --force %CURL_TAR_CYG%" >>%CURL_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%
