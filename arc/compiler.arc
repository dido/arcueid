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
      (compile-error "invalid expression" expr)))

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
	   (inline-func (car expr)) (it expr ctx env cont)
	   (compile-apply expr ctx env cont))
      (compile-continuation ctx cont)))

(def spform (ident)
  (case ident
    if compile-if
    fn compile-fn
    quote compile-quote
    quasiquote compile-quasiquote
    assign compile-assign))

(def inline-func (ident)
  (case ident
    cons (fn (expr ctx env cont) (compile-inline 'icons 2 expr ctx env cont))
    car (fn (expr ctx env cont) (compile-inline 'icar 1 expr ctx env cont))
    cdr (fn (expr ctx env cont) (compile-inline 'icdr 1 expr ctx env cont))
    scar (fn (expr ctx env cont) (compile-inline 'iscar 2 expr ctx env cont))
    scdr (fn (expr ctx env cont) (compile-inline 'iscdr 2 expr ctx env cont))
    is (fn (expr ctx env cont) (compile-inline 'iis 2 expr ctx env cont))
    + (fn (expr ctx env cont) (compile-inlinen 'iadd expr ctx env cont 0))
    - (fn (expr ctx env cont) (compile-inlinen2 'isub expr ctx env cont 0))
    * (fn (expr ctx env cont) (compile-inlinen 'imul expr ctx env cont 1))
    / (fn (expr ctx env cont) (compile-inlinen2 'idiv expr ctx env cont 1))))

(def compile-inline (instr narg expr ctx env cont)
  ((afn (xexpr count)
     (if (and (<= count 0) (no xexpr)) nil
	 (no expr) (compile-error "procedure " (car expr) " expects " narg
				  " arguments")
	 (do (compile (car xexpr) ctx env cont)
	     (if (no (cdr xexpr)) nil
		 (generate ctx 'ipush))
	     (self (cdr xexpr) (- count 1))))) (cdr expr) narg)
  (generate ctx instr)
  (compile-continuation ctx cont))

(def compile-inlinen (instr expr ctx env cont base)
  (compile base ctx env cont)
  (walk (cdr expr) [do (generate ctx 'ipush)
		       (compile _ ctx env cont)
		       (generate ctx instr)])
  (compile-continuation ctx cont))

(def compile-inlinen2 (instr expr ctx env cont base)
  (withs (xexpr (cdr expr) xelen (len xexpr))
    (if (is xelen 0) (compile-error (car expr) ": expects at least one argument, given 0")
	(is xelen 1) (compile-inlinen instr expr ctx env cont base)
	(compile-inlinen instr (cons (car expr) (cdr xexpr)) ctx env cont
			 (car xexpr)))))

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
		   (code-patch ctx (+ jumpaddr 1) (- (code-ptr ctx) jumpaddr)) c
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

;; This generates code to set up the new environment given the arguments.
(def compile-args (args ctx env)
  (if (no args) env
      (let (nargs names rest)
	  ((afn (args count names)
	     (if (no args) (list count (rev names) nil)
		 (atom args) (list (+ 1 count) (rev (cons args names)) t)
		 (and (isa (car args) 'cons) (isnt (caar args) 'o))
		 (let dsb (dsb-list (car args))
		   (self (cdr args) (+ (len dsb) count)
			 (join (rev dsb) names)))
		 (self (cdr args) (+ 1 count)
		       (cons (car args) names))))
	   args 0 nil)
	;; Create a new environment frame of the appropriate size
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
			       (generate ctx 'imvoarg count)
			       (code-patch ctx (+ jumpaddr 1)
					   (- (code-ptr ctx)
					      jumpaddr)))))
		     (self (cdr arg) (+ 1 count) rest
			   (cons (cadr oarg) rnames)))
		   ;; Destructuring bind argument.
		   (and (isa (car arg) 'cons) (isnt (caar arg) 'o))
		   (do (map [generate ctx _] (cdr:car arg))
		       (generate ctx 'imvarg count)
		       ;; Check if this is the last destructuring bind
		       ;; in the group.  If so, generate a pop instruction
		       ;; to discard the argument on the stack.
		       (let next (cadr arg)
			 (if (no (and (isa next 'cons) (isnt (car next) 'o)))
			     (generate ctx 'ipop)))
		       (self (cdr arg) (+ 1 count) rest
			     (cons (caar arg) rnames)))
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
;; list.  XXX: This is a rather naive algorithm, and we can probably
;; do better by generating code to load and store all values as we visit
;; them while traversing the conses, but I think it will do nicely for
;; a simple interpreter.
(def dsb-list (list)
  (let dsbinst
      ((afn (list instr ret)
	 (if (no list) ret
	     (isa list 'cons) (join
				(self (car list)
				      (cons 'icar instr) ret)
				(self (cdr list)
				      (cons 'icdr instr) ret))
	     (cons (cons list (cons 'idup (rev instr))) ret))) list nil nil)
    dsbinst))

;; Compile a quoted expression.  This is fairly trivial: all that it
;; actually does is convert the expression passed as argument into
;; a literal which then gets folded into the literal list, and an
;; instruction is added to load ito onto the stack.
(def compile-quote (expr ctx env cont)
  (generate ctx 'ildl (find-literal (cadr expr) ctx))
  (compile-continuation ctx cont))

;; Compile a quasiquoted expression.  This is not so trivial.
;; Basically, it requires us to reverse the list that is to be
;; quasiquoted, and then each element in its turn is quoted and consed
;; until the end of the list is encountered.  If an unquote is
;; encountered, the contents of the unquote are compiled, so that the
;; top of stack contains the results, and this too is consed to the
;; head of the list.  If an unquote-splicing is encountered, the
;; contents are also compiled, but the results are spliced.  The
;; reversing is necessary so that when the lists are consed together
;; they appear in the correct order.
(def compile-quasiquote (expr ctx env cont)
  (generate ctx 'inil)
  ((afn (rexpr)
     (if (no rexpr) nil
	 (and (isa (car rexpr) 'cons) (is (caar rexpr) 'unquote))
	 (do (generate ctx 'ipush)
	     (compile (car:cdr:car rexpr) ctx env cont)
	     (generate ctx 'cons)
	     (self (cdr rexpr)))
	 (and (isa (car rexpr) 'cons) (is (caar rexpr)
					  'unquote-splicing))
	 (do (generate ctx 'ipush)
	     (compile (car:cdr:car rexpr) ctx env cont)
	     (generate ctx 'ispl)	; XXX - undefined instruction for splicing a list
	     (self (cdr rexpr)))
	 (do (generate ctx 'ipush)
	     (generate ctx 'ildl (find-literal (car rexpr) ctx))
	     (generate ctx 'cons)
	     (self (cdr rexpr)))))
   (rev (cadr expr)))
  (compile-continuation ctx cont))

(def compile-apply (expr ctx env cont)
  (with (fname (car expr) args (cdr expr) contaddr (code-ptr ctx))
    (generate ctx 'icont 0)
    (walk (rev args) [do (compile _ ctx env cont)
			 (generate ctx 'ipush)])
    (compile fname ctx env cont)
    (generate ctx 'iapply (len args))
    (code-patch ctx (+ contaddr 1) (- (code-ptr ctx) contaddr))
    (compile-continuation ctx cont)))

;; Expand a macro.  This is taken from arc.arc.  This causes compile-time
;; expansion of macros.
(def cmacex (e)
  (if (atom e) e
      (let op (and (atom (car e)) (eval (car e)))
        (if (isa op 'mac) (apply (rep op) (cdr e))
            e))))

;; Compile an assign special form.  Called in previous versions of Arc
;; set, the assign form takes symbol-value pairs in its argument and
;; assigns them.
(def compile-assign (expr ctx env cont)
  ((afn (x)
     (if (no x) nil
	 (with (a (cmacex (car x)) val (cadr x))
	   (compile val ctx env cont)
	   (if (no a) (compile-error "Can't rebind nil")
	       (is a 't) (compile-error "Can't rebind t")
	       (let (found level offset) (find-var a env)
		 (if found (generate ctx 'iste level offset)
		     (generate ctx 'istg (find-literal a ctx)))))
	   (self (cddr x))))) (cdr expr))
  (compile-continuation ctx cont))

(def compile-continuation (ctx cont)
  (if cont (generate ctx 'iret) ctx))

(def literalp (expr)
  (or (no expr)
      (is expr t)
      (isa expr 'char)
      (isa expr 'string)
      (isa expr 'int)
      (isa expr 'num)))
