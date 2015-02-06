top_srcdir=../..

include $(top_srcdir)/rules/environment.linux.makefile

SOLUTION_DIR = $(top_srcdir)
TEST_DIR = $(SOLUTION_DIR)/_tests/lcb-stdlib

check: lcb-stdlib-check
clean:
	-rm -rf $(TEST_DIR)

.PHONY: all check

################################################################

STDLIB_MLC = $(shell find . -name '*.mlc' | sed -e's:^\./::' | grep -v '^\.')
STDLIB_LCM = $(patsubst %.mlc,$(TEST_DIR)/%.lcm,$(STDLIB_MLC))
STDLIB_LOGS = $(patsubst %.lcm,%.log,$(STDLIB_LCM))

LCC_FLAGS = --modulepath $(TEST_DIR) --modulepath $(MODULE_DIR)

TEST_RUNNER_LCM = $(TEST_DIR)/_testrunner.lcm

lcb-stdlib-check-summarize: $(LC_RUN) $(LC_COMPILE)
	-rm -f $(STDLIB_LCM) _test_suite.log
	mkdir -p $(TEST_DIR)
	@for mlcfile in $(STDLIB_MLC); do \
	    lcmfile=$(TEST_DIR)/`echo $$mlcfile | sed -e's:mlc$$:lcm:'` ; \
	    echo "$(LC_COMPILE) $(LCC_FLAGS) --output $$lcmfile $$mlcfile" \
	        $(_PRINT_RULE); \
	    $(LC_COMPILE) $(LCC_FLAGS) --output $$lcmfile $$mlcfile || exit $$? ; \
	done
	@echo
	@for lcmfile in $(STDLIB_LCM); do \
	    logfile=`echo $$lcmfile | sed -e's:lcm$$:log:'` ; \
	    $(LC_RUN) --handler RunModuleTests $(TEST_RUNNER_LCM) \
	        --lc-run $(LC_RUN) $$lcmfile > $$logfile || exit $$? ; \
	done
	@$(LC_RUN) --handler SummarizeTests $(TEST_RUNNER_LCM) \
	    --summary-log _test_suite.log $(STDLIB_LOGS)

lcb-stdlib-check: lcb-stdlib-check-summarize

.PHONY: lcb-stdlib-check lcb-stdlib-check-summarize
