;; Copyright (C) 2009 Rafael R. Sevilla
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

;; Compile an expression with all special syntax expanded.
(def compile (expr ctx env cont)
  (if (literalp expr) (compile-literal expr ctx cont)
      (isa expr 'sym) (compile-ident expr ctx env cont)
      (isa expr 'cons) (compile-list expr ctx env cont)
      (syntax-error "invalid expression" expr)))

;; Compile a literal value.
(def compile-literal (expr ctx cont)
  (do (if (no expr) (generate ctx 'inil)
	  (is expr t) (generate ctx 'itrue)
	  (fixnump expr) (generate ctx 'ildi (+ (* expr 2) 1))
	  (generate ctx 'ildl (find-literal expr ctx)))
      (compile-continuation ctx cont)))

(def compile-ident (expr ctx env cont)
  (do (let (found level offset) (find-var expr env)
	(if found (generate ctx 'ilde level offset)
	    (generate ctx 'ildg (find-literal expr ctx))))
      (compile-continuation ctx cont)))
  
(def compile-list (expr ctx env cont)
  (do (aif (spform (car expr)) (it expr ctx env cont)
	  (inlinablep (car expr)) (compile-inline expr ctx env cont)
	  (compile-apply expr ctx env cont))
      (compile-continuation ctx cont)))

(def spform (ident)
  (case ident
    if compile-if
    fn compile-fn
    quote compile-quote
    quasiquote compile-quasiquote
    assign compile-assign
    compose compile-compose
    andf compile-andf))

(def compile-if (expr ctx env cont)
  (do
    ((afn (args)
       (if (no args) (generate ctx 'inil)
	   ;; compile tail end if no additional
	   (no:cdr args) (compile (car args) ctx env nil)
	   (do (compile (car args) ctx env nil)
	       (let jumpaddr (code-ptr ctx)
		 (generate ctx 'ijf 0)
		 (compile (cadr args) ctx env nil)
		 (let jumpaddr2 (code-ptr ctx)
		   (generate ctx 'ijmp 0)
		   (code-patch ctx (+ jumpaddr 1) (- (code-ptr ctx) jumpaddr))
		   ;; compile the else portion
		   (self (cddr args))
		   ;; Fix target address of jump
		   (code-patch ctx (+ jumpaddr2 1)
			       (- (code-ptr ctx) jumpaddr2)))))))
     (cdr expr))
    (compile-continuation ctx cont)))

(def compile-fn (expr ctx env cont)
  (with (args (cadr expr) body (cddr expr) nctx (compiler-new-context))
    (let nenv (compile-args args nctx env)
      ;; The body of a fn works as an implicit do/progn
      (map [compile _ nctx nenv nil] body)
      (compile-continuation nctx t)
      ;; Convert the new context into a code object and generate
      ;; an instruction in the present context to load it as a
      ;; literal, and then create a closure using the code object
      ;; and the current environment.
      (let newcode (context->code nctx)
	(generate ctx 'ildl (find-literal newcode ctx))
	(generate ctx 'icls)
	(compile-continuation ctx cont)))))

;; This sets up the new environment given the arguments.
;; XXX - For now, this only handles ordinary, rest, and optional
;; arguments. This needs to be revised to provide for destructuring
;; arguments as well.
(def compile-args (args ctx env)
  (if (no args) env
      (let (nargs names rest)
	  ((afn (args count names)
	     (if (no args) (list count (rev names) nil)
		 (atom args) (list (+ 1 count) (rev (cons args names)) t)
		 (and (isa (car args) 'cons) (isnt (caar args) 'o))
		 (let dsb (dsb-args (car args))
		   (self (cdr args) (+ (len dsb) args)
			 (join (rev dsb) names) rest))
		 (self (cdr args) (+ 1 count)
		       (cons (car args) names))))
	   args 0 nil)
	;; Create a new environment frame
	(generate ctx 'ienv nargs)
	;; Generate instructions to bind the values of the
	;; of the arguments to the environment.
	(let realnames
	    ((afn (arg count rest rnames)
	       (if (and (no (cdr arg)) rest)
		   (do (generate ctx 'imvrarg count) ; XXX still undefined instruction
		       (rev (cons (car arg) rnames)))
		   (no arg) (rev rnames) ; done
		   ;; some optional argument
		   (and (isa (car arg) 'cons) (is (caar arg) 'o))
		   (let oarg (car arg)
		     (generate ctx 'imvoarg count) ; XXX still undefined
		     ;; To handle default parameters
		     (if (cddr oarg)
			 (do (generate ctx 'ilde 0 count)
			     ;; If we have a default value, fill it in
			     ;; if necessary.
			     (let jumpaddr (code-ptr ctx)
			       (generate ctx 'ijt 0)
			       (compile (car:cddr oarg) ctx env nil)
			       (generate ctx 'mvoarg count)
			       (code-patch ctx (+ jumpaddr 1)
					   (- (code-ptr ctx)
					      jumpaddr)))))
		     (self (cdr arg) (+ 1 count) rest
			   (cons (cadr oarg) rnames)))
		   ;; Destructuring bind argument.
		   (and (isa (car arg) 'cons) (isnt (caar arg) 'o))
		   (do (map [generate ctx _] (cddr arg))
		       (generate ctx 'mvarg count))
		   ;; ordinary arguments XXX - mvarg undefined instruction
		   (do (generate ctx 'imvarg count)
		       (self (cdr arg) (+ 1 count) rest
			     (cons (car arg) rnames)))))
	     names 0 rest nil)
	;; Create a new environment frame
	(cons (cons realnames nil) env)))))

;; Unroll and generate instructions for a destructuring bind of a list.
;; This function will return an assoc list of each element in the
;; original list followed by a list of car/cdr instructions that are
;; needed to get at that particular value given a copy of the original
;; list.
(def dsb-list (list)
  (let dsbinst
      ((afn (list instr ret)
	 (if (no list) ret
	     (isa list 'cons) (join
				(self (car list)
				      (cons 'icar instr) ret)
				(self (cdr list)
				      (cons 'icdr instr) ret))
	     (cons (cons list (cons 'idup (rev instr))) ret))) list)
    dsbinst))

(def compile-continuation (ctx cont)
  (if cont (generate ctx 'iret) ctx))

(def literalp (expr)
  (or (no expr)
      (is expr t)
      (isa expr 'char)
      (isa expr 'string)
      (isa expr 'int)
      (isa expr 'num)))

;; THE FUNCTIONS BELOW THIS MARK SHOULD BE PROVIDED FOR WITHIN THE
;; CARC RUNTIME CORE.  DEFINITIONS GIVEN HERE ARE FOR TESTING ONLY.

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
