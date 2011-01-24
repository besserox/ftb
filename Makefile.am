ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -I${top_srcdir}/include

noinst_LTLIBRARIES =
lib_LTLIBRARIES =
sbin_PROGRAMS =
dist_include_HEADERS =
bin_PROGRAMS =
bin_SCRIPTS =
EXTRA_DIST = $(top_srcdir)/VERSION

include src/Makefile.mk
include include/Makefile.mk
include components/Makefile.mk
include docs/Makefile.mk

EXTRA_DIST += $(top_srcdir)/LICENSE
