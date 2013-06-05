REM @ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

REM ############################################################################
REM #
REM #   CONFIGURE VERSIONS AND FOLDERS
REM #

SET _ROOT_DIR=C:\Builds\libraries
SET _INSTALL_DIR=%_ROOT_DIR%\prefix
SET _WINSDK_ROOT=c:\program files\microsoft sdks\windows\v6.1

SET OPENSSL_VERSION=1.0.0e

echo "Will build into %_ROOT_DIR%"
cmd.exe /V:ON /E:ON /C mkdir %_ROOT_DIR%

echo "Will install into %_INSTALL_DIR%"
cmd.exe /V:ON /E:ON /C mkdir %_INSTALL_DIR%

REM # get the drive & path of the folder this script lives in
REM # (note: ends with \ path delimiter)
FOR /F "delims=" %%A IN ("%0") DO SET _TOOLS_DIR=%%~dpA

CALL "%_WINSDK_ROOT%\bin\setenv.cmd" /x86 /release /xp

REM ############################################################################
REM #
REM #   BUILD OPENSSL
REM #

SET OPENSSL_TGZ=%_ROOT_DIR%\openssl-%OPENSSL_VERSION%.tar.gz
SET OPENSSL_SRC=%_ROOT_DIR%\openssl-%OPENSSL_VERSION%
SET OPENSSL_CONFIG=no-hw no-idea no-rc5 no-asm enable-static-engine --prefix=%_INSTALL_DIR% VC-WIN32

cd "%_ROOT_DIR%"

if not exist %OPENSSL_TGZ% (
	echo "Fetching openssl-%OPENSSL_VERSION%
	perl -MLWP::Simple -e "getstore('http://www.openssl.org/source/openssl-%OPENSSL_VERSION%.tar.gz', '%OPENSSL_TGZ%')"
)

if not exist %OPENSSL_SRC% (
	echo "Unpacking openssl-%OPENSSL_VERSION%"
	perl -MArchive::Tar -e "$Archive::Tar::FOLLOW_SYMLINK=1;Archive::Tar->extract_archive('%OPENSSL_TGZ%', 1);"
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
IF NOT EXIST "%_INSTALL_DIR%\lib" mkdir "%_INSTALL_DIR%\lib"

XCOPY /E /Y inc32\* "%_INSTALL_DIR%\include"
COPY /Y revsecurity.dll "%_INSTALL_DIR%\lib"
COPY /Y revsecurity.lib "%_INSTALL_DIR%\lib"
COPY /Y revsecurity.exp "%_INSTALL_DIR%\lib"
COPY /Y out32\libeay32.lib "%_INSTALL_DIR%\lib"
COPY /Y out32\ssleay32.lib "%_INSTALL_DIR%\lib"
