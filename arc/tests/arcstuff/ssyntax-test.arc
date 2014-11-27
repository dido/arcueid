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

(register-test
 '(suite "SSyntax Tests"
	 ("recognises compose"
	  (arc-ssyntax 'a:b)
	  t)

	 ("recognises complement and compose"
	  (arc-ssyntax '~a:b)
	  t)

	 ("recognises complement alone"
	  (arc-ssyntax '~a)
	  t)

	 ("recognises list"
	  (arc-ssyntax 'a.b)
	  t)

	 ("recognises list-quoted"
	  (arc-ssyntax 'a!b)
	  t)

	 ("andf"
	  (arc-ssyntax 'a&b)
	  t)

	 ("andf"
	  (arc-ssyntax '&a&b&)
	  t)))

(register-test
 '(suite "ssexpand"
	 ("expand compose"
	  (arc-ssexpand 'x:y)
	  (compose x y))

	 ("expand complement"
	  (arc-ssexpand '~p)
	  (complement p))

	 ("expand compose/complement"
	  (arc-ssexpand 'p:~q:r)
	  (compose p (complement q) r) )

	 ("expand compose/complement"
	  (arc-ssexpand '~p:q:r)
	  (compose (complement p) q r) )

	 ("expand compose with numbers"
	  (arc-ssexpand 'x:1.2)
	  (compose x 1.2))              ; bizarre but true

	 ("expand compose with numbers"
	  (type ((arc-ssexpand 'x:1.2) 2))
	  num)                          ; bizarre but true

	 ("expand list"
	  (arc-ssexpand '*.a.b)
	  ((* a) b))

	 ("expand quoted list"
	  (arc-ssexpand 'cons!a!b)
	  ((cons (quote a)) (quote b)) )

	 ("expand chained dots and bangs"
	  (arc-ssexpand 'a.b!c.d)
	  (((a b) (quote c)) d))

	 ("ssexpand with initial dot"
	  (arc-ssexpand '.a.b.c)
	  (((get a) b) c))

	 ("ssexpand with initial quote"
	  (arc-ssexpand '!a.b.c)
	  (((get (quote a)) b) c))

	 ("andf"
	  (arc-ssexpand 'a&b)
	  (andf a b))))

