## Process this file with automake to produce Makefile.in

SUBDIRS = include . 

DIST_SUBDIRS = include . doc example python xstc

INCLUDES = -I$(top_builddir)/include -I@srcdir@/include @THREAD_CFLAGS@ @Z_CFLAGS@ 

noinst_libdir = ${exec_prefix}/lib/cvsnt
noinst_lib_LTLIBRARIES = libxml2.la
libxml2_la_LIBADD = @THREAD_LIBS@ @Z_LIBS@ $(ICONV_LIBS) @M_LIBS@ @WIN32_EXTRA_LIBADD@

libxml2_la_LDFLAGS = @CYGWIN_EXTRA_LDFLAGS@ @WIN32_EXTRA_LDFLAGS@ -version-info @LIBXML_VERSION_INFO@ @MODULE_PLATFORM_LIBS@

if WITH_TRIO_SOURCES
libxml2_la_SOURCES = SAX.c entities.c encoding.c error.c parserInternals.c  \
		parser.c tree.c hash.c list.c xmlIO.c xmlmemory.c uri.c  \
		valid.c xlink.c HTMLparser.c HTMLtree.c debugXML.c xpath.c  \
		xpointer.c xinclude.c nanohttp.c nanoftp.c DOCBparser.c \
		catalog.c globals.c threads.c c14n.c xmlstring.c \
		xmlregexp.c xmlschemas.c xmlschemastypes.c xmlunicode.c \
		triostr.c trio.c xmlreader.c relaxng.c dict.c SAX2.c \
		xmlwriter.c legacy.c chvalid.c pattern.c xmlsave.c \
		xmlmodule.c schematron.c
else
libxml2_la_SOURCES = SAX.c entities.c encoding.c error.c parserInternals.c  \
		parser.c tree.c hash.c list.c xmlIO.c xmlmemory.c uri.c  \
		valid.c xlink.c HTMLparser.c HTMLtree.c debugXML.c xpath.c  \
		xpointer.c xinclude.c nanohttp.c nanoftp.c DOCBparser.c \
		catalog.c globals.c threads.c c14n.c xmlstring.c \
		xmlregexp.c xmlschemas.c xmlschemastypes.c xmlunicode.c \
		xmlreader.c relaxng.c dict.c SAX2.c \
		xmlwriter.c legacy.c chvalid.c pattern.c xmlsave.c \
		xmlmodule.c schematron.c
endif

DEPS = $(top_builddir)/libxml2.la
LDADDS = @STATIC_BINARIES@ $(top_builddir)/libxml2.la @THREAD_LIBS@ @Z_LIBS@ $(ICONV_LIBS) @M_LIBS@ @WIN32_EXTRA_LIBADD@


man_MANS = xml2-config.1 libxml.3

m4datadir = $(datadir)/aclocal
m4data_DATA = libxml.m4

install:
	

cleanup:
	-@(find . -name .\#\* -exec rm {} \;)

dist-hook: cleanup libxml2.spec
	-cp libxml2.spec $(distdir)
	(cd $(srcdir) ; tar -cf - --exclude CVS win32 macos vms bakefile test result) | (cd $(distdir); tar xf -)

dist-source: distdir
	$(AMTAR) -chof - --exclude Tests --exclude test --exclude result $(distdir) | GZIP=$(GZIP_ENV) gzip -c >`echo "$(distdir)" | sed "s+libxml2+libxml2-sources+"`.tar.gz

dist-test: distdir
	(mkdir -p $(distdir))
	(cd $(srcdir) ; tar -cf - --exclude CVS xstc/Tests) | (cd $(distdir); tar xf -)
	tar -cf - $(distdir)/test $(distdir)/result $(distdir)/xstc/Tests  $(distdir)/Makefile.tests $(distdir)/README $(distdir)/README.tests $(distdir)/AUTHORS $(distdir)/testapi.c $(distdir)/runtest.c $(distdir)/runsuite.c | GZIP=$(GZIP_ENV) gzip -c >`echo "$(distdir)" | sed "s+libxml2+libxml2-tests+"`.tar.gz
	@(rm -rf $(distdir)/xstc/Test)

cleantar:
	@(rm -f libxml*.tar.gz COPYING.LIB)

rpm: cleanup cleantar
	@(unset CDPATH ; $(MAKE) dist-source dist && rpmbuild -ta $(distdir).tar.gz)

## We create xml2Conf.sh here and not from configure because we want
## to get the paths expanded correctly.  Macros like srcdir are given
## the value NONE in configure if the user doesn't specify them (this
## is an autoconf feature, not a bug).

xml2Conf.sh: xml2Conf.sh.in Makefile
## Use sed and then mv to avoid problems if the user interrupts.
	sed -e 's?\@XML_LIBDIR\@?$(XML_LIBDIR)?g' \
	    -e 's?\@XML_INCLUDEDIR\@?$(XML_INCLUDEDIR)?g' \
	    -e 's?\@VERSION\@?$(VERSION)?g' \
	    -e 's?\@XML_LIBS\@?$(XML_LIBS)?g' \
	       < $(srcdir)/xml2Conf.sh.in > xml2Conf.tmp \
	&& mv xml2Conf.tmp xml2Conf.sh

CLEANFILES=xml2Conf.sh

confexecdir=$(libdir)
confexec_DATA = xml2Conf.sh
CVS_EXTRA_DIST=
EXTRA_DIST = xml2-config.in xml2Conf.sh.in libxml.spec.in libxml2.spec \
             libxml.m4 Copyright check-xml-test-suite.py gentest.py \
	     check-relaxng-test-suite.py check-relaxng-test-suite2.py \
	     check-xsddata-test-suite.py check-xinclude-test-suite.py \
             example/Makefile.am example/gjobread.c example/gjobs.xml \
	     $(man_MANS) libxml-2.0.pc.in libxml-2.0-uninstalled.pc.in \
	     trionan.c trionan.h triostr.c triostr.h trio.c trio.h \
	     triop.h triodef.h libxml.h elfgcchack.h \
	     testThreadsWin32.c genUnicode.py TODO_SCHEMAS \
	     dbgen.pl dbgenattr.pl regressions.py regressions.xml \
	     README.tests Makefile.tests \
	     $(CVS_EXTRA_DIST)


pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libxml-2.0.pc

#
# Install the tests program sources as examples 
#
BASE_DIR=$(datadir)/doc
DOC_MODULE=libxml2-$(VERSION)
EXAMPLES_DIR=$(BASE_DIR)/$(DOC_MODULE)/examples

install-data-local: 
	$(mkinstalldirs) $(DESTDIR)$(BASE_DIR)/$(DOC_MODULE)
	-@INSTALL@ -m 0644 $(srcdir)/Copyright $(DESTDIR)$(BASE_DIR)/$(DOC_MODULE)
	$(mkinstalldirs) $(DESTDIR)$(EXAMPLES_DIR)
	-@INSTALL@ -m 0644 $(srcdir)/xmllint.c $(DESTDIR)$(EXAMPLES_DIR)
	-@INSTALL@ -m 0644 $(srcdir)/testSAX.c $(DESTDIR)$(EXAMPLES_DIR)
	-@INSTALL@ -m 0644 $(srcdir)/testHTML.c $(DESTDIR)$(EXAMPLES_DIR)
	-@INSTALL@ -m 0644 $(srcdir)/testXPath.c $(DESTDIR)$(EXAMPLES_DIR)

tst: tst.c
	$(CC) $(CFLAGS) -Iinclude -o tst tst.c .libs/libxml2.a -lpthread -lm -lz

sparse: clean
	$(MAKE) CC=cgcc
