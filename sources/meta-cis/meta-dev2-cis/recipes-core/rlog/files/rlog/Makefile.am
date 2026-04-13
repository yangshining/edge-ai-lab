ACLOCAL_AMFLAGS = -I m4

include_HEADERS = inc/rlog.h
bin_PROGRAMS = src/rlog

src_rlog_SOURCES = src/rlog.c
src_rlog_CFLAGS = $(AM_CFLAGS)
src_rlog_CPPFLAGS = $(AM_CPPFLAGS)

writerlogdir = $(prefix)/bin
writerlog_SCRIPTS = $(top_srcdir)/src/write_rlog
