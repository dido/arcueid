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
	(fixnump expr) (do (acc-gen expr ctx env cont) (acc-cont ctx cont))
	(isa expr 'sym) (acc-sym expr ctx env cont)
	(atom expr) (acc-literal expr ctx env cont)
	(isa expr 'cons) (acc-call expr ctx env cont)
	(acc-compile-error "invalid expression" expr))))

(def acc-find-var (var env (o level 0))
  (if (no env) '(nil 0 0)
      (aif ((afn (var frame idx)
		 (if (no frame) nil
		     (is (car frame) var) idx
		     (self var (cdr frame) (+ 1 idx)))) var (car env) 0)
	   `(t ,idx, it)
	   (acc-find-var var (cdr env) (+ 1 level)))))

(def acc-cont (ctx cont)
  (if cont (acc-gen ctx 'iret) ctx))

(def acc-sym (expr ctx env cont)
  (if (ssyntax expr) (acc (ssexpand expr) ctx env cont)
      (do (let (found level offset) (acc-find-var expr env)
	       (if found (acc-gen ctx 'ilde level offset)
		   (acc-gen ctx 'ildg (acc-getliteral expr ctx))))
	  (acc-cont ctx cont))))

(def acc-literal (expr ctx env cont)
  (acc-gen ctx 'ildl (acc-getliteral expr ctx))
  (acc-cont ctx cont))

(def acc-call (expr ctx env cont)
  (if (is (car expr) 'quote) (acc-getliteral (cadr expr) ctx)
      (is (car expr) 'quasiquote) (acc-qq expr ctx env cont)
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
