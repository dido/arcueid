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
	     (it "apply should apply its first argument to the list"
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
		 (is (car '(1 . 2)) 1))
	     (it "car should return nil for the empty list"
		 (is (car '()) nil))
	     (it "car should be applicable"
		 (is (apply car '((1 . 2))) 1))

	     (it "cdr should be a fn"
		 (is (type cdr) 'fn))
	     (it "cdr should return the cdr of its arguments"
		 (is (cdr '(1 . 2)) 2))
	     (it "cdr should return nil for the empty list"
		 (is (cdr '()) nil))
	     (it "cdr should be applicable"
		 (is (apply cdr '((1 . 2))) 2))

	     (it "is should be a fn"
		 (is (type is) 'fn))
	     (it "is should return true if all arguments are t"
		 (is t t t t))
	     (it "is should return false if not all arguments are t"
		 (and (no (is t nil t t))
		      (no (is t 1 t t))))
	     (it "is should return true if all arguments are nil"
		 (is nil nil nil))
	     (it "is should return false if not all arguments are nil"
		 (and (no (is nil nil t nil))
		      (no (is nil 1 nil nil))))
	     (it "is should return true if all fixnum arguments are equivalent"
		 (is 12 12 12 12))
	     (it "is should return false if not all fixnum arguments are equivalent"
		 (and (no (is 12 13 12 12))
		      (no (is 12 nil nil 12))))
	     (it "is should return true if all flonum arguments are equivalent"
		 (is 1.1 1.1 1.1 1.1))
	     (it "is should return true if not all flonum arguments are equivalent"
		 (no (is 1.1 1.2 1.1)))
	     (it "is should return true if all bignum arguments are equivalent"
		 (is 893376257100809409799984939122178013930683883078043184453782452513192426147282686921795544100434905935955264558365012336103114821128262549221278699869987 893376257100809409799984939122178013930683883078043184453782452513192426147282686921795544100434905935955264558365012336103114821128262549221278699869987 893376257100809409799984939122178013930683883078043184453782452513192426147282686921795544100434905935955264558365012336103114821128262549221278699869987))
	     (it "is should return false if not all bignum arguments are equivalent"
		 (and (no (is 893376257100809409799984939122178013930683883078043184453782452513192426147282686921795544100434905935955264558365012336103114821128262549221278699869987 893376257100809409799984939122178013930683883078043184453782452513192426147282686921795544100434905935955264558365012336103114821128262549221278699869986 893376257100809409799984939122178013930683883078043184453782452513192426147282686921795544100434905935955264558365012336103114821128262549221278699869987))
		      (no (is 893376257100809409799984939122178013930683883078043184453782452513192426147282686921795544100434905935955264558365012336103114821128262549221278699869987 nil 893376257100809409799984939122178013930683883078043184453782452513192426147282686921795544100434905935955264558365012336103114821128262549221278699869987))))))

(print-results (test-builtins) t)
