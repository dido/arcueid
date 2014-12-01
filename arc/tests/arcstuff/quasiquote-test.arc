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
(mac plusall args `(+ ,@args))
(mac overwrite (after) `(+ ,after 3))
(= x '(a b c))
(= a 10 b 20 c 30)
(= aa (list 10) bb (list 20) cc (list 30))
(= xx '(aa bb cc))

(register-test
 '(suite "quasiquote"
	 ("quasi-quotation"
          '`qqfoo
          (quasiquote qqfoo))

	 ("unquote"
          ',uqfoo
          (unquote uqfoo))

        ("unquote-splicing"
          ',@uqsfoo
          (unquote-splicing uqsfoo) )

        ("quasi-quotation is just like quotation"
          `qqqfoo
          qqqfoo )

	;; Changed from C. Dalton's tests because Arcueid expands
	;; nested quasiquotes.
        ("quasi-quote quasi-quote"
	 ``double-qq
	 'double-qq )

        ("quasiquotation unquote is identity"
	 ( (fn (x) `,x) "foo" )
	 "foo" )

        ("quasiquotation of list with unquote"
	 `(1 2 ,(+ 1 2))
	 (1 2 3) )

        ("quasiquotation of list with unquote"
	 `(a b c (+ 1 2 ,(+ 1 2)))
	 (a b c (+ 1 2 3)) )

        ("quasiquotation of list with unquote in first position"
	 `(,(+ 1 2) 2 1)
	 (3 2 1) )

        ("quasiquotation of list with unquote-splicing"
	 `(a b c (+ 1 2 ,@(cons 3 (cons 4 nil))))
	 (a b c (+ 1 2 3 4)) )

        ("quasiquotation of list with unquote-splicing in first position"
	 `(,@(cons 3 (cons 4 nil)) a b c ,(+ 0 5))
	 (3 4 a b c 5) )

	;; Expansion of nested qq's
        ("expand nested quasiquotes"
	 `(* 17 ,(plusall 1 2 3) `(+ 1 ,(plusall 1 2 3)))
	 (* 17 6 (list '+ '1 (plusall 1 2 3))))

        ("quasiquote uses local namespace"
	 ((fn (x) (overwrite (+ x x))) 7)
	 17)

        ("nested quasiquote with double unquote"
	 ((fn (qqq) `(a b qqq ,qqq `(a b qqq ,qqq ,,qqq))) 'qoo)
	 (a b qqq qoo (list 'a 'b 'qqq qqq qoo)))

	;; Changed from Dalton's tests again
        ("more nested quasiquote"
	 ((fn (x) ``,,x) 'y)
	 y)

	;; Additional tests from Fallintothis

	("Basic quasiquote test"
	 `(x ,x ,@x foo ,(cadr  x) bar ,(cdr x) baz ,@(cdr x) ,@ x)
	 (x (a b c) a b c foo b bar (b c) baz b c a b c))

	;; XXX - seems that nesting on-errs doesn't work with
	;; Anarki at least.  So we depend on the unit test framework's
	;; 
	("Splicing quasiquote error"
	 `,@x
	 "Error thrown: The syntax `,@x is invalid")

	("Locally bound symbols in quasiquotes"
	 (let lst '(a b c d)
	   `(foo `(bar ,@',(map (fn (sym) `(baz ',sym ,sym)) lst))))
	 (foo '(bar (baz 'a a) (baz 'b b) (baz 'c c) (baz 'd d))))

	("Quotes with nothing unquoted"
	 `(1 2 3 4)
	 (1 2 3 4))

	("Basic unquote-splicing"
	 `(,@x)
	 (a b c))

	("Basic unquotation"
	 `(,a ,b, c)
	 (10 20 30))

	("Double unquote"
	 (eval ``(,,@x))
	 (10 20 30))

	("Unquote/unquote splicing"
	 (eval ``(,,@(map (fn (z) `(list ',z)) x)))
	 ((a) (b) (c)))

	("Double unquote splicing"
	 (eval ``(,@,@(map (fn (z) `(list ',z)) x)))
	 (a b c))

	("Triply nested quasiquotes"
	 (eval (eval ```(,,@,@(map (fn (z) `(list ',z)) x))))
	 (10 20 30))
))

