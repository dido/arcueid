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
(register-test
 '(suite "Arc Reader"
	 ("Tokenise a symbol"
	  (w/instring fp "foo" (getsymbol fp))
	  "foo")

	 ("Tokenise two symbols separated by spaces"
	  (w/instring fp "foo 123 " (list (do1 (getsymbol fp) (scan fp))
					 (getsymbol fp)))
	  ("foo" "123"))

	 ("Read a simple regex"
	  (w/instring fp "r/foo/" (let rx (getsymbol fp)
				    (list (type rx)
					  (rep rx))))
	  (regexp ("foo" . 0)))

	 ("Read two symbols with numeric conversion"
	  (w/instring fp "foo 123 " (list (zread fp) (zread fp)))
	  (foo 123))

	 ("Read a simple list"
	  (w/instring fp " (1 2 3 ) " (zread fp))
	  (1 2 3))

	 ("Read a dotted list"
	  (w/instring fp "(1 2 . 3)" (zread fp))
	  (1 2 . 3))

	 ("Errors if dot is not followed by only one element"
	  (w/instring fp "(1 2 . 3 4)" (zread fp))
	  "Error thrown: illegal use of .")

	 ("Errors if dot is the first"
	  (w/instring fp "(. 3 4)" (zread fp))
	  "Error thrown: illegal use of .")

	 ("Read a bracket function"
	  (w/instring fp "[idfn _]" (zread fp))
	  (fn (_) (idfn _)))

	 ("Read something that appears after a long, multiline comment"
	  (w/instring fp "; a comment\n; more commentary\n; lorem ipsum dolor sit amet\nfoo" (zread fp))
	  foo)

	 ("Read a list with a comment in between"
	  (w/instring fp "(a ; comment here\n; more comment\nb; lorem ipsum\n c)" (zread fp))
	  (a b c))

	 ("Read a bracket function with a comment inside"
	  (w/instring fp "[a _; comment here\n; more comment\nb; lorem ipsum\n c]" (zread fp))
	  (fn (_) (a _ b c)))

	 ("Read empty"
	  (w/instring fp "" (zread fp))
	  nil)

	 ("Read comments only"
	  (w/instring fp "; lorem ipsum dolor\n; sit amet" (zread fp))
	  nil)

	 ("Read a quoted symbol"
	  (w/instring fp "'abc" (zread fp))
	  (quote abc))

	 ("Read a quoted list"
	  (w/instring fp "'(a b c)" (zread fp))
	  (quote (a b c)))

	 ("Read a quoted bracket function"
	  (w/instring fp "'[idfn _]" (zread fp))
	  (quote (fn (_) (idfn _))))

	 ("Read a quasiquoted symbol"
	  (w/instring fp "`abc" (zread fp))
	  (quasiquote abc))

	 ("Read a quasiquoted list"
	  (w/instring fp "`(a b c)" (zread fp))
	  (quasiquote (a b c)))

	 ("Read unquote sym"
	  (w/instring fp ",z" (zread fp))
	  (unquote z))

	 ("Read unquote list"
	  (w/instring fp ",(a b c)" (zread fp))
	  (unquote (a b c)))

	 ("Read unquote splicing sym"
	  (w/instring fp ",@@z" (zread fp))
	  (unquote-splicing z))

	 ("Read unquote-splicing list"
	  (w/instring fp ",@@(a b c)" (zread fp))
	  (unquote-splicing (a b c)))
))
