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
(load "spec.arc")
(load "../compiler.arc")

(= test-literals
   (describe "Literal management function"
	     (prolog (= ctx (cons nil nil)))
	     (it "should add a literal not already present to the list"
		 (is (find-literal 1.0 ctx) 0))
	     (it "should add another literal to the list"
		 (is (find-literal 2.0 ctx) 1))
	     (it "should not add the same literal to the list twice"
		 (is (find-literal 1.0 ctx) 0))
	     (it "should handle symbols"
		 (is (find-literal 'some-symbol ctx) 2))
	     (it "should not add the symbol more than once"
		 (is (find-literal 'some-symbol ctx) 2))
	     (it "should have added three elements to the literal list"
		 (is (len (cdr ctx)) 3))))
(= test-codegen
   (describe "Code generation"
	     (prolog (= ctx '(nil . nil)))
	     (it "should generate an instruction with no arguments"
		 (do (generate ctx 'inop)
		     (and (is (len (car ctx)) 1)
			  (is (caar ctx) 'inop))))
	     (it "should generate an instruction with one argument"
		 (do (generate ctx 'ildi 31337)
		     (and (is (len (car ctx)) 3)
			  (is ((car ctx) 1) 'ildi)
			  (is ((car ctx) 2) 31337))))
	     (it "should generate an instruction with two arguments"
		 (do (generate ctx 'ilde 1 2)
		     (and (is (len (car ctx)) 6)
			  (is ((car ctx) 3) 'ilde)
			  (is ((car ctx) 4) 1)
			  (is ((car ctx) 5) 2))))))

(= test-compile-literal
   (describe "The compilation of literals"
	     (it "should compile character literals"
		 (do (= ctx '(nil . nil))
		     (compile #\a ctx nil nil)
		     (and (is ((car ctx) 0) 'ildl)
			  (is ((car ctx) 1) 0)
			  (is (len (cdr ctx)) 1)
			  (is ((cdr ctx) 0) #\a))))
	     (it "should compile string literals"
		 (do (= ctx '(nil . nil))
		     (compile "foo" ctx nil nil)
		     (and (is ((car ctx) 0) 'ildl)
			  (is ((car ctx) 1) 0)
			  (is (len (cdr ctx)) 1)
			  (is ((cdr ctx) 0) "foo"))))
	     (it "should compile nils"
		 (do (= ctx '(nil . nil))
		     (compile nil ctx nil nil)
		     (is ((car ctx) 0) 'inil)))
	     (it "should compile t"
		 (do (= ctx '(nil . nil))
		     (compile t ctx nil nil)
		     (is ((car ctx) 0) 'itrue)))
	     (it "should compile fixnums"
		 (do (= ctx '(nil . nil))
		     (compile 1234 ctx nil nil)
		     (and (is ((car ctx) 0) 'ildi)
			  (is ((car ctx) 1) (+ (* 1234 2) 1)))))
	     (it "should compile bignums"
		 (do (= ctx '(nil . nil))
		     (compile 340282366920938463463374607431768211456
			      ctx nil nil)
		     (and (is ((car ctx) 0) 'ildl)
			  (is ((car ctx) 1) 0)
			  (is (len (cdr ctx)) 1)
			  (is ((cdr ctx) 0)
			      340282366920938463463374607431768211456))))
	     (it "should compile flonums"
		 (do (= ctx '(nil . nil))
		     (compile 3.1415926535 ctx nil nil)
		     (and (is ((car ctx) 0) 'ildl)
			  (is ((car ctx) 1) 0)
			  (is (len (cdr ctx)) 1)
			  (< (abs (- ((cdr ctx) 0)
				     3.1415926535)) 1e-6))))))

(= test-environments
   (describe "Environment management functions"
	     (prolog (= env '(((foo bar baz) 1 2 3)
			      ((abc def ghi) 4 5 6)
			      ((quux fred jklm) 7 8 9))))
	     (it "should find variables in the environment"
		 (let (found frame idx) (find-var 'foo env)
		   (and found
			(is frame 0)
			(is idx 0))))
	     (it "should find more (1) variables in the environment"
		 (let (found frame idx) (find-var 'bar env)
		   (and found
			(is frame 0)
			(is idx 1))))
	     (it "should find more (2) variables in the environment"
		 (let (found frame idx) (find-var 'baz env)
		   (and found
			(is frame 0)
			(is idx 2))))
	     (it "should find more (0) variables in deeper environment frames (1)"
		 (let (found frame idx) (find-var 'abc env)
		   (and found
			(is frame 1)
			(is idx 0))))
	     (it "should find more (1) variables in deeper environment frames (1)"
		 (let (found frame idx) (find-var 'def env)
		   (and found
			(is frame 1)
			(is idx 1))))
	     (it "should find more (2) variables in deeper environment frames (1)"
		 (let (found frame idx) (find-var 'ghi env)
		   (and found
			(is frame 1)
			(is idx 2))))
	     (it "should find more (0) variables in deeper environment frames (2)"
		 (let (found frame idx) (find-var 'quux env)
		   (and found
			(is frame 2)
			(is idx 0))))
	     (it "should find more (1) variables in deeper environment frames (2)"
		 (let (found frame idx) (find-var 'fred env)
		   (and found
			(is frame 2)
			(is idx 1))))
	     (it "should find more (2) variables in deeper environment frames (2)"
		 (let (found frame idx) (find-var 'jklm env)
		   (and found
			(is frame 2)
			(is idx 2))))
	     (it "should not find variables which are not there"
		 (let (found frame idx) (find-var 'nopq env)
		   (no found)))))

(= test-compile-ident
   (describe "The compilation of identifiers"
	     (prolog (do (= env '(((foo bar baz) 1 2 3)
				  ((abc def ghi) 4 5 6)
				  ((quux fred jklm) 7 8 9)))
			 (= ctx `(nil . nil))))
	     (it "should compile an identifier referring to the environment"
		 (do (compile 'foo ctx env nil)
		     (and (is ((car ctx) 0) 'ilde)
			  (is ((car ctx) 1) 0)
			  (is ((car ctx) 2) 0))))
	     (it "should compile another identifier referring to the environment"
		 (do (scar ctx nil)
		     (compile 'fred ctx env nil)
		     (and (is ((car ctx) 0) 'ilde)
			  (is ((car ctx) 1) 2)
			  (is ((car ctx) 2) 1))))
	     (it "should compile an identifier referring to the global environment"
		 (do (scar ctx nil)
		     (compile 'xyzzy ctx env nil)
		     (and (is ((car ctx) 0) 'ildg)
			  (is ((car ctx) 1) 0)
			  (is (len (cdr ctx)) 1)
			  (is ((cdr ctx) 0) 'xyzzy))))))

(= test-compile-if
   (describe "The compilation of the if special form"
	     (it "should compile an empty if statement to nil"
		 (do (= ctx '(nil . nil))
		     (compile '(if) ctx nil nil)
		     (is ((car ctx) 0) 'inil)))
	     (it "should compile (if x) to simply x"
		 (do (= ctx '(nil . nil))
		     (compile '(if 4) ctx nil nil)
		     (and (is ((car ctx) 0) 'ildi)
			  (is ((car ctx) 1) (+ (* 4 2) 1)))))
	     (it "should compile a full if statement properly"
		 (do (= ctx '(nil . nil))
		     (compile '(if t 1 2) ctx nil nil)
		     (iso (car ctx) '(itrue ijf 6 ildi 3 ijmp 4 ildi 5))))
	     (it "should compile a partial if statement properly"
		 (do (= ctx '(nil . nil))
		     (compile '(if t 1) ctx nil nil)
		     (iso (car ctx) '(itrue ijf 6 ildi 3 ijmp 3 inil))))
	     (it "should compile a compound if statement properly"
		 (do (= ctx '(nil . nil))
		     (compile '(if t 1 2 3 4 5 6) ctx nil nil)
		     (iso (car ctx) '(itrue ijf 6 ildi 3 ijmp 20
					    ildi 5 ijf 6 ildi 7
					    ijmp 12 ildi 9 ijf 6
					    ildi 11 ijmp 4 ildi 13))))))

(= test-dsb-list
   (describe "Tests for the destructuring bind instruction generator"
	     (it "should generate destructuring bind instructions (1)"
		 (let res (dsb-list '(a b (c d) e))
		   (and (is (len res) 5)
			(iso (assoc 'a res) '(a idup icar))
			(iso (assoc 'b res) '(b idup icdr icar))
			(iso (assoc 'c res) '(c idup icdr icdr icar icar))
			(iso (assoc 'd res) '(d idup icdr icdr icar icdr icar))
			(iso (assoc 'e res) '(e idup icdr icdr icdr icar)))))))

(= test-compile-fn
   (describe "The compilation of the fn special form"
	     (it "should compile a function with no arguments"
		 (do (= ctx '(nil . nil))
		     (compile '(fn () 1) ctx nil nil)
		     (iso (car (rep ((cdr ctx) 0))) '(ildi 3 iret))))
	     (it "should compile a function with one argument"
		 (do (= ctx '(nil . nil))
		     (compile '(fn (x) x) ctx nil nil)
		     (iso (car (rep ((cdr ctx) 0)))
			  '(ienv 1 imvarg 0 ilde 0 0 iret))))
	     (it "should compile a function with two arguments"
		 (do (= ctx '(nil . nil))
		     (compile '(fn (x y) x y) ctx nil nil)
		     (and  (iso (car ctx) '(ildl 0 icls))
			   (iso (car (rep ((cdr ctx) 0)))
				'(ienv 2 imvarg 0 imvarg 1
				       ilde 0 0 ilde 0 1 iret)))))
	     (it "should compile a function with a rest argument"
		 (do (= ctx '(nil . nil))
		     (compile '(fn (x y . z) x y z) ctx nil nil)
		     (and (iso (car ctx) '(ildl 0 icls))
			  (iso (car (rep ((cdr ctx) 0)))
			       '(ienv 3 imvarg 0 imvarg 1
				      imvrarg 2 ilde 0 0 ilde 0 1
				      ilde 0 2 iret)))))
	     (it "should compile a function with an optional argument"
		 (do (= ctx '(nil . nil))
		     (compile '(fn (x (o y)) x y) ctx nil nil)
		     (and (iso (car ctx) '(ildl 0 icls))
			  (iso (car (rep ((cdr ctx) 0)))
			       '(ienv 2 imvarg 0 imvoarg 1
				      ilde 0 0 ilde 0 1 iret)))))
	     (it "should compile a function with an optional argument with a default value"
		 (do (= ctx '(nil . nil))
		     (compile '(fn (x (o y 1)) x y) ctx nil nil)
		     (and (iso (car ctx) '(ildl 0 icls))
			  (iso (car (rep ((cdr ctx) 0)))
			       '(ienv 2 imvarg 0 imvoarg 1
				      ilde 0 1 ijt 6 ildi 3
				      mvoarg 1 ilde 0 0 ilde 0 1 iret)))))))
(print-results (test-literals))
(print-results (test-codegen))
(print-results (test-compile-literal))
(print-results (test-environments))
(print-results (test-compile-ident))
(print-results (test-compile-if))
(print-results (test-compile-fn))
(print-results (test-dsb-list))
