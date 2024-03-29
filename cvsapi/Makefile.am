## Process this file with automake to produce Makefile.in

if WITH_MDNS
  MDNS_SUBDIR = mdns
  mdns_client_lib = ../mdnsclient/libmdnsclient.la
else
  MDNS_SUBDIR =
  mdns_client_lib = 
endif

SUBDIRS=db $(MDNS_SUBDIR)

if WITH_PCRE
  pcre_inc = 
  pcre_lib = @PCRE_LIBS@
else
  pcre_inc = -I$(top_srcdir)/pcre
  pcre_lib = $(top_srcdir)/pcre/.libs/libpcreposix.a $(top_srcdir)/pcre/.libs/libpcre.a
endif

if WITH_LIBXML
  libxml_lib = @LIBXML_LIBS@
else
  libxml_lib = $(top_srcdir)/libxml/.libs/libxml2.a
endif

if WITH_ZLIB
  zlib_lib = @ZLIB_LIBS@
else
  zlib_lib = ../zlib/libz.a
endif

AM_CPPFLAGS = -I$(top_srcdir)/lib $(pcre_inc) $(includeopt) $(INCLTDL) -D_CVSAPI

libcvsapi_la_SOURCES = \
	ufc-crypt/ufccrypt.c \
	ufc-crypt/crypt_util.c \
	Codepage.cpp \
	Codepage.h \
	crypt.cpp \
	crypt.h \
	cvsapi.h \
	cvs_smartptr.h \
	cvs_string.cpp \
	cvs_string.h \
	diff/DiffBase.cpp \
	diff/DiffBase.h \
	diff/StringDiff.cpp \
	diff/StringDiff.h \
	DirectoryAccess.h \
	DnsApi.h \
	FileAccess.h \
	FileEntry.h \
	GetOptions.cpp \
	GetOptions.h \
	HttpSocket.h \
	LibraryAccess.h \
	md5calc.cpp \
	md5calc.h \
	mdns.cpp \
	mdns.h \
	rpcBase.cpp \
	rpcBase.h \
	RunFile.h \
	ServerIO.cpp \
	ServerIO.h \
	SocketIO.h \
	SqlConnection.h \
	SqlConnection.cpp \
	SqlConnectionInformation.h \
	SqlConnectionInformation.cpp \
	SqlRecordset.cpp \
	SqlRecordset.h \
	SqlVariant.cpp \
	SqlVariant.h \
	SSPIHandler.h \
	TagDate.cpp \
	TagDate.h \
	TokenLine.cpp \
	TokenLine.h \
	XmlNode.cpp \
	XmlNode.h \
	XmlTree.cpp \
	XmlTree.h \
	Zeroconf.cpp \
	Zeroconf.h \
	lib/api_system.h \
	lib/fncmp.c \
	lib/fncmp.h \
	lib/fnmatch.c \
	lib/fnmatch.h \
	lib/getdate.c \
	lib/getdate.h \
	lib/getmode.c \
	lib/getmode.h \
	lib/md5.c \
	lib/md5.h \
	lib/md5crypt.c \
	lib/md5crypt.h \
	lib/timegm.c \
	lib/timegm.h \
	unix/DirectoryAccess.cpp \
	unix/DnsApi.cpp \
	unix/FileAccess.cpp \
	unix/HttpSocket.cpp \
	unix/LibraryAccess.cpp \
	unix/RunFile.cpp \
	unix/SocketIO.cpp \
	unix/SSPIHandler.cpp 

libcvsapi_la_LDFLAGS = -release $(VERSION) -L$(DESTDIR)/$(libdir)


## This line doesnt work on HPUX if the target is a static library
## Any static libraries are delayed until the link of the executable
## and then when the executable is linked they are dropped leaving a 
## binary with missing symbols so you must also link in manually using
## LDADD in 'src' and 'run_script'

libcvsapi_la_LIBADD = \
        $(libxml_lib) \
        $(zlib_lib) \
        $(pcre_lib) \
        $(mdns_client_lib) \
        $(LIBLTDL) 

lib_LTLIBRARIES = libcvsapi.la
