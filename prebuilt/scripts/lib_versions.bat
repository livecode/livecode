REM # This file sets the libraries versions defined in versions/

FOR /f %%A IN ('type versions\openssl') DO SET OpenSSL_VERSION=%%A
FOR /f %%A IN ('type versions\icu') DO SET ICU_VERSION=%%A
FOR /f %%A IN ('type versions\icu_major') DO SET ICU_VERSION_MAJOR=%%A
FOR /f %%A IN ('type versions\cef') DO SET CEF_VERSION=%%A
FOR /f %%A IN ('type versions\curl') DO SET Curl_VERSION=%%A
