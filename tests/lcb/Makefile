top_srcdir=../..

include $(top_srcdir)/rules/environment.linux.makefile

SOLUTION_DIR = $(top_srcdir)
TEST_DIR = $(SOLUTION_DIR)/_tests/lcb

check: lcb-check
clean:
	-rm -rf $(TEST_DIR)

.PHONY: all check

################################################################

STDLIB_LCB = $(shell find . -name '*.lcb' | sed -e's:^\./::' | grep -v '^\.')
STDLIB_LCM = $(patsubst %.lcb,$(TEST_DIR)/%.lcm,$(STDLIB_LCB))
STDLIB_LOGS = $(patsubst %.lcm,%.log,$(STDLIB_LCM))

LCC_FLAGS = --modulepath $(TEST_DIR) --modulepath $(MODULE_DIR)

TEST_RUNNER_LCM = $(TEST_DIR)/_testrunner.lcm

lcb-check: $(LC_RUN) $(LC_COMPILE)
	-rm -f $(STDLIB_LCM) _test_suite.log
	mkdir -p $(TEST_DIR)
	@for lcbfile in $(STDLIB_LCB); do \
	    lcmfile=$(TEST_DIR)/`echo $$lcbfile | sed -e's:lcb$$:lcm:'` ; \
	    echo "$(LC_COMPILE) $(LCC_FLAGS) --output $$lcmfile $$lcbfile" \
	        $(_PRINT_RULE); \
	    $(LC_COMPILE) $(LCC_FLAGS) --output $$lcmfile $$lcbfile || exit $$? ; \
	done
	@echo
	@for lcmfile in $(STDLIB_LCM); do \
	    logfile=`echo $$lcmfile | sed -e's:lcm$$:log:'` ; \
	    $(LC_RUN) --handler RunModuleTests $(TEST_RUNNER_LCM) \
	        --lc-run $(LC_RUN) $$lcmfile > $$logfile || exit $$? ; \
	done
	@$(LC_RUN) --handler SummarizeTests $(TEST_RUNNER_LCM) \
	    --summary-log _test_suite.log $(STDLIB_LOGS)

.PHONY: lcb-check lcb-check-summarize
