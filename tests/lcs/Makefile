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

ENGINE ?= $(bin_dir)/standalone-community
ENGINE_FLAGS ?= -ui

################################################################

.DEFAULT: check

check: livecodescript-check
clean:
	-rm -rf $(TEST_DIR)

.PHONY: check

################################################################

TEST_LOG = _test_suite.log

TEST_RUNNER_SCRIPT = _testrunner.livecodescript

livecodescript-check: $(ENGINE)
	@rm -f $(TEST_LOG)
	@$(ENGINE) $(ENGINE_FLAGS) $(TEST_RUNNER_SCRIPT) run
