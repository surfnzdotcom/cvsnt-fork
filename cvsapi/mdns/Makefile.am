## Process this file with automake to produce Makefile.in

libdir=$(cvs_library_dir)/mdns

if WITH_HOWL
howl_la_SOURCES = \
	howl/mdns_howl.cpp \
	howl/mdns_howl.h 
howl_la_LDFLAGS = -module -avoid-version  -L$(DESTDIR)/$(libdir)
howl_la_LIBADD = @HOWL_LIBS@ 
howl_mdns_lib = howl.la
else
howl_mdns_lib =
endif

if WITH_DNS_SD
apple_la_SOURCES = \
	apple/mdns_apple.cpp \
	apple/mdns_apple.h 
apple_la_LDFLAGS = -module -avoid-version  -L$(DESTDIR)/$(libdir)
apple_la_LIBADD = @DNS_SD_LIBS@ 
apple_mdns_lib = apple.la
else
apple_mdns_lib =
endif

if WITH_MDNS
mini_la_SOURCES = \
	mini/mdns_mini.cpp \
	mini/mdns_mini.h 
mini_la_LDFLAGS = -module -avoid-version  -L$(DESTDIR)/$(libdir)
mini_la_LIBADD = ../../mdnsclient/libmdnsclient.la 
mini_mdns_lib = mini.la
else
mini_mdns_lib =
endif

AM_CPPFLAGS = -I$(top_srcdir)/lib -I$(top_srcdir)/cvsapi $(includeopt)

lib_LTLIBRARIES = $(howl_mdns_lib) \
	$(apple_mdns_lib) $(mini_mdns_lib) 



