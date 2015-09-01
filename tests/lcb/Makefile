top_srcdir ?= ../..
TEST_DIR ?= $(top_srcdir)/_tests/lcb

################################################################
# Attempt to guess where to find the LCB toolchain & LCI files
################################################################

guess_linux_arch_script := \
	case `uname -p` in \
	    x86_64) echo x86_64 ;; \
	    x86|i*85) echo x86 ;; \
	esac
guess_linux_arch := $(shell $(guess_linux_arch_script))

guess_platform_script := \
	case `uname -s` in \
	    Linux) echo linux-$(guess_linux_arch) ;; \
	    Darwin) echo mac ;; \
	esac
guess_platform := $(shell $(guess_platform_script))

bin_dir ?= $(top_srcdir)/$(guess_platform)-bin

LC_COMPILE ?= $(bin_dir)/lc-compile
LC_RUN ?= $(bin_dir)/lc-run
MODULE_DIR ?= $(bin_dir)/modules/lci

################################################################

.DEFAULT: check

check: lcb-check
clean:
	-rm -rf $(TEST_DIR)

.PHONY: all check

################################################################

TESTLIB_LCB = _testrunner.lcb _testlib.lcb
TESTLIB_LCM = $(patsubst %.lcb,$(TEST_DIR)/%.lcm,$(TESTLIB_LCB))

TEST_LCB = $(shell find . -name '*.lcb' | sed -e's:^\./::' | grep -v '[._][^/]*.lcb$$' | sort)
TEST_LCM = $(patsubst %.lcb,$(TEST_DIR)/%.lcm,$(TEST_LCB))
TEST_LOGS = $(patsubst %.lcm,%.log,$(TEST_LCM))

LC_COMPILE_FLAGS = --modulepath $(TEST_DIR) --modulepath $(MODULE_DIR)
LC_RUN_FLAGS = -l $(TEST_DIR)/_testlib.lcm

TEST_RUNNER_LCM = $(TEST_DIR)/_testrunner.lcm
TEST_LOG = _test_suite.log

lcb-check: $(LC_RUN) $(LC_COMPILE)
	-rm -f $(TESTLIB_LCM) $(TEST_LCM) $(TEST_LOG)
	mkdir -p $(TEST_DIR)
	@for lcbfile in $(TESTLIB_LCB) $(TEST_LCB); do \
	    lcmfile=$(TEST_DIR)/`echo $$lcbfile | sed -e's:lcb$$:lcm:'` ; \
	    mkdir -p `dirname $$lcmfile`; \
	    cmd="$(LC_COMPILE) $(LC_COMPILE_FLAGS) --output $$lcmfile $$lcbfile" ; \
	    echo "$$cmd" $(_PRINT_RULE); \
	    $$cmd || exit $$? ; \
	done
	@echo
	@for lcmfile in $(TEST_LCM); do \
	    logfile=`echo $$lcmfile | sed -e's:lcm$$:log:'` ; \
	    $(LC_RUN) $(LC_RUN_FLAGS) --handler RunModuleTests -- \
	        $(TEST_RUNNER_LCM) --lc-run "$(LC_RUN) $(LC_RUN_FLAGS)" \
	        $$lcmfile > $$logfile || exit $$? ; \
	done
	@$(LC_RUN) $(LC_RUN_FLAGS) --handler SummarizeTests -- \
	    $(TEST_RUNNER_LCM) --summary-log $(TEST_LOG) $(TEST_LOGS)

.PHONY: lcb-check lcb-check-summarize
