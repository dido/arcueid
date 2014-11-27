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
;; Some functions normally defined in ac.scm rewritten in pure Arc


;; Pure Arc ssyntax
(def ssyntax (x)
  (let sscharp
      (afn (str i)
	   (and (>= i 0)
		(or (let c (str i)
		      (or (is c #\:) (is c #\~) (is c #\&)
			  ; (is c #\_)
			  (is c #\.) (is c #\!)))
		    (self str (- i 1)))))
    (and (isa x 'sym)
	 (no (or (is x '+) (is x '++) (is x '_)))
	 (let name (coerce x 'string)
	   (sscharp name (- (len name) 1))))))

;; Ought to be built-in maybe. Looks horribly inefficient.
(def sstokens (s (o sep whitec) (o keepsep))
  (let test testify.sep
    (rev:map [coerce _ 'string]
             (map rev
                  (loop (cs  (coerce s 'cons)
			     toks  nil
			     tok  nil
			     lastsep nil)
			(if no.cs
			    (consif tok (consif lastsep toks))
			    (test car.cs)
			    (recur cdr.cs
				   (consif tok (consif lastsep toks))
				   nil (if keepsep (coerce car.cs 'string)
					   lastsep))
			    (recur cdr.cs toks (cons car.cs tok) lastsep)))))))

;; Pure Arc ssexpand.  
(def ssexpand (s)
  ;; XXX - these sub-functions need to be defined
  (withs
   (insymp
    (afn (char s (o index 0))
	 (if (isa s 'sym) (self char (coerce s 'string) 0)
	     (>= index (len s)) nil
	     (is char (s index)) t
	     (self char s (+ 1 index))))
    expand-compose
    (fn (s)
	(let elts (map [if (is #\~ (_ 0))
			   (if (is (len _) 1) 'no
			       `(complement ,(sym (substring _ 1))))
			   (sym _)] (sstokens (string s) #\:))
	  (if (no (cdr elts)) (car elts)
	      (cons 'compose elts))))
    build-sexpr
    (afn (toks orig)
	 (if no.toks 'get
	     (no (cdr toks)) (sym (car toks))
	     (list (self (cddr toks) orig)
		   (if (is (cadr toks) "!")
		       (list 'quote (sym (car toks)))
		       (or (is (car toks) ".") (is (car toks) "!"))
		       (err "Bad ssyntax" orig)
		       (sym (car toks))))))
    expand-sexpr
    (fn (s)
	(build-sexpr (rev (sstokens (coerce s 'string)
				  [or (is _ #\.) (is _ #\!)] t)) s))
    expand-and
    (fn (s)
	(let elts (map [sym _] (sstokens (string s) #\&))
	  (if (no (cdr elts)) (car elts)
	      (cons 'andf elts)))))
   ((if (or (insymp #\: s) (insymp #\~ s)) expand-compose
	(or (insymp #\. s) (insymp #\! s)) expand-sexpr
	(insymp #\& s) expand-and
	(error "Unknown ssyntax" s)) s)))

;; Pure Arc macex
(def macex (e)
  (if (atom e)
      e
      (let op (and (atom (car e)) (eval (car e)))
        (if (isa op 'mac)
            (apply (rep op) (cdr e))
            e))))

;; https://bitbucket.org/fallintothis/qq/raw/04a5dfbc592e5bed58b7a12fbbc34dcd5f5
;; CL-style quasiquote (ported from GNU clisp 2.47, backquote.lisp).
;; Rewritten to be more Arc-like May 2010

;; Like join, except it can create dotted lists if the last arg is an atom

(def append args
  (if (no args)
       nil
      (no (cdr args))
       (car args)
      (let a (car args)
        (if (no a)
            (apply append (cdr args))
            (cons (car a) (apply append (cdr a) (cdr args)))))))

;; Like list, except that the last cons of the constructed list is dotted

(def dotted-list xs
  (if (no (cdr xs))
      (car xs)
      (rreduce cons xs)))

(def proper (xs) (and (alist xs) (~dotted xs)))

(def qq-non-list-splice-error (expr)
  (err (+ "The syntax `,@" (tostring:write expr) " is invalid")))

(def qq-dotted-splice-error (expr)
  (err (+ "The syntax `(... . ,@" (tostring:write expr) ") is invalid")))

;; Quasiquotation

(mac quasiquote (expr)
  (qq-expand expr))

;; Since quasiquote handles 'unquote and 'unquote-splicing, we can define those
;; as macros down to errors, as they're automatically outside of a quasiquote.

(mac unquote (expr)
  (list 'err "unquote not allowed outside of a quasiquote:" expr))

(mac unquote-splicing (expr)
  (list 'err "unquote-splicing not allowed outside of a quasiquote:" expr))


;; Recursive Expansion Engine

;; The behaviour is more-or-less dictated by the Common Lisp HyperSpec's general
;; description of backquote:
;;
;;   `atom/nil -->  'atom/nil
;;   `,expr     -->  expr
;;   `,@expr    -->  error
;;   ``expr     -->  `expr-expanded
;;   `list-expr -->  expand each element & handle dotted tails:
;;       `(x1 x2 ... xn)     -->  (append y1 y2 ... yn)
;;       `(x1 x2 ... . xn)   -->  (append y1 y2 ... 'xn)
;;       `(x1 x2 ... . ,xn)  -->  (append y1 y2 ... xn)
;;       `(x1 x2 ... . ,@xn) -->  error
;;     where each yi is the output of (qq-transform xi).

(def qq-expand (expr)
  (if (atom expr)
      (list 'quote expr)
      (case (car expr)
        unquote          (cadr expr)
        unquote-splicing (qq-non-list-splice-error (cadr expr))
        quasiquote       (list 'quasiquote (qq-expand (cadr expr)))
                         (qq-appends (qq-expand-list expr)))))

;; Produce a list of forms suitable for append.
;; Note: if we see 'unquote or 'unquote-splicing in the middle of a list, we
;; assume it's from dotting, since (a . (unquote b)) == (a unquote b).
;; This is a "problem" if the user does something like `(a unquote b c d), which
;; we interpret as `(a . ,b).

(def qq-expand-list (expr)
  (and expr
       (if (atom expr)
           (list (list 'quote expr))
           (case (car expr)
             unquote          (list (cadr expr))
             unquote-splicing (qq-dotted-splice-error (cadr expr))
                              (cons (qq-transform (car expr))
                                    (qq-expand-list (cdr expr)))))))

;; Do the transformations for elements in qq-expand-list that aren't the dotted
;; tail.  Also, handle nested quasiquotes.

(def qq-transform (expr)
  (case (acons&car expr)
    unquote          (qq-list (cadr expr))
    unquote-splicing (cadr expr)
    quasiquote       (qq-list (list 'quasiquote (qq-expand (cadr expr))))
                     (qq-list (qq-expand expr))))


;; Expansion Optimizer

;; This is mainly woven through qq-cons and qq-append.  It can run in a
;; non-optimized mode (where lists are always consed at run-time), or
;; optimizations can be done that reduce run-time consing / simplify the
;; macroexpansion.  For example,
;;   `(,(foo) ,(bar)) 
;;      non-optimized --> (append (cons (foo) nil) (cons (bar) nil))
;;      optimized     --> (list (foo) (bar))

;; Optimization is enabled by default, but can be turned off for debugging.

(set optimize-cons* optimize-append*)

(def toggle-optimize ()
  (= optimize-cons*   (no optimize-cons*)
     optimize-append* (no optimize-append*)))

;; Test whether the given expr may yield multiple list elements.
;; Note: not only does ,@x splice, but so does ,,@x (unlike in vanilla Arc)

(def splicing (expr)
  (case (acons&car expr)
    unquote-splicing t
    unquote          (splicing (cadr expr))))

(def splicing->non (expr)
  (if (splicing expr) (list 'append expr) expr))

(def quoted-non-splice (expr)
  (and (caris expr 'quote)
       (single (cdr expr))
       (~splicing (cadr expr))))

(def qq-cons (expr1 expr2)
  ;; assume expr2 is non-splicing
  (let operator (if (splicing expr1) 'dotted-list 'cons)
    (if (no optimize-cons*)
         (list operator expr1 expr2)
        (and (~splicing expr1) (literal expr1) (no expr2))
         (list 'quote (list (eval expr1)))
        (no expr2)
         (list 'list expr1)
        (atom expr2)
         (list operator expr1 expr2)
        (caris expr2 'list)
         (dotted-list 'list expr1 (cdr expr2))
        (and (quoted-non-splice expr1) (quoted-non-splice expr2))
         (list 'quote (cons (cadr expr1) (cadr expr2)))
        (list operator expr1 expr2))))

(def qq-list (expr) (qq-cons expr nil))

(def qq-append (expr1 (o expr2))
  (if (no optimize-append*)
       (list 'append expr1 expr2)
      (no expr1)
       expr2
      (no expr2)
       expr1
      (caris expr1 'list)
       (if (single expr1)
            expr2
           (single (cdr expr1))
            (qq-cons (cadr expr1) expr2)
           (cons 'dotted-list (append (cdr expr1) (list expr2))))
      (and (quoted-non-splice expr1)
           (proper (cadr expr1))
           (~caris (cadr expr1) 'unquote)) ; since unquote expects only 1 arg
       (rreduce (fn (x xs) (qq-cons (list 'quote x) xs))
                (+ (cadr expr1) (list (splicing->non expr2))))
      (caris expr2 'append)
       (dotted-list 'append expr1 (cdr expr2))
      (list 'append expr1 expr2)))

(def qq-appends (exprs) (splicing->non (rreduce qq-append exprs)))
