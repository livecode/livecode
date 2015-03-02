top_srcdir=../..

include $(top_srcdir)/rules/environment.linux.makefile

SOLUTION_DIR = $(top_srcdir)

LC_COMPILE ?= $(shell PATH=$(BUILD_DIR):$(PATH) \
	              which lc-compile 2>/dev/null || \
	              echo "lc-compile")
LC_RUN ?= $(shell PATH=$(BUILD_DIR):$(PATH) \
	              which lc-run 2>/dev/null || \
	              echo "lc-run")

TEST_DIR = $(SOLUTION_DIR)/_tests/lc-run

# Basic functionality tests for lc-run
#
# 1) Does lc-run run a trivial program?
# 2) Does lc-run list the handlers of a trivial program?
check:
	mkdir -p $(TEST_DIR)
	$(LC_COMPILE) --modulepath $(TEST_DIR) --modulepath $(MODULE_DIR) \
	    lc-run-lib.mlc --output $(TEST_DIR)/lc-run-lib.lcm
	$(LC_COMPILE) --modulepath $(TEST_DIR) --modulepath $(MODULE_DIR) \
	    lc-run-test.mlc --output $(TEST_DIR)/lc-run-test.lcm
	$(LC_RUN) -l $(TEST_DIR)/lc-run-lib.lcm $(TEST_DIR)/lc-run-test.lcm
	test "`$(LC_RUN) -l $(TEST_DIR)/lc-run-lib.lcm --list-handlers $(TEST_DIR)/lc-run-test.lcm`" = "main"
