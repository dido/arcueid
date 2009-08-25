# 
# Copyright (C) 2009 Rafael R. Sevilla
#
# This file is part of CArc
#
# CArc is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA
#  02110-1301 USA.
#
TESTS = check_arith
check_PROGRAMS = check_arith
check_alloc_SOURCES = check_arith.c $(top_builddir)/src/carc.h
check_alloc_CFLAGS = @CHECK_CFLAGS@
check_alloc_LDADD = @CHECK_LIBS@ $(top_builddir)/src/libcarc.la
check_alloc_LDFLAGS = -static
