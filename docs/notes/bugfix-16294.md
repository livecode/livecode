---
version: 8.0.0-dp-11
---
# Improve printing on Linux

The Linux printing commands now use the revpdfprinter external to
generate a PDF to send through the system printing process.

Among other things, this means that there is now full support for Unicode
text.

**Note:** To use printing on Linux in standalones you must now make
sure you include revpdfprinter.  Make sure the appropriate checkbox is
highlighted in the standalone builder.
