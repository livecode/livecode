@ECHO OFF
REM This file requires that the execution environment is set up as follows:
REM	Cygwin is in PATH
REM	MSVC is in PATH
REM	INCLUDE and LIB point to the Windows SDK

ECHO Build ICU for %BUILDTRIPLE%

SET ICU_VERSION_ALT=%ICU_VERSION:.=_%
SET ICU_VERSION_MINOR=%ICU_VERSION:*.=%
CALL SET ICU_VERSION_MAJOR=%%ICU_VERSION:.%ICU_VERSION_MINOR%=%%

SET ICU_TGZ=%_ROOT_DIR%\icu4c-%ICU_VERSION_ALT%-src.tgz
SET ICU_SRC=%_ROOT_DIR%\icu-%ICU_VERSION%-%BUILDTRIPLE%-src
SET ICU_TMP=%_ROOT_DIR%\icu-%ICU_VERSION%-%BUILDTRIPLE%-tmp
SET ICU_BIN=%_ROOT_DIR%\icu-%ICU_VERSION%-%BUILDTRIPLE%-bin
SET ICU_BUILD_LOG=%_ROOT_DIR%\icu-%ICU_VERSION%-%BUILDTRIPLE%.log

IF DEFINED ICU_BUILDREVISION (
	SET ICU_TAR=%_PACKAGE_DIR%\ICU-%ICU_VERSION%-%BUILDTRIPLE%-%ICU_BUILDREVISION%.tar
) ELSE (
	SET ICU_TAR=%_PACKAGE_DIR%\ICU-%ICU_VERSION%-%BUILDTRIPLE%.tar
)

FOR /F "usebackq tokens=*" %%x IN (`cygpath.exe -u %ICU_SRC%`) DO SET ICU_SRC_CYG=%%x
FOR /F "usebackq tokens=*" %%x IN (`cygpath.exe -u %ICU_BIN%`) DO SET ICU_BIN_CYG=%%x
FOR /F "usebackq tokens=*" %%x IN (`cygpath.exe -u %ICU_TAR%`) DO SET ICU_TAR_CYG=%%x

cd "%_ROOT_DIR%

IF NOT EXIST %ICU_TGZ% (
	echo Fetching icu-%ICU_VERSION% for %BUILDTRIPLE%
	perl -MLWP::Simple -e "getstore('https://downloads.sourceforge.net/project/icu/ICU4C/%ICU_VERSION%/icu4c-%ICU_VERSION_ALT%-src.tgz', '%ICU_TGZ%')"
	IF NOT EXIST %ICU_TGZ% (
		ECHO "Failed to download https://downloads.sourceforge.net/project/icu/ICU4C/%ICU_VERSION%/icu4c-%ICU_VERSION_ALT%-src.tgz
		EXIT /B 1
	)
)

if not exist %ICU_SRC% (
	echo Unpacking icu-%ICU_VERSION% for %BUILDTRIPLE%
	perl -MArchive::Tar -e "$Archive::Tar::FOLLOW_SYMLINK=1;Archive::Tar->extract_archive('%ICU_TGZ%', 1);"
	REN icu icu-%ICU_VERSION%-%BUILDTRIPLE%-src
)

IF "%1"=="prepare" (
	EXIT /B 0
)

ECHO Configuring ICU for %BUILDTRIPLE%
ECHO ========== CONFIGURING ==========  >%ICU_BUILD_LOG%
IF NOT EXIST "%ICU_TMP%" mkdir %ICU_TMP%

cd %ICU_TMP%
SET ICU_CONFIG=--prefix=%ICU_BIN_CYG% --with-data-packaging=archive --enable-static --disable-shared --disable-samples --disable-tests --disable-extras
SET ICU_CFLAGS=-DU_USING_ICU_NAMESPACE=0\ -DUNISTR_FROM_CHAR_EXPLICIT=explicit\ -DUNISTR_FROM_STRING_EXPLICIT=explicit

