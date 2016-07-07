ifeq ($(V),)
_PRINT_RULE ?= > /dev/null
else
_PRINT_RULE ?=
endif

################################################################
# Attempt to guess where to find various tools and libraries
################################################################

MODE ?= release
ifeq ($(MODE),release)
	UC_MODE := Release
else ifeq ($(MODE),debug)
	UC_MODE := Debug
else ifeq ($(MODE),fast)
	UC_MODE := Fast
else
	$(error "Mode must be 'debug' or 'release'")
endif

top_srcdir ?= ..

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

ifeq ($(guess_platform),mac)
	bin_dir ?= $(top_srcdir)/_build/mac/$(UC_MODE)
else
	bin_dir ?= $(top_srcdir)/$(guess_platform)-bin
endif

########## LiveCode Script test parameters

guess_engine := $(if $(filter mac,$(guess_platform)),$(bin_dir)/Standalone-Community.app/Contents/MacOS/Standalone-Community,$(bin_dir)/standalone-community)

# When running on headless Linux, run lints in -ui mode.
guess_engine_flags_script := \
	if echo $(guess_platform) | grep "^linux" >/dev/null 2>&1 && \
	        ! xset -q >/dev/null 2>&1 ; then \
	    echo "-ui"; \
	fi
guess_engine_flags := $(shell $(guess_engine_flags_script))

ENGINE ?= $(guess_engine)
ENGINE_FLAGS ?= $(guess_engine_flags)

LINTER ?= $(top_srcdir)/lints/LintCLI.livecodescript

LINT_CMD = $(ENGINE) $(ENGINE_FLAGS) $(LINTER) $(top_srcdir)

################################################################
# Top-level targets
################################################################

.DEFAULT: check

check:
	@cmd="$(LINT_CMD)"; \
	echo "$$cmd" $(_PRINT_RULE); \
	$$cmd

clean:
	true

.PHONY: check clean
