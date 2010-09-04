;; Copyright (C) 2010 Rafael R. Sevilla
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
;; License along with this library; if not, see <http://www.gnu.org/licenses/>.
;;
;; First phase of compilation.  Generate a ctx file out of the base
;; library and the compiler itself.  This basically compiles the compiler
;; with itself, bootstrapping it.
;;
(load "compiler.arc")

;; Comment this out when using Arcueid to bootstrap.  This is only required
;; when using standard Paul Graham Arc3 or Anarki.
(load "comp-bsdef.arc")

(= ctx (compiler-new-context))
(= arcbaselib (readfile "arc.arc"))
(= arcueid-compiler (readfile "compiler.arc"))
(prn "Compiling Arc base library")
(walk arcbaselib [compile _ ctx nil nil])
(prn "Compiling Arcueid compiler")
(walk arcueid-compiler [compile _ ctx nil nil])
(prn "Outputting generated code to basecomp.ctx")
(w/outfile fp "basecomp.ctx" (write ctx fp))
