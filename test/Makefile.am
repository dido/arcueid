# Copyright (C) 2013 Rafael R. Sevilla
#
# This file is part of Arcueid
#
# Arcueid is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation; either version 3 of the
# License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, see <http://www.gnu.org/licenses/>.
#
TESTS = check_string check_is_iso check_aff check_io check_reader \
	check_arith check_vmengine check_env check_compiler check_builtins \
	check_hash check_error check_pp check_arc
check_PROGRAMS = check_string check_is_iso check_aff \
	check_io check_reader check_arith check_vmengine check_env \
	check_compiler check_builtins check_hash check_error check_pp \
	check_arc

# check_gc_SOURCES = check_gc.c $(top_builddir)/src/arcueid.h
# check_gc_CFLAGS = @CHECK_CFLAGS@
# check_gc_LDADD = @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_string_SOURCES = check_string.c $(top_builddir)/src/arcueid.h
check_string_CFLAGS = @CHECK_CFLAGS@
check_string_LDADD = ../src/libarcueid.la @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_is_iso_SOURCES = check_is_iso.c $(top_builddir)/src/arcueid.h
check_is_iso_CFLAGS = @CHECK_CFLAGS@
check_is_iso_LDADD = ../src/libarcueid.la @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_pp_SOURCES = check_pp.c $(top_builddir)/src/arcueid.h
check_pp_CFLAGS = @CHECK_CFLAGS@
check_pp_LDADD = ../src/libarcueid.la @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_aff_SOURCES = check_aff.c $(top_builddir)/src/arcueid.h
check_aff_CFLAGS = @CHECK_CFLAGS@
check_aff_LDADD = ../src/libarcueid.la @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_io_SOURCES = check_io.c $(top_builddir)/src/arcueid.h
check_io_CFLAGS = @CHECK_CFLAGS@
check_io_LDADD = ../src/libarcueid.la @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_reader_SOURCES = check_reader.c $(top_builddir)/src/arcueid.h
check_reader_CFLAGS = @CHECK_CFLAGS@
check_reader_LDADD = ../src/libarcueid.la @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_arith_SOURCES = check_arith.c $(top_builddir)/src/arcueid.h
check_arith_CFLAGS = @CHECK_CFLAGS@
check_arith_LDADD = ../src/libarcueid.la @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_vmengine_SOURCES = check_vmengine.c $(top_builddir)/src/arcueid.h
check_vmengine_CFLAGS = @CHECK_CFLAGS@
check_vmengine_LDADD = ../src/libarcueid.la @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_env_SOURCES = check_env.c $(top_builddir)/src/arcueid.h
check_env_CFLAGS = @CHECK_CFLAGS@
check_env_LDADD = ../src/libarcueid.la @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_compiler_SOURCES = check_compiler.c $(top_builddir)/src/arcueid.h
check_compiler_CFLAGS = @CHECK_CFLAGS@
check_compiler_LDADD = ../src/libarcueid.la @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_builtins_SOURCES = check_builtins.c $(top_builddir)/src/arcueid.h
check_builtins_CFLAGS = @CHECK_CFLAGS@
check_builtins_LDADD = ../src/libarcueid.la @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_hash_SOURCES = check_hash.c $(top_builddir)/src/arcueid.h
check_hash_CFLAGS = @CHECK_CFLAGS@
check_hash_LDADD = ../src/libarcueid.la @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_error_SOURCES = check_error.c $(top_builddir)/src/arcueid.h
check_error_CFLAGS = @CHECK_CFLAGS@
check_error_LDADD = ../src/libarcueid.la @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_arc_SOURCES = check_arc.c $(top_builddir)/src/arcueid.h
check_arc_CFLAGS = @CHECK_CFLAGS@
check_arc_LDADD = ../src/libarcueid.la @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@
