@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

REM ############################################################################
REM #
REM #   BUILD CEF
REM #

ECHO Build CEF-%CEF_VERSION%.%CEF_BUILDREVISION% for %BUILDTRIPLE%

IF %ARCH%==x86_64 (
	SET CEF_SRC_NAME=cef_binary_%CEF_VERSION%%%2B%CEF_BUILDREVISION%%%2Bchromium-%CEFChromium_VERSION%_windows64
	SET CEF_DST_NAME=cef_binary_%CEF_VERSION%+%CEF_BUILDREVISION%+chromium-%CEFChromium_VERSION%_windows64
) ELSE (
	SET CEF_SRC_NAME=cef_binary_%CEF_VERSION%%%2B%CEF_BUILDREVISION%%%2Bchromium-%CEFChromium_VERSION%_windows32
	SET CEF_DST_NAME=cef_binary_%CEF_VERSION%+%CEF_BUILDREVISION%+chromium-%CEFChromium_VERSION%_windows32
)

SET CEF_TGZ=%_ROOT_DIR%\%CEF_DST_NAME%.tar.bz2
SET CEF_SRC=%_ROOT_DIR%\%CEF_DST_NAME%
SET CEF_BIN=%_ROOT_DIR%\cef-%CEF_VERSION%.%CEF_BUILDREVISION%-%BUILDTRIPLE%-bin
SET CEF_BUILD_LOG=%_ROOT_DIR%\cef-%CEF_VERSION%.%CEF_BUILDREVISION%-%BUILDTRIPLE%.log

SET CEF_TAR=%_PACKAGE_DIR%\CEF-%CEF_VERSION%-%BUILDTRIPLE%-%CEF_BUILDREVISION%.tar

cd "%_ROOT_DIR%"

if not exist %CEF_TGZ% (
	echo Fetching CEF-%CEF_VERSION% for %BUILDTRIPLE%
	perl -MLWP::Simple -e "getstore('http://opensource.spotify.com/cefbuilds/%CEF_SRC_NAME%.tar.bz2', '%CEF_TGZ%')"
)

if not exist %CEF_TGZ% (
	echo Failed to download http://opensource.spotify.com/cefbuilds/%CEF_SRC_NAME%.tar.bz2 to %CEF_TGZ%
	EXIT /B 1
)

if not exist %CEF_SRC% (
	echo Unpacking CEF-%CEF_VERSION%.%CEF_BUILDREVISION% for %BUILDTRIPLE%
	bash -c "tar -jxf %CEF_DST_NAME%.tar.bz2"
)

ECHO Packaging CEF-%CEF_VERSION%.%CEF_BUILDREVISION% for %BUILDTRIPLE%
IF NOT EXIST "%CEF_BIN%\lib\CEF"     MKDIR "%CEF_BIN%\lib\CEF"
XCOPY /E /Y %CEF_SRC%\Resources\*     "%CEF_BIN%\lib\CEF" >>%CEF_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%
IF %MODE%==debug (
	XCOPY /E /Y %CEF_SRC%\Debug\*     "%CEF_BIN%\lib\CEF" >>%CEF_BUILD_LOG% 2>>&1
) ELSE (
	XCOPY /E /Y %CEF_SRC%\Release\*     "%CEF_BIN%\lib\CEF" >>%CEF_BUILD_LOG% 2>>&1
)
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

REM Build the prebuilt CEF-%CEF_VERSION%.%CEF_BUILDREVISION% archives for %BUILDTRIPLE%

cd "%CEF_BIN%"
FOR /F "usebackq tokens=*" %%x IN (`cygpath.exe -u %CEF_TAR%`) DO SET CEF_TAR_CYG=%%x

ECHO Archiving CEF-%CEF_VERSION%.%CEF_BUILDREVISION% for %BUILDTRIPLE%
ECHO ========== ARCHIVING ==========  >>%CEF_BUILD_LOG%
bash -c "tar --create --file=%CEF_TAR_CYG% --transform='flags=r;s|^|%BUILDTRIPLE%/|' lib" >>%CEF_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

ECHO Compressing CEF-%CEF_VERSION%.%CEF_BUILDREVISION% for %BUILDTRIPLE%
ECHO ========== COMPRESSING ==========  >>%CEF_BUILD_LOG%
bash -c "bzip2 --force %CEF_TAR_CYG%" >>%CEF_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%
