@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

REM ############################################################################
REM #
REM #   BUILD CURL
REM #

SET CURL_TGZ=%_ROOT_DIR%\curl-%CURL_VERSION%.tar.gz
SET CURL_SRC=%_ROOT_DIR%\curl-%CURL_VERSION%

cd "%_ROOT_DIR%"

if not exist %CURL_TGZ% (
	echo "Fetching curl-%CURL_VERSION%
	perl -MLWP::Simple -e "getstore('http://curl.haxx.se/download/curl-%CURL_VERSION%.tar.gz', '%CURL_TGZ%')"
)

if not exist %CURL_SRC% (
	echo "Unpacking curl-%CURL_VERSION%"
	perl -MArchive::Tar -e "$Archive::Tar::FOLLOW_SYMLINK=1;Archive::Tar->extract_archive('%CURL_TGZ%', 1);"
)

cd "%CURL_SRC%"

cd winbuild

REM Curl configuration settings
SET CURL_CONFIG=VC=9 WITH_DEVEL="%_INSTALL_DIR%\win32\%ARCH%" WITH_SSL=static DEBUG=no GEN_PDB=no RTLIBCFG=static ENABLE_IDN=no

IF %ARCH%==x86_64 (
  SET SHORT_ARCH=x64
  SET CURL_CONFIG=%CURL_CONFIG% MACHINE=x64
) ELSE (
  SET SHORT_ARCH=x86
  SET CURL_CONFIG=%CURL_CONFIG% MACHINE=x86
)

REM NOTE: Will produce errors because $(PROGRAM_NAME) target in MakefileBuild.vc
REM   has unsatisfied dependencies. For now just execute make in 'ignore errors' (/I) mode.
nmake /F Makefile.vc MODE=static %CURL_CONFIG%
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

XCOPY /E /Y ..\builds\libcurl-vc9-%SHORT_ARCH%-release-static-ssl-static-ipv6-sspi\include\*     "%_INSTALL_DIR%\win32\%ARCH%\include"
COPY /Y ..\builds\libcurl-vc9-%SHORT_ARCH%-release-static-ssl-static-ipv6-sspi\lib\libcurl_a.lib "%_INSTALL_DIR%\win32\%ARCH%\lib"
