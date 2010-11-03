# Copyright (C) 2010 Rafael R. Sevilla
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
TESTS = check_arith check_alloc check_hash check_io check_pp \
	check_unicode check_vm check_ciel
check_PROGRAMS = check_arith check_alloc check_hash check_io \
	check_pp check_unicode check_vm check_ciel


check_arith_SOURCES = check_arith.c $(top_builddir)/src/arcueid.h
check_arith_CFLAGS = @CHECK_CFLAGS@
check_arith_LDADD = @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_alloc_SOURCES = check_alloc.c $(top_builddir)/src/arcueid.h
check_alloc_CFLAGS = @CHECK_CFLAGS@
check_alloc_LDADD = @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_hash_SOURCES = check_hash.c $(top_builddir)/src/arcueid.h
check_hash_CFLAGS = @CHECK_CFLAGS@
check_hash_LDADD = @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_pp_SOURCES = check_pp.c $(top_builddir)/src/arcueid.h
check_pp_CFLAGS = @CHECK_CFLAGS@
check_pp_LDADD = @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

#check_reader_SOURCES = check_reader.c $(top_builddir)/src/arcueid.h
#check_reader_CFLAGS = @CHECK_CFLAGS@
#check_reader_LDADD = @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_unicode_SOURCES = check_unicode.c $(top_builddir)/src/utf.h
check_unicode_CFLAGS = @CHECK_CFLAGS@
check_unicode_LDADD = @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_vm_SOURCES = check_vm.c $(top_builddir)/src/vmengine.h
check_vm_CFLAGS = @CHECK_CFLAGS@
check_vm_LDADD = @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_io_SOURCES = check_io.c $(top_builddir)/src/arcueid.h
check_io_CFLAGS = @CHECK_CFLAGS@
check_io_LDADD = @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@

check_ciel_SOURCES = check_ciel.c $(top_builddir)/src/arcueid.h
check_ciel_CFLAGS = @CHECK_CFLAGS@
check_ciel_LDADD = @CHECK_LIBS@ -L../src @LIBARCUEID_LIBS@
