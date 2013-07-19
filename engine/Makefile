.PHONY: engine standalone runtime kernel newruntime server kernel-standalone kernel-development kernel-server

kernel:
	$(MAKE) -f Makefile.kernel libkernel

kernel-standalone:
	$(MAKE) -f Makefile.kernel-standalone libkernel-standalone

kernel-development:
	$(MAKE) -f Makefile.kernel-development libkernel-development

kernel-server:
	$(MAKE) -f Makefile.kernel-server libkernel-server

engine: kernel kernel-development
	$(MAKE) -f Makefile.development engine-community

standalone: kernel kernel-standalone
	$(MAKE) -f Makefile.standalone standalone-community

installer: kernel
	$(MAKE) -f Makefile.installer installer

server: kernel-server
	$(MAKE) -f Makefile.server server-community

clean:
	$(MAKE) -f Makefile.development clean
	$(MAKE) -f Makefile.installer clean
	$(MAKE) -f Makefile.standalone clean
	$(MAKE) -f Makefile.runtime clean
	$(MAKE) -f Makefile.kernel clean
	$(MAKE) -f Makefile.server clean
