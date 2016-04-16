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

bin_dir ?= $(top_srcdir)/_build/mac/Debug

ENGINE ?= $(if $(filter mac,$(guess_platform)),$(bin_dir)/Standalone-Community.app/Contents/MacOS/Standalone-Community,$(bin_dir)/standalone-community)
PROFILE_ENGINE ?= $(bin_dir)/server-community

ENGINE_FLAGS ?= -ui

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
