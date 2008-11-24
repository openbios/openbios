ARCH= $(shell test -r rules.xml && cat rules.xml |grep "^ARCH" |cut -d\= -f2|tr -d \ )
HOSTARCH=$(shell config/scripts/archname)
CROSSCFLAGS=$(shell config/scripts/crosscflags $(HOSTARCH) $(ARCH))
ODIR=obj-$(ARCH)

all: requirements archtest info build

archtest: prepare
	@test -r config.xml -a -r rules.xml || \
		( echo ; echo "Please run the following command first:"; echo ; \
		  echo "  $$ config/scripts/switch-arch <arch>"; \
		  echo; echo "<arch> can be one out of x86, amd64, cross-ppc, ppc"; \
		  echo "       cross-sparc32, sparc32, cross-sparc64, sparc64"; \
		  echo; exit 1 )

requirements:
	@which toke &>/dev/null || ( echo ; echo "Please install the OpenBIOS fcode utilities."; \
			echo; echo "Download with :"; \
			echo "  $$ svn co svn://openbios.org/openbios/fcode-utils"; \
			echo; exit 1 )
	@which xsltproc &>/dev/null || ( echo ; echo "Please install libxslt2"; \
			echo; exit 1 )

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
	@mkdir -p $(ODIR)/target
	@mkdir -p $(ODIR)/target/include
	@mkdir -p $(ODIR)/target/arch
	@mkdir -p $(ODIR)/target/arch/unix
	@mkdir -p $(ODIR)/target/arch/$(ARCH)
	@mkdir -p $(ODIR)/target/arch/ppc
	@mkdir -p $(ODIR)/target/arch/ppc/briq # no autodetection of those..
	@mkdir -p $(ODIR)/target/arch/ppc/pearpc
	@mkdir -p $(ODIR)/target/arch/ppc/qemu
	@mkdir -p $(ODIR)/target/arch/ppc/mol
	@mkdir -p $(ODIR)/target/arch/x86
	@mkdir -p $(ODIR)/target/arch/x86/xbox
	@mkdir -p $(ODIR)/target/libgcc
	@mkdir -p $(ODIR)/target/kernel
	@mkdir -p $(ODIR)/target/modules
	@mkdir -p $(ODIR)/target/fs
	@mkdir -p $(ODIR)/target/fs/grubfs
	@mkdir -p $(ODIR)/target/fs/hfs
	@mkdir -p $(ODIR)/target/fs/hfsplus
	@mkdir -p $(ODIR)/target/drivers
	@mkdir -p $(ODIR)/target/libc
	@mkdir -p $(ODIR)/host/include
	@mkdir -p $(ODIR)/host/kernel
	@mkdir -p $(ODIR)/forth
	@ln -s $(PWD)/include/$(ARCH) $(ODIR)/target/include/asm
	@#compile the host binary with target settings instead
	@#ln -s $(PWD)/include/$(HOSTARCH) $(ODIR)/host/include/asm
	@echo "ok."

# This is needed because viewvc messes with the permissions of executables:
prepare:
	@chmod 755 utils/dist/debian/rules
	@chmod 755 config/scripts/switch-arch
	@chmod 755 config/scripts/archname
	@chmod 755 config/scripts/reldir
	@chmod 755 config/scripts/crosscflags

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
	@( $(MAKE) -f $(ODIR)/Makefile "CROSSCFLAGS=$(CROSSCFLAGS)" > $(ODIR)/build.log 2>&1 && echo "ok." ) || \
		( echo "error:"; tail -15 $(ODIR)/build.log )

build-verbose:
	@echo "Building..."
	$(MAKE) -f $(ODIR)/Makefile "CROSSCFLAGS=$(CROSSCFLAGS)"

run: 
	@echo "Running..."
	@$(ODIR)/openbios-unix $(ODIR)/openbios-unix.dict


# The following two targets will only work on x86 so far.
# 
$(ODIR)/openbios.iso: $(ODIR)/openbios.multiboot $(ODIR)/openbios-x86.dict
	@mkisofs -input-charset UTF-8 -r -b boot/grub/stage2_eltorito -no-emul-boot \
	-boot-load-size 4 -boot-info-table -o $@ utils/iso $^

runiso: $(ODIR)/openbios.iso
	qemu -cdrom $^

