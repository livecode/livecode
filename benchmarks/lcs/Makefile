top_srcdir ?= ../..

################################################################
# Attempt to guess where the files we need are
################################################################

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

# When running on headless Linux, run tests in -ui mode.
guess_flags_script := \
	if echo $(guess_platform) | grep "^linux" >/dev/null 2>&1 && \
	        ! xset -q >/dev/null 2>&1 ; then \
	    echo "-ui"; \
	fi
guess_flags := $(shell $(guess_flags_script))

bin_dir ?= $(top_srcdir)/_build/mac/Debug

ENGINE ?= $(if $(filter mac,$(guess_platform)),$(bin_dir)/Standalone-Community.app/Contents/MacOS/Standalone-Community,$(bin_dir)/standalone-community)
PROFILE_ENGINE ?= $(binder)/server-community

ENGINE_FLAGS ?= $(guess_flags)

################################################################

.DEFAULT: bench

bench: livecodescript-bench
clean:
	#-rm -rf $(TEST_DIR)

.PHONY: check

################################################################

BENCH_LOG = _benchmark_suite.log

BENCH_RUNNER_SCRIPT = _benchrunner.livecodescript

livecodescript-bench: $(ENGINE) $(PROFILE_ENGINE)
	@rm -f $(BENCH_LOG)
	$(ENGINE) $(ENGINE_FLAGS) $(BENCH_RUNNER_SCRIPT) run
