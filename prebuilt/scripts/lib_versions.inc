# This file set the libraries version variables to the version defined in versions/

script_dir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

OpenSSL_VERSION=$(cat "${script_dir}/../versions/openssl")
ICU_VERSION=$(cat "${script_dir}/../versions/icu")
ICU_VERSION_MAJOR=$(cat "${script_dir}/../versions/icu_major")
Curl_VERSION=$(cat "${script_dir}/../versions/curl")
CEF_VERSION=$(cat "${script_dir}/../versions/cef")
