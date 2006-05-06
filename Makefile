ARCH= $(shell cat rules.xml |grep "^ARCH" |cut -d\= -f2|tr -d \ )
HOSTARCH=$(shell config/scripts/archname)
ODIR=obj-$(ARCH)

all: info build

info:
	@echo "Building OpenBIOS on $(HOSTARCH) for $(ARCH)"

clean:
	@printf "Cleaning up..."
	@rm -rf $(ODIR) forth.dict.core 
	@find . -type f -name "*~" -exec rm \{\} \;
	@echo " ok"

directories: clean
	@printf "Initializing build tree..."
	@mkdir $(ODIR)
	@mkdir -p $(ODIR)/target/include
	@mkdir -p $(ODIR)/target/arch/unix
	@mkdir -p $(ODIR)/target/arch/$(ARCH)
	@mkdir -p $(ODIR)/target/arch/ppc/briq # no autodetection of those..
	@mkdir -p $(ODIR)/target/arch/ppc/pearpc
	@mkdir -p $(ODIR)/target/arch/ppc/mol
	@mkdir -p $(ODIR)/target/arch/x86/xbox
	@mkdir -p $(ODIR)/target/arch/sparc32/libgcc
	@mkdir -p $(ODIR)/target/kernel
	@mkdir -p $(ODIR)/target/modules
	@mkdir -p $(ODIR)/target/fs/grubfs
	@mkdir -p $(ODIR)/target/fs/hfs
	@mkdir -p $(ODIR)/target/fs/hfsplus
	@mkdir -p $(ODIR)/target/drivers
	@mkdir -p $(ODIR)/target/libc
	@mkdir -p $(ODIR)/host/include
	@mkdir -p $(ODIR)/host/kernel
	@mkdir -p $(ODIR)/host/toke
	@mkdir -p $(ODIR)/forth
	@ln -s $(PWD)/include/$(ARCH) $(ODIR)/target/include/asm
	@#compile the host binary with target settings instead
	@#ln -s $(PWD)/include/$(HOSTARCH) $(ODIR)/host/include/asm
	@echo "ok."

xml: directories
	@printf "Creating target Makefile..."
	@xsltproc config/xml/xinclude.xsl build.xml > $(ODIR)/build-full.xml
	@xsltproc config/xml/makefile.xsl $(ODIR)/build-full.xml > $(ODIR)/Makefile
	@echo "ok."
	@printf "Creating config files..."
	@xsltproc config/xml/config-c.xsl config.xml > $(ODIR)/host/include/autoconf.h
	@xsltproc config/xml/config-c.xsl config.xml > $(ODIR)/target/include/autoconf.h
	@xsltproc config/xml/config-forth.xsl config.xml > $(ODIR)/forth/config.fs
	@echo "ok."

build: xml
	@printf "Building..."
	@( $(MAKE) -f $(ODIR)/Makefile > $(ODIR)/build.log 2>&1 && echo "ok." ) || \
		( echo "error:"; tail -15 $(ODIR)/build.log )

build-verbose:
	@echo "Building..."
	$(MAKE) -f $(ODIR)/Makefile

run: 
	@echo "Running..."
	@$(ODIR)/openbios-unix $(ODIR)/openbios-unix.dict
