MODE ?= debug
top_srcdir ?= ..

include $(top_srcdir)/Makefile.common

########## LiveCode Script benchmark parameters

LCS_ENGINE ?= $(guess_engine)
LCS_ENGINE_FLAGS ?= $(guess_engine_flags)

LCS_LOG = _lcs_benchmarks.log
LCS_BENCHMARKRUNNER ?= $(top_srcdir)/benchmarks/_benchmarkrunner.livecodescript

LCS_CMD = $(LCS_ENGINE) $(LCS_ENGINE_FLAGS) $(LCS_BENCHMARKRUNNER) run


################################################################
# Top-level targets
################################################################

.DEFAULT: benchmark

benchmark: lcs-benchmark

clean:
	-rm -rf $(LCS_LOG)

.PHONY: benchmark clean

################################################################
# Engine benchmarks
################################################################

lcs-benchmark: $(LCS_ENGINE)
	@rm -f $(LCS_LOG)
	@cmd="$(LCS_CMD)"; \
	echo "$$cmd" $(_PRINT_RULE); \
	$$cmd
