;; Copyright (C) 2012 Rafael R. Sevilla
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
;; License along with this library; if not, see <http://www.gnu.org/licenses/>.
;;
(load "spec.arc")

(= test-builtins
   (describe "built-in functions"
	     (it "sig should be a table"
		 (is (type sig) 'table))
	     (it "sig should contain a binding for idfn"
		 (sig 'idfn))
	     (it "sig shouldn't contain a binding for a unique symbol"
		 (no (sig (uniq))))
	     (it "apply should be a fn"
		 (is (type apply) 'fn))
	     (it "apply should apply a function"
		 (is (apply - '(1 2 3 4 5 6)) -19))
	     (it "cons should be a fn"
		 (is (type cons) 'fn))
	     (it "cons should cons its arguments"
		 (iso (cons 1 2) '(1 . 2)))
	     (it "cons should be applicable"
		 (iso (apply cons '(1 2)) '(1 . 2)))
	     (it "car should be a fn"
		 (is (type car) 'fn))
	     (it "car should get the car of its arguments"
		 (is (car '(1 . 2)) '1))
	     (it "car should return nil for the empty list"
		 (is (car '()) nil))
	     (it "car should be applicable"
		 (is (apply car '((1 . 2))) 1))))

(print-results (test-builtins) t)
