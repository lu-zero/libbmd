ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -I$(sdk_dir)
AM_CXXFLAGS = -fno-rtti -Wno-multichar -D__STDC_CONSTANT_MACROS
libbmd_la_LINK = $(LINK) $(libbmd_la_LDFLAGS)
# Mention a dummy pure C file to trigger generation of the $(LINK) variable
nodist_EXTRA_libbmd_la_SOURCES = dummy.c

libbmdincludedir = $(includedir)/libbmd

libbmdinclude_HEADERS = \
    $(srcdir)/libbmd/decklink.h \
    $(srcdir)/libbmd/decklink_capture.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libbmd.pc

lib_LTLIBRARIES = libbmd.la

libbmd_la_LDFLAGS = -version-info @LIBBMD_VERSION@ -no-undefined
libbmd_la_CXXFLAGS = $(AM_CXXFLAGS)
libbmd_la_LIBADD = -lsupc++

libbmd_la_SOURCES = \
	$(srcdir)/libbmd/decklink.cpp \
	$(srcdir)/libbmd/decklink_capture.cpp

if HAVE_TOOLS

bmdcapture_SOURCES = \
	$(srcdir)/src/bmdcapture.c

bmdcapture_CFLAGS = $(TOOLS_CFLAGS) $(AM_CFLAGS)
bmdcapture_LDADD = $(TOOLS_LIBS) libbmd.la

bin_PROGRAMS = bmdcapture

endif

#EXTRA_DIST = $(srcdir)/NOTICE

