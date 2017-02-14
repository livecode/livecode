MODE ?= release
top_srcdir ?= ..

include $(top_srcdir)/Makefile.common

########## LiveCode Script test parameters

TEST_DIR ?= $(top_srcdir)/_tests
LCM_DIR ?= $(TEST_DIR)/_build

# When running on headless Linux, run tests in -ui mode.

LCS_ENGINE ?= $(guess_engine)
LCS_ENGINE_FLAGS ?= $(guess_engine_flags)

LCS_LOG = _lcs_test_suite.log
LCS_TESTRUNNER ?= $(top_srcdir)/tests/_testrunner.livecodescript

ifeq ($(LCS_TESTENGINE),development)
LCS_CMD = $(LCS_ENGINE) $(LCS_ENGINE_FLAGS) $(LCS_TESTRUNNER) run $(guess_dev_engine) $(LCS_ENGINE_FLAGS)
else
LCS_CMD = $(LCS_ENGINE) $(LCS_ENGINE_FLAGS) $(LCS_TESTRUNNER) run
endif

########## LiveCode Builder test parameters

MODULE_DIR ?= $(bin_dir)/modules/lci

LC_COMPILE ?= $(bin_dir)/lc-compile
LC_COMPILE_FLAGS += --modulepath $(LCM_DIR) --modulepath $(MODULE_DIR)

LC_RUN ?= $(bin_dir)/lc-run

LCB_LOG = _lcb_test_suite.log

LCB_TESTRUNNER = $(LCM_DIR)/_testrunner.lcm
LCB_TESTLIB = $(LCM_DIR)/_testlib.lcm

########## LiveCode Builder Compiler test parameters

LCC_COMPILERTESTRUNNER ?= $(top_srcdir)/tests/_compilertestrunner.livecodescript

LCC_CMD = $(LCS_ENGINE) -ui $(LCC_COMPILERTESTRUNNER) run

LCC_LOG = _compiler_test_suite.log

########## lc-run test params

LC_RUN_TESTS ?= $(top_srcdir)/tests/_lcruntests.livecodescript
LC_RUN_TEST_CMD = $(LCS_ENGINE) -ui $(LC_RUN_TESTS)

LC_RUN_LOG = _lc_run_test_suite.log

########## lc-compile-ffi-java test parameters

LC_COMPILE_FFI_JAVA ?= $(bin_dir)/lc-compile-ffi-java

LC_COMPILE_FFI_JAVA_TESTRUNNER ?= $(top_srcdir)/tests/_lc_compile_ffi_java_testrunner.livecodescript

LC_COMPILE_FFI_JAVA_CMD = $(LCS_ENGINE) -ui $(LC_COMPILE_FFI_JAVA_TESTRUNNER) run

LC_COMPILE_FFI_JAVA_LOG = _lc-compile-ffi-java_test_suite.log

################################################################
# Top-level targets
################################################################

.DEFAULT: check

check: lcs-check lcb-check compiler-check lc-compile-ffi-java-check

clean:
	-rm -rf $(TEST_DIR) $(LCS_LOG) $(LCB_LOG)

.PHONY: check clean

################################################################
# LCB test compilation rules
################################################################

LCM_SOURCES ?= $(patsubst ./%,%,$(shell find . -name '*.lcb' | sort))
LCM_LIB_SOURCES = $(shell for f in $(LCM_SOURCES); do echo $$f | grep '_[^/]*.lcb$$'; done)

LCM_TEST_SOURCES ?= $(shell for f in $(LCM_SOURCES); do echo $$f | grep -v '_[^/]*.lcb$$'; done)

LCM_MODULES = $(patsubst %.lcb,$(LCM_DIR)/%.lcm,$(LCM_SOURCES))
LCM_TEST_MODULES = $(patsubst %.lcb,$(LCM_DIR)/%.lcm,$(LCM_TEST_SOURCES))

########## Build dependencies rules

ifneq ($(LCM_SOURCES),)
	include $(LCM_DIR)/deps.mk
endif

$(LCM_DIR):
	mkdir -p $(LCM_DIR)

# We need to rewrite some of the rules to make sure that lcm files
# inherit the correct dependencies.  Specifically, we need to make
# sure that .lcm files depend on all the same things that their
# corresponding .lci files depend on.
$(LCM_DIR)/deps.mk: $(LCM_SOURCES) Makefile $(LC_COMPILE) | $(LCM_DIR)
	@set -e; rm -f $@; \
	for lcb in $(LCM_SOURCES); do \
	  lcm=`echo $$lcb | sed 's/lcb$$/lcm/'` ; \
	  $(LC_COMPILE) $(LC_COMPILE_FLAGS) --deps make \
	    --output $(LCM_DIR)/$$lcm -- $$lcb >> $@; \
	done; \
	$(LC_COMPILE) $(LC_COMPILE_FLAGS) --deps make -- $(LCM_SOURCES) >> $@ ; \
	sed -i -e 's,\(.* \([^ ]*\)\.lcb\)$$,$(LCM_DIR)/\2.lcm \1,g' $@

