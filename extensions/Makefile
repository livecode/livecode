# This Makefile takes advantage of the fact that the engine's
# "tests/Makefile" already does pretty much everything required for
# running extension tests.  Rather than duplicating it, it can
# just be included with a few tweaks to its configuration.

# Only do livecodescript-based checks
extensions-check: lcs-check
.DEFAULT: extensions-check

top_srcdir ?= ..
LCM_SOURCES ?=

# Things have now been setup enough that the engine's test Makefile
# can perform the tests without any further configuration.
include $(top_srcdir)/tests/Makefile
