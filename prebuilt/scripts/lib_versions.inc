:; cur_dir=$( cd $(dirname ${BASH_SOURCE[0]}) ; pwd -P ); OpenSSL_VERSION=$(cat $cur_dir/openssl_version); ICU_VERSION=$(cat $cur_dir/icu_version); ICU_VERSION_MAJOR=$(cat $cur_dir/icu_version_major); Curl_VERSION=$(cat $cur_dir/curl_version); CEF_VERSION=$(cat $cur_dir/cef_version); return $?;

REM # The first line is ignored by Windows cmd, and contains the shell variables setting.
REM # This file is to be sourced in any of the scripts/build-<lib> script,
REM # as well as in fetch-libraries.sh and build-libs.bat
REM # That allows a simple update of both to-build and to-fetch libraries.

FOR /f %%A IN ('type scripts\openssl_version') DO SET OpenSSL_VERSION=%%A
FOR /f %%A IN ('type scripts\icu_version') DO SET ICU_VERSION=%%A
FOR /f %%A IN ('type scripts\icu_version_major') DO SET ICU_VERSION_MAJOR=%%A
FOR /f %%A IN ('type scripts\cef_version') DO SET CEF_VERSION=%%A
FOR /f %%A IN ('type scripts\curl_version') DO SET Curl_VERSION=%%A
