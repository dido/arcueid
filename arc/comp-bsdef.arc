;; Copyright (C) 2009,2010 Rafael R. Sevilla
;;
;; This file is part of CArc
;;
;; CArc is free software; you can redistribute it and/or modify it
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
;; License along with this library; if not, write to the Free Software
;; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA
;; 02110-1301 USA.
;;

;; This file contains definitions of functions that should properly be
;; defined within the CArc runtime core.  The definitions provided
;; here are intended for use when bootstrapping the compiler as well
;; as for use by the unit tests.  They are intended to be used only
;; when the compiler is run from within mzscheme-based versions of
;; Arc, and should never be loaded from within CArc itself.

;; This should be a C function (not standard Arc), but is defined here
;; for testing.
(def fixnump (val)
  (and (isa val 'int)
       (>= val -9223372036854775808)	; valid for 64-bit arches
       (<= val 9223372036854775807)))

;; This should be a C function as well.  The context argument is a
;; compiler context, which is in the CArc runtime an opaque type
;; which can only be manipulated from within an Arc function in
;; limited ways.  For development purposes, we let it be a cons
;; cell whose car is a list of instructions and whose cdr is a list
;; of literals used in the code being generated.
(def generate (ctx opcode . args)
  (do (= (car ctx) (join (car ctx) (cons opcode args)))
      ctx))

;; This should also be a C function.  Returns the current offset
;; for code generation.
(def code-ptr (ctx)
  (len (car ctx)))

;; This should also be a C function.  Patch an opcode at the provided
;; offset.
(def code-patch (ctx offset value)
  (= ((car ctx) offset) value))

;; This should be a C function as well.  If the literal +lit+ is not
;; found in the literal list of the context, it will add the literal
;; to the context.
(def find-literal (lit ctx)
  (let literals (cdr ctx)
    (aif ((afn (lit literals idx)
	    (if (no literals) nil
		(iso (car literals) lit) idx
		(self lit (cdr literals) (+ 1 idx)))) lit literals 0)
	 it
	 (do1 (len literals)
	      (= (cdr ctx) (join literals (cons lit nil)))))))

;; These should be parts of a C function as well.  Environments are
;; defined as lists of vectors (they appear here as lists as well because
;; standard Arc doesn't have vectors).  The first element of the
;; environment is all we really care about.
(def find-in-frame (var frame idx)
  (if (no frame) nil
      (is (car frame) var) idx
      (find-in-frame var (cdr frame) (+ idx 1))))

(def find-var (var env (o idx 0))
  (if (no env) '(nil 0 0)
      (aif (find-in-frame var (car (car env)) 0) `(t ,idx ,it)
	   (find-var var (cdr env) (+ idx 1)))))

;; This C function creates a new empty compilation context
(def compiler-new-context ()
  (cons nil nil))

;; This C function turns a compilation context into a code object
(def context->code (ctx)
  (annotate 'code ctx))

(def bytecode (code)
  (case code
    inop 0
    ipush 1
    ipop 2
    ildl 3
    ildi 4
    ildg 5
    istg 6
    iste 7
    imvarg 8
    imvoarg 9
    imvrarg 10
    icont 11
    ienv 12
    iapply 13
    iret 14
    ijmp 15
    ijt 16
    ijf 17
    itrue 18
    inil 19
    ihlt 20
    iadd 21
    isub 22
    imul 23
    idiv 24
    icons 25
    icar 26
    icdr 27
    iscar 28
    iscdr 29
    iis 30
    icmp 31
    ispl 32
    iiso 33
    code))
