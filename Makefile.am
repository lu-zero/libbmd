ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -I$(sdk_dir)
AM_CXXFLAGS = -fno-rtti -Wno-multichar -D__STDC_CONSTANT_MACROS
libbmd_la_LINK = $(LINK) $(libbmd_la_LDFLAGS)
# Mention a dummy pure C file to trigger generation of the $(LINK) variable
nodist_EXTRA_libbmd_la_SOURCES = dummy.c

libbmdincludedir = $(includedir)/libbmd

libbmdinclude_HEADERS = \
    $(top_srcdir)/src/decklink_capture.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libbmd.pc

lib_LTLIBRARIES = libbmd.la

libbmd_la_LDFLAGS = -version-info @LIBBMD_VERSION@ -no-undefined
libbmd_la_CXXFLAGS = $(AM_CXXFLAGS)
libbmd_la_LIBADD = $(LIBCXX_LDFLAGS)

libbmd_la_SOURCES = \
	$(top_srcdir)/src/decklink_capture.cpp

if HAVE_TOOLS

bmdplay_SOURCES = \
	$(top_srcdir)/src/bmdplay.cpp \
	$(top_srcdir)/src/Play.h

bmdplay_CXXFLAGS = $(TOOLS_CFLAGS) $(AM_CXXFLAGS)
bmdplay_LDADD = $(TOOLS_LIBS)

bmdcapture_SOURCES = \
	$(top_srcdir)/src/bmdcapture.c

bmdcapture_CFLAGS = $(TOOLS_CFLAGS) $(AM_CFLAGS)
bmdcapture_LDADD = $(TOOLS_LIBS) libbmd.la

bmdgenlock_SOURCES = \
	$(top_srcdir)/src/bmdgenlock.cpp

bmdgenlock_CXXFLAGS = $(TOOLS_CFLAGS) $(AM_CXXFLAGS)
bmdgenlock_LDADD = $(TOOLS_LIBS)

bin_PROGRAMS = bmdplay bmdcapture bmdgenlock

endif

#EXTRA_DIST = $(top_srcdir)/NOTICE

