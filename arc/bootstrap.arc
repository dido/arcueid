;; Copyright (C) 2009 Rafael R. Sevilla
;;
;; This file is part of Arcueid
;;
;; Arcueid is free software; you can redistribute it and/or modify it
;; under the terms of the GNU Lesser General Public License as
;; published by the Free Software Foundation; either version 3 of the
;; License, or (at your option) any later version.
;;
;; This library is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU Lesser General Public License for more details.
;;
;; You should have received a copy of the GNU Lesser General Public
;; License along with this library; if not, see  <http://www.gnu.org/licenses/>.
;;
(load "compiler.arc")
(load "comp-bsdef.arc")

(= ctx (compiler-new-context))
(= arcbaselib (readfile "arc.arc"))
(= arcueid-compiler (readfile "compiler.arc"))
(prn "Compiling Arc base library")
(walk arcbaselib [compile _ ctx nil nil])
(prn "Compiling Arcueid compiler")
(walk arcueid-compiler [compile _ ctx nil nil])
(prn "Outputting generated code to basecomp.arc")
(w/outfile fp "basecomp.arc" (write ctx fp))
(prn "Outputting generated C code to basecomp.c")
(w/outfile fp "basecomp.c" (code->ccode fp "basecomp" (context->code ctx)))
