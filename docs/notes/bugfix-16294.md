# Improve printing on Linux

The Linux printing commands now use revpdfprinter to generate a PDF to
send through the system printing process.

Among other things, this means that there is now full support for Unicode
text.

Note: To use printing on Linux in standalones you must now make sure you
include revpdfprinter - make sure the appropriate checkbox is hilited in
the standalone builder.
