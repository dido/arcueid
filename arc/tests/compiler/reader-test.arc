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
	  (foo 123))))