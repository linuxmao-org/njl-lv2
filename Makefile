PREFIX = /usr/local
INSTALL_DIR = $(PREFIX)/lib/lv2


PLUGPKG = njl
VERSION = 0.0.1

PLUGINS =                                       \
        ieee_noise-njl.lv2                      \
        int_noise-njl.lv2                       \
        representation_experiments-njl.lv2      \
        risset_scales-njl.lv2

DISTFILES = AUTHORS Makefile README plugins

DARWIN := $(shell uname | grep Darwin)
OS := $(shell uname -s)

ifdef DARWIN
EXT = so
CC = gcc
PLUGIN_CFLAGS = -Wall -I. -Iinclude -O3 -fomit-frame-pointer -fstrength-reduce -funroll-loops -fPIC -DPIC -arch i386 -arch ppc -ffast-math -msse -fno-common -flat_namespace -bundle -isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch ppc $(CFLAGS)
PLUGIN_LDFLAGS = -arch i386 -arch ppc -dynamic -Wl,-syslibroot,/Developer/SDKs/MacOSX10.4u.sdk -bundle -multiply_defined suppress -lc $(LDFLAGS)
BUILD_PLUGINS = $(PLUGINS)
else
EXT = so
PLUGIN_CFLAGS = -Wall -I. -Iinclude -O3 -fomit-frame-pointer -fstrength-reduce -funroll-loops -fPIC -DPIC $(CFLAGS)
PLUGIN_LDFLAGS = -shared $(LDFLAGS)
BUILD_PLUGINS = $(PLUGINS)
endif


OBJECTS = $(shell echo $(BUILD_PLUGINS) | sed 's/\([^ ]*\.lv2\)/plugins\/\1\/plugin.$(EXT)/g')

DISTDIR = $(PLUGPKG)-lv2-$(VERSION)

all: $(OBJECTS)

%.c: OBJ = $(shell echo $@ | sed 's/\.c$$/-@OS@.$(EXT)/')

%.o: NAME = $(shell echo -n $@ | sed 's/plugins\/\(.*\)-$(PLUGPKG).*/\1/')
%.o: %.c
	$(CC) $(PLUGIN_CFLAGS) $($(NAME)_CFLAGS) $*.c -c -o $@

%.so: NAME = $(shell echo -n $@ | sed 's/plugins\/\(.*\)-$(PLUGPKG).*/\1/')
%.so: %.o %.ttl
	$(CC) $*.o $(PLUGIN_LDFLAGS) $($(NAME)_LDFLAGS) -o $@
	cp $@ $*-$(OS).$(EXT)
	sed 's/@OS@/$(OS)/g' < `dirname $@`/manifest.ttl.in > `dirname $@`/manifest.ttl

clean: dist-clean

dist-clean:
	rm -f plugins/*/*.{$(EXT),o} plugins/*/*.o plugins/*/manifest.ttl

install:
	@echo 'use install-user to install in home or install-system to install system wide'

install-system: INSTALL_DIR_REALLY=$(INSTALL_DIR)
install-system: all install-really

install-user: INSTALL_DIR_REALLY=~/.lv2/bundles
install-user: all install-really

install-really:
	for plugin in $(BUILD_PLUGINS); do \
		echo Installing $$plugin; \
		install -d $(INSTALL_DIR_REALLY)/$$plugin; \
		install plugins/$$plugin/*-$(OS).$(EXT) plugins/$$plugin/*.ttl $(INSTALL_DIR_REALLY)/$$plugin/ ; \
	done

dist: all dist-clean
	mkdir $(DISTDIR) && \
	cp -rf $(DISTFILES) $(DISTDIR) && \
	tar cofz $(DISTDIR).tar.gz $(DISTDIR) && \
	rm -rf $(DISTDIR)

.PRECIOUS: %.c %.ttl
