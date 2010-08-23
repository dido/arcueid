;; Copyright (C) 2009,2010 Rafael R. Sevilla
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
;; License along with this library; if not, write to the Free Software
;; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA
;; 02110-1301 USA.
;;

;; This file contains definitions of functions that should properly be
;; defined within the Arcueid runtime core.  The definitions provided
;; here are intended for use when bootstrapping the compiler as well
;; as for use by the unit tests.  They are intended to be used only
;; when the compiler is run from within mzscheme-based versions of
;; Arc, and should never be loaded from within Arcueid itself.

;; This should be a C function (not standard Arc), but is defined here
;; for testing.
(def fixnump (val)
  (and (isa val 'int)
       (>= val -9223372036854775808)	; valid for 64-bit arches
       (<= val 9223372036854775807)))

;; This should be a C function as well.  The context argument is a
;; compiler context, which is in the Arcueid runtime an opaque type
;; which can only be manipulated from within an Arc function in
;; limited ways.  For development purposes, we let it be a cons
;; cell whose car is a list of instructions and whose cdr is a list
;; of literals used in the code being generated.
(def generate (ctx opcode . args)
  (do (= (car ctx) (join (car ctx) (cons opcode args) nil))
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
    ilde 7
    iste 8
    imvarg 9
    imvoarg 10
    imvrarg 11
    icont 12
    ienv 13
    iapply 14
    iret 15
    ijmp 16
    ijt 17
    ijf 18
    itrue 19
    inil 20
    ihlt 21
    iadd 22
    isub 23
    imul 24
    idiv 25
    icons 26
    icar 27
    icdr 28
    iscar 29
    iscdr 30
    ispl 31
    iis 32
    iiso 33
    igt 34
    ilt 35
    code))

;; Number of instructions to the arg
(def instnargs (code)
  (case code
    ildl 1
    ildi 1
    ildg 1
    istg 1
    ilde 2
    iste 2
    imvarg 1
    imvoarg 1
    imvrarg 1
    icont 1
    ienv 1
    iapply 1
    ijmp 1
    ijt 1
    ijf 1
    0))

(def literal->c (lit name count)
  (case (type lit)
    char (+ "arc_mkchar(c, " (coerce lit 'int) ")")
    ;; XXX: This is broken for some escape sequences!
    string (+ "arc_mkstringc(c, \"" lit "\")")
    ;; Only bignums will generate int values
    int (+ "arc_mkbignumstr(c, \"" lit "\")")
    ;; XXX: Will only work for flonum values.  The sad fact is that
    ;; the type function does not discriminate between flonums and
    ;; complex numbers.
    num (+ "arc_mkflonum(c, " lit ")")
    code (+ "gencode_" name count "(c)")
    sym (+ "arc_intern_cstr(c, \"" (coerce lit 'string) "\")")
    (err "Unrecognized type of literal " lit " with type " (type lit))))

(def code->ccode (port name code)
  ;; Look for literals in the code which represent code objects.  Call
  ;; ourselves recursively on those so that the code for them gets
  ;; generated.
  ((afn (code count)
     (if (no code) nil
  	 (is (type (car code)) 'code) (do (code->ccode port (+ name count)
   						       (car code))
  					  (self (cdr code) (+ 1 count)))
   	 (self (cdr code) (+ 1 count)))) (cdr (rep code)) 0)
  ;; Generate a gencode function, and produce calls that generate the
  ;; associated code.
  (disp (+ "value gencode_" name "(arc *c)\n{\n") port)
  (disp "  value insts, vcode;\n  Inst **ctp, *code;\n\n" port)
  (disp (+ "  insts = arc_mkvmcode(c, "
 	    (len (car (rep code))) ");\n") port)
  (disp "  code = (Inst*)&VINDEX(insts, 0);\n  ctp = &code;\n" port)
  (= close nil)
  ((afn (code index)
     (if (>= index (len code)) nil
	 (with (name (+ "gen_" (cut (coerce (code index) 'string) 1))
		     nargs (instnargs (code index)))
	   (disp (+ "  " name "(ctp") port)
	   (for i (+ 1 index) (+ index nargs)
		(disp (+ ", " (code i)) port))
	   (disp ");\n" port)
	   (self code (+ index nargs 1)))))
   (car (rep code)) 0)

  ;; Generate the whole code object and fill in the literals
  (disp (+ "  vcode = arc_mkcode(c, insts, arc_mkstringc(c, \"" name
	   "\", CNIL," (len (cdr (rep code))) ");\n") port)
  ((afn (code count)
     (if (no code) nil
	 (do (disp (+ "  CODE_LITERAL(vcode, " count ") = "
		      (literal->c (car code) name count) ";\n") port)
	     (self (cdr code) (+ 1 count))))) (cdr (rep code)) 0)
  (disp "  return(vcode);\n" port)
  (disp "}\n\n" port))
