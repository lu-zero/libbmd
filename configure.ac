#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([libbmd], [0.1], [http://github.com/lu-zero/libbmd])
AC_CONFIG_AUX_DIR(.)
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([tar-ustar foreign no-dist-gzip dist-xz])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_HEADERS([config.h])

AC_ARG_WITH([sdkdir], AS_HELP_STRING([--with-sdkdir=DIR],
            [Path to the sdk directory @<:@LIBDIR/blackmagic-sdk@:>@]),
            [sdk_dir="$withval"], [sdk_dir='${libdir}/blackmagic-sdk'])

AC_SUBST([sdk_dir])

PKG_HAVE_WITH_MODULES([TOOLS], [libavformat libavcodec libswscale])

AX_CHECK_LIBRARY([LIBCXX], [], [supc++], [LIBBMD_DEPS=""; echo "sup"],
    [AX_CHECK_LIBRARY([LIBCXX], [], [stdc++], [LIBBMD_DEPS+=" -lstdc++"; echo "std"],
        [echo "FAIL"])]
)

AC_CANONICAL_HOST

AC_PROG_CXX

AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)
AC_SUBST(LIBBMD_DEPS)
AC_SUBST(LIBCXX_LDFLAGS)
AC_SUBST(LIBBMD_DEPS)

DOLT

LIBBMD_VERSION=0:1:0
AC_SUBST(LIBBMD_VERSION)

AC_CONFIG_FILES([Makefile
                 libbmd.pc])
AC_OUTPUT
