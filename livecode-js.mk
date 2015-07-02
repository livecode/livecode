top_srcdir = $(shell pwd)

EMSDK_ENV = /home/pbrett/opt/emsdk_portable/emsdk_env.sh

all: generate-js

normal-compile:
	. $(EMSDK_ENV); $(MAKE) -k all-emscripten V=1

generate-js: normal-compile
	cd build-emscripten/livecode/out/Debug && \
	cp standalone-community{,.bc} && \
	. $(EMSDK_ENV); emcc -O2 -g \
	    -s EXPORTED_FUNCTIONS=@$(top_srcdir)/engine/src/em-exported.json \
	    -s ASSERTIONS=1 \
	    -s EMTERPRETIFY=1 \
	    -s EMTERPRETIFY_ASYNC=1 \
	    -s EMTERPRETIFY_WHITELIST=@$(top_srcdir)/engine/src/em-whitelist.json \
	    -s ALLOW_MEMORY_GROWTH=1 \
	    -s TOTAL_MEMORY=67108864 \
	    --preload-file boot \
	    --js-library $(top_srcdir)/engine/src/em-async.js \
	    --js-library $(top_srcdir)/engine/src/em-event.js \
	    -o standalone-community.html \
	    standalone-community.bc

.PHONY: all normal-compile generate-js
