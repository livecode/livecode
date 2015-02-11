@echo off
REM This file requires that the execution environment is set up as follows:
REM	Cygwin is in PATH
REM	MSVC is in PATH
REM	INCLUDE and LIB point to the Windows SDK
REM
REM It is also assumed that the ICU source has been unpacked in the current directory
REM (i.e. ./icu/source/runConfigureICU is the path to the configure script)

mkdir %_ROOT_DIR%\build-icu
mkdir %_ROOT_DIR%\install-icu
cd %_ROOT_DIR%\build-icu

REM --with-data-packaging=archive
bash ../../icu/source/runConfigureICU Cygwin/MSVC --prefix=/ --with-data-packaging=static --enable-static --disable-samples --disable-tests

REM Unfortunately, the following defines do not work properly for Win32 builds but
REM are used by the build scripts for the other platforms. This shouldn't make
REM anything break for Win32 but it makes it a bit less strict for some checks
REM
REM	-DU_USING_ICU_NAMESPACE=0
REM	-DUNISTR_FROM_CHAR_EXPLICIT=explicit
REM	-DUNISTR_FROM_STRING_EXPLICIT=explicit

REM Do the build
bash -c "make"

REM Build the minimal ICU data lib required by the installer
REM bash -c "bin/pkgdata.exe --bldopt data/icupkg.inc --quiet --copyright --sourcedir data/out/build/icudt52l --destdir ./lib --entrypoint icudt52 --tempdir data/out/tmp --name icudt52l-minimal --mode static --revision 52 --libname icudt-minimal ../../minimal-data.lst"

REM Install the files to the staging location
bash -c "make DESTDIR=%_ROOT_DIR%\install-icu install"

