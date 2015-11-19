ifeq ($(V),)
_PRINT_RULE ?= > /dev/null
else
_PRINT_RULE ?=
endif

################################################################
# Attempt to guess where to find various tools and libraries
################################################################

top_srcdir ?= ..
TEST_DIR ?= $(top_srcdir)/_tests
LCM_DIR ?= $(TEST_DIR)/_build

guess_linux_arch_script := \
	case `uname -p` in \
	    x86_64) echo x86_64 ;; \
	    x86|i*86) echo x86 ;; \
	esac
guess_linux_arch := $(shell $(guess_linux_arch_script))

guess_platform_script := \
	case `uname -s` in \
	    Linux) echo linux-$(guess_linux_arch) ;; \
	    Darwin) echo mac ;; \
	esac
guess_platform := $(shell $(guess_platform_script))

bin_dir ?= $(top_srcdir)/$(guess_platform)-bin

########## LiveCode Script test parameters

guess_engine := $(if $(filter mac,$(guess_platform)),$(bin_dir)/Standalone-Community.app/Contents/MacOS/Standalone-Community,$(bin_dir)/standalone-community)

# When running on headless Linux, run tests in -ui mode.
guess_engine_flags_script := \
	if echo $(guess_platform) | grep "^linux" >/dev/null 2>&1 && \
	        ! xset -q >/dev/null 2>&1 ; then \
	    echo "-ui"; \
	fi
guess_engine_flags := $(shell $(guess_engine_flags_script))

LCS_ENGINE ?= $(guess_engine)
LCS_ENGINE_FLAGS ?= $(guess_engine_flags)

LCS_LOG = _lcs_test_suite.log
LCS_TESTRUNNER = $(top_srcdir)/tests/_testrunner.livecodescript

########## LiveCode Builder test parameters

MODULE_DIR ?= $(bin_dir)/modules/lci

LC_COMPILE ?= $(bin_dir)/lc-compile
LC_COMPILE_FLAGS += --modulepath $(LCM_DIR) --modulepath $(MODULE_DIR)

LC_RUN ?= $(bin_dir)/lc-run

LCB_LOG = _lcb_test_suite.log

LCB_TESTRUNNER = $(LCM_DIR)/_testrunner.lcm
LCB_TESTLIB = $(LCM_DIR)/_testlib.lcm

################################################################
# Top-level targets
################################################################

.DEFAULT: check

check: lcs-check lcb-check

clean:
	-rm -rf $(TEST_DIR) $(LCS_LOG) $(LCB_LOG)

.PHONY: check clean

################################################################
# LCB test compilation rules
################################################################

LCM_SOURCES = $(patsubst ./%,%,$(shell find . -name '*.lcb' | sort))
LCM_LIB_SOURCES = $(shell for f in $(LCM_SOURCES); do echo $$f | grep '_[^/]*.lcb$$'; done)
LCM_TEST_SOURCES = $(shell for f in $(LCM_SOURCES); do echo $$f | grep -v '_[^/]*.lcb$$'; done)

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
	sed -i 's,\(.* \([^ ]*\)\.lcb\)$$,$(LCM_DIR)/\2.lcm \1,g' $@

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

lcs-check: $(LCS_ENGINE)
	@rm -f $(LCS_LOG)
	@cmd="$(LCS_ENGINE) $(LCS_ENGINE_FLAGS) $(LCS_TESTRUNNER) run"; \
	echo "$$cmd" $(_PRINT_RULE); \
	$$cmd

################################################################
# LCB-only tests
################################################################

LCB_TEST_LOGS = $(patsubst %.lcb,$(TEST_DIR)/%.log,$(LCM_TEST_SOURCES))

LC_RUN_FLAGS += -l $(LCB_TESTLIB)

lcb-check: lcm_compile
	@set -e; \
	for lcbfile in $(LCM_TEST_SOURCES); do \
	    lcmfile="$(LCM_DIR)/`echo $$lcbfile | sed -e 's:lcb$$:lcm:'`" ; \
	    logfile="$(TEST_DIR)/`echo $$lcbfile | sed -e's:lcb$$:log:'`" ; \
	    mkdir -p `dirname $$logfile`; \
	    $(LC_RUN) $(LC_RUN_FLAGS) --handler RunModuleTests -- $(LCB_TESTRUNNER) --lc-run "$(LC_RUN) $(LC_RUN_FLAGS)" $$lcmfile > $$logfile; \
	done; \
	$(LC_RUN) $(LC_RUN_FLAGS) --handler SummarizeTests -- $(LCB_TESTRUNNER) --summary-log $(LCB_LOG) $(LCB_TEST_LOGS)
