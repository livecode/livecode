@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

REM ############################################################################
REM #
REM #   BUILD OPENSSL
REM #

SET OPENSSL_TGZ=%_ROOT_DIR%\openssl-%OpenSSL_VERSION%.tar.gz
SET OPENSSL_SRC=%_ROOT_DIR%\openssl-%OpenSSL_VERSION%

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

REM Configure and build OpenSSL
REM We disable hardware crypto engine support and any patented algorithms
REM We build as static (but position-independent) libraries then link manually later
SET OPENSSL_CONFIG=no-hw no-rc5 no-shared --prefix=%_INSTALL_DIR%

IF %ARCH%==x86_64 (
  SET OPENSSL_CONFIG=%OPENSSL_CONFIG% VC-WIN64A
  SET BITS=64
  SET MACHINE=x64
) ELSE (
  SET OPENSSL_CONFIG=%OPENSSL_CONFIG% VC-WIN32
  SET BITS=32
  SET MACHINE=x86
)

perl Configure %OPENSSL_CONFIG%
nmake clean
nmake
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

REM Test the OpenSSL build (just in case)
nmake test
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

REM Generate the .def files for libcrypto and libssl
perl util\mkdef.pl crypto %BITS% > revcrypto.def
perl util\mkdef.pl ssl    %BITS% > revssl.def

REM Merge the definitions files
%_TOOLS_DIR%Revolution.exe %_TOOLS_DIR%merge_dll_def.rev --name REVSECURITY revcrypto.def revssl.def > revsecurity.def

REM Link the revsecurity.dll file
link /nologo /subsystem:console /machine:%MACHINE% /opt:ref /dll /release /out:revsecurity.dll /def:revsecurity.def libcrypto.lib libssl.lib ws2_32.lib gdi32.lib advapi32.lib crypt32.lib user32.lib libcmt.lib
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

IF NOT EXIST "%_INSTALL_DIR%\win32\%ARCH%\include" MKDIR "%_INSTALL_DIR%\win32\%ARCH%\include"
IF NOT EXIST "%_INSTALL_DIR%\win32\%ARCH%\lib"     MKDIR "%_INSTALL_DIR%\win32\%ARCH%\lib"

XCOPY /E /Y /I include\openssl\* "%_INSTALL_DIR%\win32\%ARCH%\include\openssl\"
COPY /Y revsecurity.dll "%_INSTALL_DIR%\win32\%ARCH%\lib"
COPY /Y revsecurity.lib "%_INSTALL_DIR%\win32\%ARCH%\lib"
COPY /Y revsecurity.exp "%_INSTALL_DIR%\win32\%ARCH%\lib"
COPY /Y revsecurity.def "%_INSTALL_DIR%\win32\%ARCH%\lib"

REM Curl statically links against OpenSSL so give it a copy of the static libraries that were built
REM Current versions of Curl still use the old library names for linking against OpenSSL 1.1.0 on Windows
COPY /Y libcrypto.lib "%_INSTALL_DIR%\win32\%ARCH%\lib\libeay32.lib"
COPY /Y libssl.lib    "%_INSTALL_DIR%\win32\%ARCH%\lib\ssleay32.lib"
