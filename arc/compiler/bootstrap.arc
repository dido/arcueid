;; Copyright (C) 2014 Rafael R. Sevilla
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
;; License along with this library; if not, see <http://www.gnu.org/licenses/>
;;
;; This file contains definitions of functions that should properly be
;; defined within the Arcueid runtime core.  The definitions provided
;; here are intended for use when bootstrapping the compiler as well
;; as for use by the unit tests.  They are intended to be used only
;; when the compiler is run from within mzscheme/racket-based versions of
;; Arc, and should never be loaded from within Arcueid itself.

(def fixnump (val)
  (and (isa val 'int)
       (>= val -9223372036854775808)	; valid for 64-bit arches
       (<= val 9223372036854775807)))

;;(def fixnump (val)
;;  (and (isa val 'int)
;;       (>= val -2147483648)	; valid for 32-bit arches
;;       (<= val 2147483648)))

;; This definition will cause the compiler to NEVER generate ldi instructions!
;; All numbers will be considered bignums.
;; (def fixnump (val) nil)

;; This should be defined by the runtime. The contex argument is a compiler
;; context, which is in most Arcueid runtimes an opaque type that can only
;; be manipulated by other runtime functions.  For development and
;; bootstrapping purposes, we let it be a cons cell whose car is a list of
;; instructions and whose cdr is a list of literals used in the code being
;; generated.
(def acc-gen (ctx opcode . args)
  (do (= (car ctx) (join (car ctx) (cons opcode args) nil))
      ctx))

;; Another runtime funtion. Returns current offset for code generation
(def acc-codeptr (ctx)
  (len (car ctx)))

;; Patch an opcode at the provided offset
(def acc-patch (ctx offset value)
  (= ((car ctx) offset) value))

;; If the literal +lit+ is not found in the literal list of the context,
;; it will add the literal to the context.
(def acc-getliteral (lit ctx)
  (let literals (cdr ctx)
    (aif ((afn (lit literals idx)
	       (if (no literals) nil
		   (iso (car literals) lit) idx
		   (self lit (cdr literals) (+ 1 idx)))) lit literals 0)
	 it
	 (do1 (len literals)
	      (= (cdr ctx) (join literals (cons lit nil)))))))

