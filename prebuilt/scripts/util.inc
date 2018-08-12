# Common script functions

# Download url to file
function fetchUrl {
	URL=$1
	SAVEPATH=$2

	# Download using an HTTP client of some variety
	if $(which curl 1>/dev/null 2>/dev/null) ; then
		curl --fail -L "${URL}" -o "${SAVEPATH}"
	elif $(which wget 1>/dev/null 2>/dev/null) ; then
		wget "${URL}" -O "${SAVEPATH}"
	else
		# Perl as a last resort (useful for Cygwin)
		perl -MLWP::Simple -e "getstore('${URL}', '${SAVEPATH}') == 200 or exit 1"
	fi
}
