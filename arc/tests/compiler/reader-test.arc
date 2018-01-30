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

	 ("Read something that appears after a long, multiline comment (style 2)"
	  (w/instring fp "#! a comment\n#! more commentary\n#! lorem ipsum dolor sit amet\nfoo" (zread fp))
	  foo)

	 ("Read something that appears after a block comment"
	  (w/instring fp "#| a comment\nmore commentary\nlorem ipsum dolor sit amet|#foo" (zread fp))
	  foo)

	 ("Nested block comment"
	  (w/instring fp "#| a comment\n#| more commentary |#\nlorem ipsum dolor sit amet|#foo" (zread fp))
	  foo)

	 ("Read something that appears after a sexpr comment 1"
	  (w/instring fp "#;a;z\nb" (zread fp))
	  b)

	 ("Read something that appears after a sexpr comment 2"
	  (w/instring fp "#;(z y x) b" (zread fp))
	  b)

	 ("Read something that appears after a sexpr comment 3"
	  (w/instring fp "#;[_ z y x] b" (zread fp))
	  b)

	 ("Read something that appears after a sexpr comment 4"
	  (w/instring fp "#;(z y x #| n |#) b" (zread fp))
	  b)

	 ("Read something that appears after a sexpr comment 4"
	  (w/instring fp "#;[_ z y x n #| n |#] b" (zread fp))
	  b)

	 ("List with comments interspersed"
	  (w/instring fp "(a ; comment here\n; more comment\nb; lorem ipsum\n c ; another comment\n)" (zread fp))
	  (a b c))

	 ("List with comments interspersed (style 2)"
	  (w/instring fp "(a #! comment here\n#! more comment\nb #! lorem ipsum\n c ; another comment\n)" (zread fp))
	  (a b c))

	 ("List with block comments"
	  (w/instring fp "(a #| comment here\nmore comment |#b #| lorem ipsum |#c #| another comment |#)" (zread fp))
	  (a b c))

	 ("List with sexpr comment"
	  (w/instring fp "(a #;(b c d) e f)" (zread fp))
	  (a e f))

	 ("Bracket function with comments interspersed"
	  (w/instring fp "[a _; comment here\n; more comment\nb; lorem ipsum\n c; more comments\n]" (zread fp))
	  (fn (_) (a _ b c)))

	 ("Bracket function with comments interspersed (style 2)"
	  (w/instring fp "[a _ #! comment here\n#! more comment\nb #! lorem ipsum\n c #! more comments\n]" (zread fp))
	  (fn (_) (a _ b c)))

	 ("Bracket function with block comments"
	  (w/instring fp "[a _ #| comment here\nmore comment |#b #| lorem ipsum |#c #| another comment |#]" (zread fp))
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
	  (w/instring fp ",@z" (zread fp))
	  (unquote-splicing z))

	 ("Read unquote-splicing list"
	  (w/instring fp ",@(a b c)" (zread fp))
	  (unquote-splicing (a b c)))

	 ("Read characters"
	  (w/instring fp "(#\\a #\\b #\\c #\\nul #\\backspace #\\tab #\\newline #\\linefeed #\\vtab #\\page #\\return #\\space #\\rubout #\\377 #\\u9f8d #\\龍)" (zread fp))
	  (#\a #\b #\c #\nul #\backspace #\tab #\newline #\linefeed #\vtab #\page #\return #\space #\rubout #\377 #\u9f8d #\龍))

	 ("Read a string"
	  (w/instring fp "\"cwm fjord bank glyphs vext quiz 龍\\a\\b\\t\\n\\v\\f\\r\\e\\\"\\'\\\\\"" (zread fp))
	  "cwm fjord bank glyphs vext quiz 龍\a\b\t\n\v\f\r\e\"\'\\")

	 ("Octal escape sequences"
	  (w/instring fp "\"\\377\1234\\728\"" (zread fp))
	  "\377S4:8")

	 ("Unicode escape sequences"
	  (w/instring fp "\"\\u9f8dd\u1d107\U1d107\U00009f8de\"" (zread fp))
	  "龍dᴐ7𝄇龍e")

	 ("Basic regex"
	  (w/instring fp "#/foo\\[/" (let rx (zread fp)
				    (list (type rx)
					  (rep rx))))
	  (regexp ("foo\\[" . 0)))

	 ("Regex with multiline flag"
	  (w/instring fp "#/foo\\[/m" (let rx (zread fp)
				    (list (type rx)
					  (rep rx))))
	  (regexp ("foo\\[" . 1)))

	 ("Regex with case insensitive flag"
	  (w/instring fp "#/foo\\[/i" (let rx (zread fp)
				    (list (type rx)
					  (rep rx))))
	  (regexp ("foo\\[" . 2)))

	 ("Regex with both multiline and insensitive flags"
	  (w/instring fp "#/foo\\[/mi" (let rx (zread fp)
				    (list (type rx)
					  (rep rx))))
	  (regexp ("foo\\[" . 3)))

	 ("Regex with both insensitive and multiline flags"
	  (w/instring fp "#/foo\\[/im" (let rx (zread fp)
				    (list (type rx)
					  (rep rx))))
	  (regexp ("foo\\[" . 3)))


))
