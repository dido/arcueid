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

(register-test
 '(suite "quasiquote"
      (suite "quasiquote"
        ("quasi-quotation"
          '`qqfoo
          (quasiquote qqfoo))

        ("unquote"
          ',uqfoo
          (unquote uqfoo)
        )

        ("unquote-splicing"
          ',@uqsfoo
          (unquote-splicing uqsfoo) )

        ("quasi-quotation is just like quotation"
          `qqqfoo
          qqqfoo )

        ("quasi-quote quasi-quote"
	 ``double-qq
	 `double-qq )

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

        ("don't expand nested quasiquotes"
	 `(* 17 ,(plusall 1 2 3) `(+ 1 ,(plusall 1 2 3)))
	 (* 17 6 `(+ 1 ,(plusall 1 2 3))))

        ("quasiquote uses local namespace"
	 ((fn (x) (overwrite (+ x x))) 7)
	 17)

        ("nested quasiquote with double unquote"
	 ((fn (qqq) `(a b qqq ,qqq `(a b qqq ,qqq ,,qqq))) 'qoo)
	 (a b qqq qoo `(a b qqq ,qqq ,qoo)))

        ("more nested quasiquote"
	 ((fn (x) ``,,x) 'y)
	 `,y))))
	 
