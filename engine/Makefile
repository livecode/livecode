.PHONY: engine standalone runtime kernel newruntime server

kernel:
	$(MAKE) -f Makefile.kernel libkernel

engine: kernel
	$(MAKE) -f Makefile.development engine
	if [ -e ../_build/linux/debug/engine ]; then cp ../_build/linux/debug/engine revolution.debug.x86; fi
	if [ -e ../_build/linux/release/engine ]; then cp ../_build/linux/release/engine revolution.release.x86; fi

standalone: kernel
	$(MAKE) -f Makefile.standalone standalone

installer: kernel
	$(MAKE) -f Makefile.installer installer

server:
	$(MAKE) -f Makefile.server server

clean:
	$(MAKE) -f Makefile.development clean
	$(MAKE) -f Makefile.installer clean
	$(MAKE) -f Makefile.standalone clean
	$(MAKE) -f Makefile.runtime clean
	$(MAKE) -f Makefile.kernel clean
	$(MAKE) -f Makefile.server clean

