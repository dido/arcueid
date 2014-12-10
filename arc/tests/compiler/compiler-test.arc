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
 '(suite "Arcueid Compiler"
	 ("nil"
	  (acc nil (acc-context) nil nil)
	  ((inil) . nil))
	 ("nil cont"
	  (acc nil (acc-context) nil t)
	  ((inil iret) . nil))

	 ("true"
	  (acc t (acc-context) nil nil)
	  ((itrue) . nil))

	 ("fixnum"
	  (acc 1234 (acc-context) nil nil)
	  ((ildi 1234) . nil))

	 ("global symbol"
	  (acc 'foo (acc-context) nil nil)
	  ((ildg 0) . (foo)))

	 ("other literal"
	  (acc 3.0 (acc-context) nil nil)
	  ((ildl 0) . (3.0)))

	 ("'if' special form - empty"
	  (acc '(if) (acc-context) nil nil)
	  ((inil) . nil))

	 ("'if' special form - single clause"
	  (acc '(if 4) (acc-context) nil nil)
	  ((ildi 4) . nil))

	 ("'if' special form - full"
	  (acc '(if t 1 2) (acc-context) nil nil)
	  ((itrue ijf 6 ildi 1 ijmp 4 ildi 2) . nil))

	 ("'if' special form - partial"
	  (acc '(if t 1) (acc-context) nil nil)
	  ((itrue ijf 6 ildi 1 ijmp 3 inil) . nil))

	 ("'if' special form - compound"
	  (acc '(if t 1 t 3 t 5 6) (acc-context) nil nil)
	  ((itrue ijf 6 ildi 1 ijmp 18 itrue ijf 6 ildi 3 ijmp 11
		  itrue ijf 6 ildi 5 ijmp 4 ildi 6) . nil))

	 ("quote"
	  (acc ''foo (acc-context) nil nil)
	  ((ildl 0) . (foo)))

	 ("lambda abstraction, basic"
	  (acc  '(fn (x) x) (acc-context) nil nil)
	  ((ildl 0 icls) ((ienv 1 0 0 ilde 0 0 iret))))
))