########## Build rules

lcm_compile: $(LCM_MODULES)

$(LCM_DIR)/%.lcm: %.lcb | $(LCM_DIR)
	@set -e; \
	cmd="mkdir -p $$(dirname $@)" ; \
	echo "$$cmd" $(_PRINT_RULE); \
	$$cmd
	@set -e; \
	cmd="$(LC_COMPILE) $(LC_COMPILE_FLAGS) --output $@ -- $<" ; \
	echo "$$cmd" $(_PRINT_RULE); \
	$$cmd

.PHONY: lcm_compile

################################################################
# Engine-based tests
################################################################

lcs-check: $(LCS_ENGINE) lcm_compile
	@rm -f $(LCS_LOG)
	@export TEST_SUITE_LOG_FILE="$(LCS_LOG)"; \
	cmd="$(LCS_CMD)"; \
	echo "$$cmd" $(_PRINT_RULE); \
	$$cmd

.PHONY: lcs-check

################################################################
# LCB compiler tests
################################################################

compiler-check: $(LC_COMPILE)
	@rm -f $(LCC_LOG)
	@export LC_COMPILE="$(LC_COMPILE)"; \
	export LCC_VERBOSE="$(LCC_VERBOSE)"; \
	export TEST_SUITE_LOG_FILE="$(LCC_LOG)"; \
	cmd="$(LCC_CMD)"; \
	echo "$$cmd" $(_PRINT_RULE); \
	$$cmd

################################################################
# Basic LCB runner test
################################################################

lc-run-check: $(LC_RUN) lcm_compile
	@export LC_RUN="$(LC_RUN)"; \
	export LCM_DIR="$(LCM_DIR)/lc-run"; \
	export TEST_SUITE_LOG_FILE="$(LC_RUN_LOG)"; \
	cmd="$(LC_RUN_TEST_CMD)"; \
	echo "$$cmd" $(_PRINT_RULE); \
	$$cmd

.PHONY: lc-run-check

################################################################
# LCB-only tests
################################################################

LCB_TEST_LOGS = $(patsubst %.lcb,$(TEST_DIR)/%.log,$(LCM_TEST_SOURCES))

LC_RUN_FLAGS += -l $(LCB_TESTLIB)

lcb-check: lcm_compile
	@set -e; \
	export TEST_SUITE_LOG_FILE="$(LCB_LOG)"; \
	for lcbfile in $(LCM_TEST_SOURCES); do \
	    lcmfile="$(LCM_DIR)/`echo $$lcbfile | sed -e 's:lcb$$:lcm:'`" ; \
	    logfile="$(TEST_DIR)/`echo $$lcbfile | sed -e's:lcb$$:log:'`" ; \
	    parent="`echo $$lcbfile | sed -e 's:.*/\([^/]*\)/tests.*$$:\1:'`"; \
	    loadfile="$(LCM_DIR)/`echo $$lcbfile | sed -e 's:tests/.*\.lcb$$::'`$$parent.lcm"; \
	    mkdir -p `dirname $$logfile`; \
	    if [ -e $$loadfile ] ; then \
	    	$(LC_RUN) $(LC_RUN_FLAGS) --handler RunModuleTests -- $(LCB_TESTRUNNER) --lc-run "$(LC_RUN) --load $$loadfile $(LC_RUN_FLAGS)" $$lcmfile > $$logfile; \
	    else \
	    	$(LC_RUN) $(LC_RUN_FLAGS) --handler RunModuleTests -- $(LCB_TESTRUNNER) --lc-run "$(LC_RUN) $(LC_RUN_FLAGS)" $$lcmfile > $$logfile; \
	    fi; \
	done; \
	$(LC_RUN) $(LC_RUN_FLAGS) --handler SummarizeTests -- $(LCB_TESTRUNNER) --summary-log $(LCB_LOG) $(LCB_TEST_LOGS)

################################################################
# lc-compile-ffi-java tests
################################################################

lc-compile-ffi-java-check: $(LC_COMPILE_FFI_JAVA)
	@rm -f $(LC_COMPILE_FFI_JAVA_LOG)
	@export LC_COMPILE_FFI_JAVA="$(LC_COMPILE_FFI_JAVA)"; \
	export LCC_VERBOSE="$(LC_COMPILE_FFI_JAVA_VERBOSE)"; \
	export TEST_SUITE_LOG_FILE="$(LC_COMPILE_FFI_JAVA_LOG)"; \
	cmd="$(LC_COMPILE_FFI_JAVA_CMD)"; \
	echo "$$cmd" $(_PRINT_RULE); \
	$$cmd
