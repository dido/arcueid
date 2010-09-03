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
    idup 36
    icls 37
    (err "Unrecognized opcode" code)))

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
    code (+ "gencode_" name "_" count "(c)")
    sym (+ "arc_intern_cstr(c, \"" (coerce lit 'string) "\")")
    (err "Unrecognized type of literal " lit " with type " (type lit))))

(def code->ccode (port name code)
  ;; Look for literals in the code which represent code objects.  Call
  ;; ourselves recursively on those so that the code for them gets
  ;; generated.
  ((afn (code count)
     (if (no code) nil
  	 (is (type (car code)) 'code) (do (code->ccode port (+ name "_" count)
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
  (disp "}\n\n" port)
  nil)

;; We will use, for the moment, a very simple serialization scheme for
;; values herein.  We'll later develop a much more sophisticated
;; serialization mechanism in the future, but this will do as a version
;; 0.0.0 for the Compact Information Expression Layer (CIEL). (sorry,
;; yet another Tsukihime reference)
;;
;; The serialized file starts with the following two bytes:
;; 0xC1 0xE1 -- Every CIEL file begins with this.  This is followed by
;; the version number of the CIEL file with the major, minor, and
;; sub-version number as 16-bit little-endian numbers each.  The code
;; herein is only capable of generating version 0.0.0 CIEL data, so
;; the magic numbers at the start of the file should be C1 E1 00 00 00
;; 00 00 00.
;;

;; Encode an unsigned value from 0 to 9223372036854775807 into an
;; 8-byte 64-bit little-endian representation.  Set the high bit
;; to 1 if last is true.  This will not perform range checks on the
;; argument.
(def int-frag (last val)
  (list (mod val 256)					      ;256^0
	(mod (trunc (/ val 256)) 256)			      ;256^1
	(mod (trunc (/ val 65536)) 256)			      ;256^2
	(mod (trunc (/ val 16777216)) 256)		      ;256^3
	(mod (trunc (/ val 4294967296)) 256)		      ;256^4
	(mod (trunc (/ val 1099511627776)) 256)		      ;256^5
	(mod (trunc (/ val 281474976710656)) 256)	      ;256^6
	(+ (if last 128 0)
	   (mod (trunc (/ val 72057594037927936)) 256))))     ;256^7

;; encode an unsigned integer value x
(def euint (x (o acc '()))
  (if (<= x 0) acc
      (let quot (trunc (/ x 9223372036854775808))
	(euint quot
	       (+ acc (int-frag (<= quot 0)
				(mod x 9223372036854775808)))))))

;; Encode a signed integer value x
(def eint (x)
   (euint (abs x) (if (>= x 0) '(#x2b) '(#x2d))))

;; Encode a string x.  Encoding may not be perfect for general UTF-8
;; strings.
(def estr (x)
  (+ (euint (len x)) (list x)))

;; Encode a flonum into an IEEE-754 double precision binary representation.
;; This will return a 64-bit integer.  There are some contortions it has
;; to go through to do this since Arc3 doesn't have any operators for
;; extracting the mantissa and exponent directly, and as such there are
;; some risks with regard to precision loss.  The same issues should not
;; trouble the C equivalent to this code.
(def eflonum (x)
  (int-frag
    nil
    (if (is x 0.0) 0
	(withs (rexp (/ (log (abs x)) (log 2))
		     ;; We need to be able to round towards negative
		     ;; infinity here.  This means that we drop all
		     ;; fractions for positive numbers, and round up
		     ;; for negative numbers.  Arc doesn't provide
		     ;; this kind of function out of the box.
		     exponent (if (and (< rexp 0.0)
				       (> (abs (- rexp
						  (trunc rexp)))
					  1e-6))
				  (- (trunc rexp) 1)
				  (trunc rexp))
		     sign (if (>= x 0.0) 0 1)
		     val (+ (* sign 2048) (+ (trunc exponent) 1023))
		     ;; We subtract one and multiply by two to
		     ;; remove the leading 1 which is implicit
		     mantissa (* (- (/ (abs x)
				       (expt 2.0 exponent)) 1) 2))
	  ;; Extract the bits of the mantissa by repeatedly
	  ;; multiplying it by 2.  If we ever get a result greater
	  ;; than 1, we have a one bit, otherwise the bit is zero.
	  ;; Accumulate the bits in val.
	  ((afn (x val num)
	     (if (is num 0) val
		 (> x 1.0) (self (* (- x 1) 2)
				 (+ (* val 2) 1) (- num 1))
		 (self (* x 2) (* val 2)
		       (- num 1)))) mantissa val 52)))))

;; Symbol to Ciel bytecode
(def cbytecode (code)
  (case code
    gnil 0				;load nil
    gtrue 1				;load true
    gint 2				;load integer
    gflo 3				;load float
    gchar 4				;load character
    gstr 5				;load string
    gsym 6				;load symbol
    gtab 7				;load empty table (not used)
    crat 8				;load rational
    ccomplex 9				;load complex
    ctadd 10				;add value to table
    ccons 11				;cons two arguments
    cannotate 12			;annotate a value with a symbol
    xdup 13				;duplicate top of stack
    xmst 14 				;memo store
    xmld 15))				;memo load

(def writelist (port arg)
  (each x arg
    (case (type x)
      int (writeb x port)
      string (disp x port)
      (err "invalid type for writelist: " (type x) " " x))))

(def writebc (port op . rest)
  (writeb (cbytecode op) port)
  (each x rest
    (writelist port x)))

(def lit-marshal (port lit count lit->memo)
  (case (type lit)
    char (writebc port 'gchar (int-frag nil (coerce lit 'int)))
    string (writebc port 'gstr (estr lit))
    sym  (writebc port 'gsym (estr (coerce lit 'string)))
    int (writebc port 'gint (eint lit))
    num (writebc port 'gflo (eflonum lit))
    ;; load the code object from memo
    code (writebc port 'xmld (euint (lit->memo count)))
    (err "Unrecognized type of literal " lit " with type " (type lit))))

(def code-marshal (port name code (o memoidx 0) (o header t))
  (with (lit->memo (table))
    (if header
	;; Write the header
	(do (writeb #xc1 port)
	    (writeb #xe1 port)
	    ;; Major 0
	    (writeb #x00 port)
	    (writeb #x00 port)
	    ;; Minor 0
	    (writeb #x00 port)
	    (writeb #x00 port)
	    ;; Sub 0
	    (writeb #x00 port)
	    (writeb #x00 port)))
    ;; Look for literals in the code which represent code objects.  Call
    ;; ourselves recursively on those so that the code for them gets
    ;; generated.  Store each inside the memo, and write the mapping
    ;; between literal index and memo index to lit->memo.
    (with (lits (cdr (rep code)) count 0)
      (each x lits
	(if (is (type x) 'code)
	    (do (= memoidx (code-marshal port (+ name "_" count) x memoidx nil))
		;; Generate code to store the generated code object at the top
		;; of the CIEL stack into the memo.
		(writebc port 'xmst (euint memoidx))
		;; Save the index into the literal-memo table
		(= (lit->memo count) memoidx)
		;; Next memo index value
		(++ memoidx)))
	(++ count)))
    (writebc port 'gstr)
    (map [writeb _ port]
	 ;; Now, generate a list with the bytecode, as bytes
	 ((afn (code paramf nparam clist)
	    (if (no code)
		clist
		paramf
		(let np (- nparam 1)
		  (self (cdr code) (> np 0) np (+ clist (eint (car code)))))
		(let na (instnargs (car code))
		  (self (cdr code) (> na 0) na
			(+ clist (list (bytecode (car code))))))))
	  (car (rep code)) nil 0 '()))
    ;; And then generate an array of literals
    ((afn (lits count)
       (if (no lits) (writebc port 'gnil)
	   (do (self (cdr lits) (+ count 1))
	       (lit-marshal port (car lits) count lit->memo)
	       (writebc port 'ccons)))) (cdr (rep code)) 0)
    ;; cons the bytecode string and the literals together
    (writebc port 'ccons)
    ;; load a symbol with the code tag
    (writebc port 'gsym (estr "code"))
    ;; and annotate the consed bytecode and literals
    (writebc port 'cannotate)
    ;; return the value of memoidx
    memoidx))

