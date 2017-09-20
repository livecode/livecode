@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

REM ############################################################################
REM #
REM #   BUILD OPENSSL
REM #

ECHO Build OpenSSL for %BUILDTRIPLE%

SET OPENSSL_TGZ=%_ROOT_DIR%\openssl-%OpenSSL_VERSION%.tar.gz
SET OPENSSL_SRC=%_ROOT_DIR%\openssl-%OpenSSL_VERSION%-%BUILDTRIPLE%-src
SET OPENSSL_BIN=%_ROOT_DIR%\openssl-%OpenSSL_VERSION%-%BUILDTRIPLE%-bin
SET OPENSSL_BUILD_LOG=%_ROOT_DIR%\openssl-%OpenSSL_VERSION%-%BUILDTRIPLE%.log

IF DEFINED OpenSSL_BUILDREVISION (
	SET OPENSSL_TAR=%_PACKAGE_DIR%\OpenSSL-%OpenSSL_VERSION%-%BUILDTRIPLE%-%OpenSSL_BUILDREVISION%.tar
) ELSE (
	SET OPENSSL_TAR=%_PACKAGE_DIR%\OpenSSL-%OpenSSL_VERSION%-%BUILDTRIPLE%.tar
)

REM The files (relative to BIN) to include in the tar archive (UNIX path format)
SET OPENSSL_FILES=lib/libeay32.lib lib/ssleay32.lib lib/ossl_static.pdb include/openssl

REM

cd "%_ROOT_DIR%"

if not exist %OPENSSL_TGZ% (
	echo Fetching openssl-%OPENSSL_VERSION%
	perl -MLWP::Simple -e "getstore('http://www.openssl.org/source/openssl-%OpenSSL_VERSION%.tar.gz', '%OPENSSL_TGZ%')"
)

if not exist %OPENSSL_SRC% (
	echo Unpacking openssl-%OPENSSL_VERSION% for %BUILDTRIPLE%
	perl -MArchive::Tar -e "$Archive::Tar::FOLLOW_SYMLINK=1;Archive::Tar->extract_archive('%OpenSSL_TGZ%', 1);"
	ren openssl-%OpenSSL_VERSION% openssl-%OpenSSL_VERSION%-%BUILDTRIPLE%-src
)

IF "%1"=="prepare" (
	EXIT /B 0
)

cd "%OPENSSL_SRC%"

REM Configure and build OpenSSL
REM We disable hardware crypto engine support and any patented algorithms
REM We build as static (but position-independent) libraries then link manually later
SET OPENSSL_CONFIG=no-hw no-rc5 no-shared --prefix=%OPENSSL_BIN%

IF %MODE%==debug (
	SET OPENSSL_CONFIG=%OPENSSL_CONFIG% --debug
)

IF %ARCH%==x86_64 (
  SET OPENSSL_CONFIG=%OPENSSL_CONFIG% VC-WIN64A
  SET BITS=64
  SET MACHINE=x64
) ELSE (
  SET OPENSSL_CONFIG=%OPENSSL_CONFIG% VC-WIN32
  SET BITS=32
  SET MACHINE=x86
)

ECHO Configuring OpenSSL for %BUILDTRIPLE%
ECHO ========== CONFIGURING ==========  >%OPENSSL_BUILD_LOG%
perl Configure %OPENSSL_CONFIG% >>%OPENSSL_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

ECHO Building OpenSSL for %BUILDTRIPLE%
ECHO ========== BUILDING ==========  >>%OPENSSL_BUILD_LOG%
nmake clean 1>NUL 2>NUL
nmake >>%OPENSSL_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

ECHO Testing OpenSSL for %BUILDTRIPLE%
ECHO ========== TESTING ==========  >>%OPENSSL_BUILD_LOG%
nmake test >>%OPENSSL_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

ECHO Packaging OpenSSL for %BUILDTRIPLE%
ECHO ========== PACKAGING ==========  >>%OPENSSL_BUILD_LOG%

IF NOT EXIST "%OPENSSL_BIN%\include" MKDIR "%OPENSSL_BIN%\include"
IF NOT EXIST "%OPENSSL_BIN%\lib"     MKDIR "%OPENSSL_BIN%\lib"

XCOPY /E /Y /I include\openssl\* "%OPENSSL_BIN%\include\openssl\" >>%OPENSSL_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

REM Current versions of Curl still use the old library names for linking against OpenSSL 1.1.0 on Windows
COPY /Y libcrypto.lib "%OPENSSL_BIN%\lib\libeay32.lib" >>%OPENSSL_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

COPY /Y libssl.lib    "%OPENSSL_BIN%\lib\ssleay32.lib" >>%OPENSSL_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

COPY /Y ossl_static.pdb    "%OPENSSL_BIN%\lib\ossl_static.pdb" >>%OPENSSL_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

REM Build the prebuilt archives

cd "%OPENSSL_BIN%"
FOR /F "usebackq tokens=*" %%x IN (`cygpath.exe -u %OPENSSL_TAR%`) DO SET OPENSSL_TAR_CYG=%%x

ECHO Archiving OpenSSL for %BUILDTRIPLE%
ECHO ========== ARCHIVING ==========  >>%OPENSSL_BUILD_LOG%
bash -c "tar --create --file=%OPENSSL_TAR_CYG% --transform='flags=r;s|^|%BUILDTRIPLE%/|' %OPENSSL_FILES%" >>%OPENSSL_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

ECHO Compressing OpenSSL for %BUILDTRIPLE%
ECHO ========== COMPRESSING ==========  >>%OPENSSL_BUILD_LOG%
bash -c "bzip2 --force %OPENSSL_TAR_CYG%" >>%OPENSSL_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%