IF %ARCH%==x86_64 (
  SET ICU_CONFIG=%ICU_CONFIG% --with-library-bits=64
  SET BITS=64
  SET MACHINE=x64
) ELSE (
  SET ICU_CONFIG=%ICU_CONFIG% --with-library-bits=32
  SET BITS=32
  SET MACHINE=x86
)

IF %MODE%==debug (
	SET ICU_CONFIG=--enable-debug --disable-release %ICU_CONFIG%
	SET ENV_VARS=CPP= CC=cl CXX=cl CPPFLAGS= CFLAGS=-Zi\ -MTd\ %ICU_CFLAGS% CXXFLAGS=-Zi\ -MTd\ %ICU_CFLAGS%  LDFLAGS=-DEBUG 
) ELSE (
	SET ICU_CONFIG=%ICU_CONFIG%
	SET ENV_VARS=CPP= CC=cl CXX=cl CPPFLAGS= CFLAGS=-Zi\ -MT\ %ICU_CFLAGS%  CXXFLAGS=-Zi\ -MT\ %ICU_CFLAGS%  LDFLAGS=
)

bash -c "%ENV_VARS% ../icu-%ICU_VERSION%-%BUILDTRIPLE%-src/source/configure %ICU_CONFIG%" >>%ICU_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

ECHO Building ICU for %BUILDTRIPLE%
ECHO ========== BUILDING ==========  >>%ICU_BUILD_LOG%
bash -c "%ENV_VARS% make" >>%ICU_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

REM Build the minimal ICU data lib required by the installer
REM bash -c "bin/pkgdata.exe --bldopt data/icupkg.inc --quiet --copyright --sourcedir data/out/build/icudt52l --destdir ./lib --entrypoint icudt52 --tempdir data/out/tmp --name icudt52l-minimal --mode static --revision 52 --libname icudt-minimal ../../minimal-data.lst"

ECHO Packaging ICU for %BUILDTRIPLE%
ECHO ========== PACKAGING ==========  >>%ICU_BUILD_LOG%
bash -c "make DESTDIR= install" >>%ICU_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

ECHO Archiving ICU for %BUILDTRIPLE%
ECHO ========== ARCHIVING ==========  >>%ICU_BUILD_LOG%

cd "%ICU_BIN%"

IF %MODE%==debug (
	SET MODE_SUFFIX=d
) ELSE (
	set MODE_SUFFIX=
)

ECHO copy "share\icu%MODE_SUFFIX%\%ICU_VERSION%\icudt%ICU_VERSION_MAJOR%l.dat" "share\icudt%ICU_VERSION_MAJOR%l.dat" >>%ICU_BUILD_LOG%
copy "share\icu%MODE_SUFFIX%\%ICU_VERSION%\icudt%ICU_VERSION_MAJOR%l.dat" "share\icudt%ICU_VERSION_MAJOR%l.dat" >>%ICU_BUILD_LOG% 2>>&1

SET ICU_FILES=lib/sicudt%MODE_SUFFIX%.lib lib/sicuin%MODE_SUFFIX%.lib lib/sicuio%MODE_SUFFIX%.lib lib/sicutu%MODE_SUFFIX%.lib lib/sicuuc%MODE_SUFFIX%.lib
SET ICU_FILES=%ICU_FILES% include share/icudt%ICU_VERSION_MAJOR%l.dat bin/icupkg.exe bin/pkgdata.exe

bash -c "tar --create --file=%ICU_TAR_CYG% --transform='flags=r;s|^|%BUILDTRIPLE%/|' --transform='flags=r;s|d.lib$|.lib|' %ICU_FILES%" >>%ICU_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%

ECHO Compressing ICU for %BUILDTRIPLE%
ECHO ========== COMPRESSING ==========  >>%ICU_BUILD_LOG%
bash -c "bzip2 --force %ICU_TAR_CYG%" >>%ICU_BUILD_LOG% 2>>&1
IF %ERRORLEVEL% NEQ 0 EXIT /B %ERRORLEVEL%
