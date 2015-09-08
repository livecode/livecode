REM @ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

REM ############################################################################
REM #
REM #   CONFIGURE VERSIONS AND FOLDERS
REM #

SET _ROOT_DIR=C:\Builds\libraries
SET _INSTALL_DIR=%_ROOT_DIR%\prefix
SET _WINSDK_ROOT=c:\program files\microsoft sdks\windows\v6.1

echo "Will build into %_ROOT_DIR%"
cmd.exe /V:ON /E:ON /C mkdir %_ROOT_DIR%

echo "Will install into %_INSTALL_DIR%"
cmd.exe /V:ON /E:ON /C mkdir %_INSTALL_DIR%

REM # get the drive & path of the folder this script lives in
REM # (note: ends with \ path delimiter)
FOR /F "delims=" %%A IN ("%0") DO SET _TOOLS_DIR=%%~dpA

REM Get the libraries version variables set from scripts/lib_versions.bat
CALL "scripts\lib_versions.bat"
CALL "%_WINSDK_ROOT%\bin\setenv.cmd" /x86 /release /xp

REM ############################################################################
REM #
REM #   BUILD OPENSSL
REM #

SET OPENSSL_TGZ=%_ROOT_DIR%\openssl-%OpenSSL_VERSION%.tar.gz
SET OPENSSL_SRC=%_ROOT_DIR%\openssl-%OpenSSL_VERSION%
SET OPENSSL_CONFIG=no-hw no-idea no-rc5 no-asm enable-static-engine --prefix=%_INSTALL_DIR% VC-WIN32

cd "%_ROOT_DIR%"

if not exist %OPENSSL_TGZ% (
	echo "Fetching openssl-%OPENSSL_VERSION%
	perl -MLWP::Simple -e "getstore('http://www.openssl.org/source/openssl-%OpenSSL_VERSION%.tar.gz', '%OPENSSL_TGZ%')"
)

if not exist %OPENSSL_SRC% (
	echo "Unpacking openssl-%OPENSSL_VERSION%"
	perl -MArchive::Tar -e "$Archive::Tar::FOLLOW_SYMLINK=1;Archive::Tar->extract_archive('%OpenSSL_TGZ%', 1);"
)

cd "%OPENSSL_SRC%"

perl Configure %OPENSSL_CONFIG%
CALL ms\do_ms.bat
SET _MERGE_FILTER="ENGINE_load_(4758cca|aep|atalla|chil|cswift|nuron|padlock|sureware|ubsec)"
%_TOOLS_DIR%Revolution.exe %_TOOLS_DIR%apply_filter.rev %_MERGE_FILTER% ms\libeay32.def > tmp_libeay32.def
move /Y tmp_libeay32.def ms\libeay32.def
nmake -f ms\nt.mak tmp32 out32 inc32\openssl headers lib
rem merge ssl & eay module def files
%_TOOLS_DIR%Revolution.exe %_TOOLS_DIR%merge_dll_def.rev --name REVSECURITY ms\ssleay32.def ms\libeay32.def > revsecurity.def
rem link objs into revsecurity.dll
@ECHO ON
link /nologo /subsystem:console /opt:ref /dll /release /out:revsecurity.dll /def:revsecurity.def tmp32\*.obj ws2_32.lib gdi32.lib advapi32.lib crypt32.lib user32.lib libcmt.lib
IF EXIST revsecurity.manifest mt -nologo -manifest revsecurity.dll.manifest -outputresource:revsecurity.dll;2

IF NOT EXIST "%_INSTALL_DIR%\include" mkdir "%_INSTALL_DIR%\include"
IF NOT EXIST "%_INSTALL_DIR%\lib\win32\i386" mkdir "%_INSTALL_DIR%\lib\win32\i386"

XCOPY /E /Y inc32\* "%_INSTALL_DIR%\include"
COPY /Y revsecurity.dll "%_INSTALL_DIR%\lib\win32\i386"
COPY /Y revsecurity.lib "%_INSTALL_DIR%\lib\win32\i386"
COPY /Y revsecurity.exp "%_INSTALL_DIR%\lib\win32\i386"
COPY /Y out32\libeay32.lib "%_INSTALL_DIR%\lib\win32\i386"
COPY /Y out32\ssleay32.lib "%_INSTALL_DIR%\lib\win32\i386"
COPY /Y revsecurity.def "%_INSTALL_DIR%\lib\win32\i386"

REM Additional copy of SSL libs so Curl can find them
COPY /Y out32\libeay32.lib "%_INSTALL_DIR%\lib"
COPY /Y out32\ssleay32.lib "%_INSTALL_DIR%\lib"

REM ############################################################################
REM #
REM #   BUILD CURL
REM #

SET CURL_TGZ=%_ROOT_DIR%\curl-%CURL_VERSION%.tar.gz
SET CURL_SRC=%_ROOT_DIR%\curl-%CURL_VERSION%
SET CURL_CONFIG=VC=9 WITH_DEVEL="%_INSTALL_DIR%" WITH_SSL=static DEBUG=no GEN_PDB=no RTLIBCFG=static ENABLE_IDN=no

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

# NOTE: Will produce errors because $(PROGRAM_NAME) target in MakefileBuild.vc has
#   unsatisfied dependencies. For now just execute make in 'ignore errors' (/I) mode.
nmake /I /f Makefile.vc MODE=static %CURL_CONFIG%

XCOPY /E /Y ..\builds\libcurl-vc9-x86-release-static-ssl-static-ipv6-sspi\include\* "%_INSTALL_DIR%\include"
COPY /Y ..\builds\libcurl-vc9-x86-release-static-ssl-static-ipv6-sspi\lib\libcurl_a.lib "%_INSTALL_DIR%\lib\win32\i386"

REM ############################################################################
REM #
REM #   BUILD ICU
REM #

CALL "%_TOOLS_DIR%scripts\build-icu.bat"
