# This Makefile takes advantage of the fact that the engine's
# "tests/Makefile" already does pretty much everything required for
# running extension tests.  Rather than duplicating it, it can
# just be included with a few tweaks to its configuration.

LCM_EXT_SOURCES ?= $(patsubst ./%,%,$(shell find . -name '*.lcb' | sort))
LCM_TEST_SOURCES ?= $(shell for f in $(LCM_EXT_SOURCES); do echo $$f | grep 'tests/.*\.lcb' | grep -v '_[^/]*.lcb$$'; done)

# Only do livecodescript-based checks
extensions-check: lcb-check lcs-check
.DEFAULT: extensions-check

top_srcdir ?= ..

LCS_TESTS_DIR =

# Things have now been setup enough that the engine's test Makefile
# can perform the tests without any further configuration.
include $(top_srcdir)/tests/Makefile
