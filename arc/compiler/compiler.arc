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

(def acc (nexpr ctx env cont)
  (let expr (macex nexpr)
    (if (no expr) (do (acc-gen ctx 'inil) (acc-cont ctx cont))
	(is expr t) (do (acc-gen ctx 'itrue) (acc-cont ctx cont))
	(ssyntax expr) (ac (ssexpand expr) ctx env cont)
	(fixnump expr) (do (acc-gen expr ctx env cont) (acc-cont ctx cont))
	(isa expr 'sym) (acc-sym expr ctx env cont)
	(atom expr) (acc-literal expr ctx env cont)
	(isa expr 'cons) (acc-call expr ctx env cont)
	(acc-compile-error "invalid expression" expr))))

(def acc-cont (ctx cont)
  (if cont (acc-gen ctx 'iret) ctx))

(def acc-sym (expr ctx env cont)
  (if (ssyntax expr) (acc (ssexpand expr) ctx env cont)
      (do (let (found level offset) (acc-find-var expr env)
	       (if found (acc-gen ctx 'ilde level offset)
		   (acc-gen ctx 'ildg (acc-getliteral expr ctx))))
	  (acc-cont ctx cont))))

;; XXX - we need to handle atstrings
(def acc-literal (expr ctx env cont)
  (acc-gen ctx 'ildl (acc-getliteral expr ctx))
  (acc-cont ctx cont))

(def acc-call (expr ctx env cont)
  (if (is (car expr) 'quote) (acc-getliteral (cadr expr) ctx)
      ;; quasiquote is a macro, see below
      ;; (is (car expr) 'quasiquote) (acc-qq expr ctx env cont)
      (is (car expr) 'if) (acc-if (cdr expr) ctx env cont)
      (is (car expr) 'fn) (acc-fn (cadr expr) (cddr expr) ctx env cont)
      (is (car expr) 'assign) (acc-assign (cdr expr) ctx env cont)
      ;; the next three clauses could be removed without changing semantics
      ;; ... except that they work for macros (so prob should do this for
      ;; every elt of s, not just the car)
      (is (car (car expr)) 'compose) (acc (acc-decompose (cdar expr) (cdr expr)) ctx env cont)
      (is (car (car expr)) 'complement) (acc (list 'no (cons (cadar expr) (cdr expr))) ctx env cont)
      (is (car (car expr)) 'andf) (acc (acc-andf expr) ctx env cont)
      (isa expr 'cons) (acc-call (car expr) (cdr expr) ctx env cont)
      ((acc-compile-error "Bad object in expression" expr))))

(def acc-decompose (fns args)
  (if (no fns) `((fn vals (car vals)) ,@args)
      (no (cdr fns)) (cons (car fns) args)
      (list (car fns) (acc-decompose (cdr fns) args))))

(def acc-andf (expr)
  (let gs (map [uniq] (cdr expr))
    `((fn ,gs (and ,@(map [cons _ gs] (cdar expr)))) ,@(cdr expr))))

(def acc-if (expr ctx env cont)
  (do ((afn (args)
	    (if (no args) (acc-gen ctx 'inil)
		;; compile tail end if no additional
		(no:cdr args) (acc (car args) ctx env nil)
		(do (acc (car args) ctx env nil)
		    (let jumpaddr (acc-codeptr ctx)
		      (acc-gen ctx 'ijf 0)
		      (compile (cadr args) ctx env nil)
		      (let jumpaddr2 (acc-codeptr ctx)
			(acc-gen ctx 'ijmp 0)
			(acc-patch ctx (+ jumpaddr 1) (- (acc-codeptr ctx) jumpaddr))
			;; compile the else portion
			(self (cddr args))
			;; fix target address of jump
			(acc-patch ctx (+ jumpaddr2 1)
				   (- (code-ptr ctx) jumpaddr2))))))) expr)
      (acc-cont ctx cont)))

(def acc-assign (expr ctx env cont)
  ((afn (x)
	(if (no x) nil
	    (with (a (macex (car x)) val (cadr x))
	      (acc val ctx env cont)
	      (if (no a) (acc-compile-error "Can't rebind nil")
		  (is a t) (acc-compile-error "Can't rebind t")
		  (let (found level offset) (acc-find-var a env)
		       (if found (acc-gen ctx 'iste level offset)
			   (acc-gen ctx 'istg (acc-getliteral a ctx)))))
	      (self (cddr x))))) expr)
  (acc-cont ctx cont))

(def acc-fn (args body ctx env cont)
  (withs (nctx (acc-context) nenv (acc-args args nctx env))
	 (if (no body) (do (acc-gen nctx 'inil) (acc-gen nctx 'iret))
	     ((afn (body)
		   (acc (car body) nctx nenv (no:cdr body))
		   (self:cdr body)) body))
	 ;; Make a literal with the new code object from the new compilation
	 ;; context, and turn it into a closure.
	 (acc-gen ctx 'ildl (acc-getliteral (acc-ctx2code nctx) ctx))
	 (acc-gen ctx 'icls)
	 (acc-cont ctx cont)))

;; Find an environment frame number and offset given a variable name
(def acc-find-var (var env (o level 0))
  (if (no env) '(nil 0 0)
      (aif (alref (car env) var) `(t ,level ,it)
	   (acc-find-var var (cdr env) (+ 1 level)))))

;; Compile arguments. This will add instructions to do destructuring binds
;; as they are required.  It will return the new environment that has
;; the new arguments thus created.
(def acc-args (args ctx env)
  (if (no args) env	     ; just return the original env if no args
      (isa args 'sym)
      ;; If args is a single name, make an environment with a single name
      ;; and a list containing the name of the sole argument.
      (do1 (cons (list (list args 0)) env)
	   (acc-gen ctx 'ienvr 0 0 0))
      (~isa args 'cons) (acc-compile-error "invalid fn arg")
      ;; Iterate over all the args, obtaining counts of each type of
      ;; argument.  Ordinary args are processed here as well.
      (let envaddr (acc-codeptr ctx)
	(acc-gen ctx 'ienv (len regargs) 0 0)
	(let (nenv regargc dsbargc oargc restarg)
	  (acc-processargs args env ctx)
	  ;; After processing the arguments, we need to patch the instruction
	  ;; as needed.
	  (acc-patch ctx (+ 1 envaddr) regargc)
	  (acc-patch ctx (+ 2 envaddr) dsbargc)
	  (acc-patch ctx (+ 3 envaddr) oargc)
	  ;; If we have a restarg, change the instruction as well, and
	  ;; add the rest argument to the new env.
	  (if restarg (acc-patch ctx envaddr 'ienvr))
	  (cons nenv env)))))

;; Process arguments.  This will return a list containing the following:
;; 1. The new environment, not including the names of destructuring bind
;;    arguments (yet), but including the names of optional arguments.
;; 2. The number of regular arguments.
;; 3. The number of destructuring bind arguments.
;; 4. The number of optional arguments.
;; 5. If a rest arg was defined, t, else nil.
;; COMPATIBILITY WARNING: Arc3/Anarki permits arguments like this:
;; (fn (a (o b) c) ...). The "optional" b argument becomes not optional.
;; Arcueid produces an error with this construction.  I see the former
;; as a bug in reference Arc and since Arcueid is not supposed to be
;; a bug-compatible implementation of reference Arc, I'm not fixing this.
(def acc-processargs (args env ctx (o nenv) (o idx 0)
			   (o regargc 0) (o dsbargc 0)
			   (o optargc 0))
  (if (no args)
      ;; no more arguments, no restarg
      (list nenv regargc dsbargc optargc nil)
      ;; rest argument specified
      (isa args 'sym)
      ;; Add the rest argument to the new env by consing it.
      (list (cons (list args idx) nenv) regargc dsbargc optargc t)
      ;; Blank argument
      (~car args)
      ;; If we have a blank argument, the index increments, as does the
      ;; number of regular arguments, but no new name is bound in the
      ;; environment for that index.
      (acc-processargs (cdr args) ctx nenv (+ 1 idx) (+ 1 regargc)
		       dsbargc optargc)
      ;; Normal, named argument.
      (isa (car args) 'sym)
      ;; Add the name to the assoc with the current idx and
      ;; move forward.
      (if (is optargc 0)
	  (acc-processargs (cdr args) env ctx (cons (list (car args) idx) nenv)
			   (+ 1 idx) (+ 1 regargc) dsbargc optargc)
	  (acc-compile-error "non-optional arg found after optional args"))
      ;; Optional argument.
      (is (caar args) 'o)
      ;; For an optional argument, we need to load its value first and
      ;; then check to see if the argument is bound.
      (let (nil name default) (car args)
	   (acc-gen 'ilde0 idx)
	   (let jumpaddr (acc-codeptr ctx)
	     (acc-gen 'ijbnd 0)
	     ;; Compile default
	     (acc default ctx (cons nenv env) nil)
	     (acc-gen 'iste idx)
	     (acc-patch ctx (+ jumpaddr 1) (- (acc-codeptr ctx) jumpaddr))
	     (acc-processargs (cdr args) env ctx (cons (list name idx) nenv)
			      (+ 1 idx) regargc dsbargc (+ 1 optargc))))
      ;; Destructuring bind argument.
      (isa (car args) 'cons)
      (if (is optarg 0)
	  (apply acc-processargs
		 (acc-destructure (car args) (cdr args)
				  env ctx nenv idx regargc dsbargc optargc))
	  (acc-compile-error "non-optional arg found after optional args"))
      (acc-compile-error "invalid fn arg")))

;; Process destructuring binds.
;; To perform a destructuring bind, we begin by assuming that the
;; value to be unbound is in the value argument.  We traverse the
;; destructuring bind argument (which can be considered a binary
;; tree).  During the traversal, there are several possibilities:
;;
;; 1. We find a symbol.  In this case, we create an iste instruction
;;    that stores the value thus unbound to get there.
;; 2. We find a cons cell.  We decide to visit the car and cdr of the
;;    cell if one is not null.  In any case, we push the argument onto
;;    the stack (so we can pop it again later) generate a car or cdr
;;    instruction, and then call ourselves recursively with it.
(def acc-destructure (dsarg restargs env ctx nenv idx regargc dsbargc optargc)
  ;; XXX - fill me in!
)