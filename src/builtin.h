/* 
  Copyright (C) 2012 Rafael R. Sevilla

  This file is part of Arcueid

  Arcueid is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; either version 3 of the
  License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _BUILTIN_H_

#define _BUILTIN_H_

#include "arcueid.h"

/* I/O */
extern value arc_read(arc *c, value src, value eof);
extern value arc_ssyntax(arc *c, value x);
extern value arc_ssexpand(arc *c, value sym);

/* application */
extern value arc_apply2(arc *c, value argv, value rv, CC4CTX);
extern value arc_eval(arc *c, value expr, value rv, CC4CTX);
extern value arc_disasm(arc *c, value code);
extern value arc_macex(arc *c, value expr);
extern value arc_macex1(arc *c, value expr);

/* pretty printer */
extern value arc_disp(arc *c, int argc, value *argv);
extern value arc_sdisp(arc *c, value sexpr, value port);
extern value arc_swrite(arc *c, value sexpr, value port);
extern value arc_write(arc *c, int argc, value *argv);

/* I/O */
extern value arc_filefp(arc *c, FILE *fp, value filename);
extern value arc_soutfile(arc *c, int argc, value *argv);
extern value arc_sinstring(arc *c, int argc, value *argv);
extern value arc_soutstring(arc *c, int argc, value *argv);
extern value arc_pipe_from(arc *c, value cmd);

extern value arc_sreadb(arc *c, int argc, value *argv);
extern value arc_swriteb(arc *c, int argc, value *argv);
extern value arc_sreadc(arc *c, int argc, value *argv);
extern value arc_sungetc(arc *c, int argc, value *argv);
extern value arc_speekc(arc *c, int argc, value *argv);
extern value arc_swritec(arc *c, int argc, value *argv);

extern value arc_seek(arc *c, value fd, value ofs, value whence);
extern value arc_tell(arc *c, value fd);
extern value arc_close(arc *c, value fd);

extern value arc_call_w_stdin(arc *c, value argv, value rv, CC4CTX);
extern value arc_call_w_stdout(arc *c, value argv, value rv, CC4CTX);

extern value arc_dir(arc *c, value dirname);
extern value arc_dir_exists(arc *c, value dirname);
extern value arc_mkdir(arc *c, value dirname);
extern value arc_file_exists(arc *c, value filename);
extern value arc_rmfile(arc *c, value filename);

/* Ciel */
extern value arc_ciel_unmarshal(arc *c, value fd);

/* misc */
extern value arc_sref(arc *c, value com, value val, value ind);

/* time */
extern value arc_current_gc_milliseconds(arc *c);
extern value arc_current_process_milliseconds(arc *c);
extern value arc_seconds(arc *c);
extern value arc_msec(arc *c);

extern value arc_system(arc *c, value cmd);
extern value arc_quit(arc *c, int argc, value *argv);

/* threads */
extern value arc_sleep(arc *c, value sleeptime);
extern value arc_mkchan(arc *c);
extern value arc_recv_channel(arc *c, value chan);
extern value arc_send_channel(arc *c, value chan, value data);
extern value arc_select_channel(arc *c, value readchans, value writechans,
				value block);
extern value arc_atomic_cell(arc *c, int argc, value *argv);
extern value arc_atomic_chan(arc *c);
extern value arc_dead(arc *c, value thr);

#endif
