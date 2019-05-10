@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

REM ############################################################################
REM #
REM #   BUILD THIRDPARTY LIBS
REM #

ECHO Build Thirdparty Libs for %BUILDTRIPLE%

SET THIRDPARTY_BIN=%_ROOT_DIR%\thirdparty-%Thirdparty_VERSION%-%BUILDTRIPLE%-bin
SET THIRDPARTY_BUILD_LOG=%_ROOT_DIR%\thirdparty-%Thirdparty_VERSION%-%BUILDTRIPLE%.log

IF DEFINED Thirdparty_BUILDREVISION (
	SET THIRDPARTY_TAR=%_PACKAGE_DIR%\Thirdparty-%Thirdparty_VERSION%-%BUILDTRIPLE%-%Thirdparty_BUILDREVISION%.tar
) ELSE (
	SET THIRDPARTY_TAR=%_PACKAGE_DIR%\Thirdparty-%Thirdparty_VERSION%-%BUILDTRIPLE%.tar
)

if not defined BUILD_PLATFORM set BUILD_PLATFORM=win-%ARCH%

IF "%1"=="prepare" (
	EXIT /B 0
)

ECHO Unpacking OpenSSL headers for %BUILDTRIPLE%
ECHO ========== UNPACKING OPENSSL HEADERS ==========  >%THIRDPARTY_BUILD_LOG%

IF DEFINED OpenSSL_BUILDREVISION (
	SET OPENSSL_TAR=%_PACKAGE_DIR%\OpenSSL-%OpenSSL_VERSION%-%BUILDTRIPLE%-%OpenSSL_BUILDREVISION%.tar.bz2
) ELSE (
	SET OPENSSL_TAR=%_PACKAGE_DIR%\OpenSSL-%OpenSSL_VERSION%-%BUILDTRIPLE%.tar.bz2
)
FOR /F "usebackq tokens=*" %%x IN (`cygpath.exe -u %OPENSSL_TAR%`) DO SET OPENSSL_TAR_CYG=%%x

SET OPENSSL_UNPACK_DIR=%_TOOLS_DIR%\unpacked\OpenSSL
FOR /F "usebackq tokens=*" %%x IN (`cygpath.exe -u %OPENSSL_UNPACK_DIR%`) DO SET OPENSSL_UNPACK_DIR_CYG=%%x

IF NOT EXIST "%_TOOLS_DIR%\unpacked" MKDIR "%_TOOLS_DIR%\unpacked"
IF NOT EXIST "%_TOOLS_DIR%\unpacked\OpenSSL" MKDIR "%_TOOLS_DIR%\unpacked\OpenSSL"

bash -c "bunzip2 --stdout %OPENSSL_TAR_CYG% | tar x -C %OPENSSL_UNPACK_DIR_CYG%" >>%THIRDPARTY_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

ECHO Configuring Thirdparty for %BUILDTRIPLE%
ECHO ========== CONFIGURING ==========  >%THIRDPARTY_BUILD_LOG%

REM Generate project files
cd %_TOOLS_DIR%..
python config.py --platform %BUILD_PLATFORM% >>%THIRDPARTY_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

REM Set build project
set BUILD_PROJECT=build-%BUILD_PLATFORM%\livecode\livecode.sln

REM Set MSBUILD Platform
if /I "%ARCH%"=="x86_64" (
    set MSBUILD_PLATFORM=x64
)
if /I "%ARCH%"=="x86" (
    set MSBUILD_PLATFORM=Win32
)

ECHO Building Thirdparty for %BUILDTRIPLE%
ECHO ========== BUILDING ==========  >>%THIRDPARTY_BUILD_LOG%

msbuild %BUILD_PROJECT% /fl /flp:Verbosity=normal /nologo /m:1 /t:thirdparty-prebuilts /p:Configuration=%MODE% /p:Platform=%MSBUILD_PLATFORM% >>%THIRDPARTY_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

ECHO Packaging Thirdparty for %BUILDTRIPLE%
ECHO ========== PACKAGING ==========  >>%THIRDPARTY_BUILD_LOG%

IF NOT EXIST "%THIRDPARTY_BIN%" MKDIR "%THIRDPARTY_BIN%"
IF NOT EXIST "%THIRDPARTY_BIN%\lib"     MKDIR "%THIRDPARTY_BIN%\lib"

SET THIRDPARTY_LIB_DIR=build-%BUILD_PLATFORM%\livecode\%MODE%\lib

SET SKIA_LIBS=skia skia_opt_arm skia_opt_avx skia_opt_hsw skia_opt_none skia_opt_sse2 skia_opt_sse3 skia_opt_sse41 skia_opt_sse42
SET THIRDPARTY_LIBS=cairo ffi gif jpeg mysql pcre png pq sqlite xml xslt z zip %SKIA_LIBS%

SET THIRDPARTY_FILES=

FOR %%L IN (%THIRDPARTY_LIBS%) DO (
	SET THIRDPARTY_LIB=%%L
	SET THIRDPARTY_LIB_FILE=lib!THIRDPARTY_LIB!.lib
	COPY /Y "%THIRDPARTY_LIB_DIR%\!THIRDPARTY_LIB_FILE!" "%THIRDPARTY_BIN%\lib\!THIRDPARTY_LIB_FILE!" >>%THIRDPARTY_BUILD_LOG% 2>>&1
	IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%
	SET THIRDPARTY_FILES=!THIRDPARTY_FILES! lib/!THIRDPARTY_LIB_FILE!
)

REM Build the prebuilt archives

cd "%THIRDPARTY_BIN%"
FOR /F "usebackq tokens=*" %%x IN (`cygpath.exe -u %THIRDPARTY_TAR%`) DO SET THIRDPARTY_TAR_CYG=%%x

ECHO Archiving Thirdparty for %BUILDTRIPLE%
ECHO ========== ARCHIVING ==========  >>%THIRDPARTY_BUILD_LOG%
bash -c "tar --create --file=%THIRDPARTY_TAR_CYG% --transform='flags=r;s|^|%BUILDTRIPLE%/|' %THIRDPARTY_FILES%" >>%THIRDPARTY_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

ECHO Compressing Thirdparty for %BUILDTRIPLE%
ECHO ========== COMPRESSING ==========  >>%THIRDPARTY_BUILD_LOG%
bash -c "bzip2 --force %THIRDPARTY_TAR_CYG%" >>%THIRDPARTY_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%
