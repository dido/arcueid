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
;; This should have some extensive tests for all of the Arc standard builtin
;; functions which are given as xdef's inside of ac.scm in reference
;; Arc or bound as builtin functions within arcueid.c
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
		      (no (is 893376257100809409799984939122178013930683883078043184453782452513192426147282686921795544100434905935955264558365012336103114821128262549221278699869987 nil 893376257100809409799984939122178013930683883078043184453782452513192426147282686921795544100434905935955264558365012336103114821128262549221278699869987))))
	     (it "is should return true if all rational arguments are equivalent"
		 (is 1/2 1/2 1/2))
	     (it "is should return false if not all rational arguments are equivalent"
		 (and (no (is 1/2 1/3 1/2))
		      (no (is 1/2 t 1/2))))
	     (it "is should return true if all complex arguments are equivalent"
		 (is 1+1i 1+1i 1+1i))
	     (it "is should return false if not all complex arguments are equivalent"
		 (and (no (is 1+1i 1+1i 1+2i 1+1i))
		      (no (is 1+1i 1+1i t 1+1i))))
	     ;; XXX - other type combinations
	     (it "is should be a fn"
		 (is (type is) 'fn))

	     ;; XXX - fill in tests for all these other functions
	     (it "err should be a fn"
		 (is (type err) 'fn))

	     (it "nil should be a sym"
		 (is (type nil) 'sym))

	     (it "t should be a sym"
		 (is (type t) 'sym))

	     (it "+ should be a fn"
		 (is (type +) 'fn))

	     (it "- should be a fn"
		 (is (type -) 'fn))

	     (it "* should be a fn"
		 (is (type *) 'fn))

	     (it "/ should be a fn"
		 (is (type /) 'fn))

	     (it "mod should be a fn"
		 (is (type mod) 'fn))

	     (it "expt should be a fn"
		 (is (type expt) 'fn))

	     (it "sqrt should be a fn"
		 (is (type sqrt) 'fn))

	     (it "> should be a fn"
		 (is (type >) 'fn))

	     (it "< should be a fn"
		 (is (type <) 'fn))

	     (it "len should be a fn"
		 (is (type len) 'fn))

	     (it "annotate should be a fn"
		 (is (type annotate) 'fn))

	     (it "type should be a fn"
		 (is (type type) 'fn))

	     (it "rep should be a fn"
		 (is (type rep) 'fn))

	     (it "uniq should be a fn"
		 (is (type uniq) 'fn))

	     (it "ccc should be a fn"
		 (is (type ccc) 'fn))

	     (it "infile should be a fn"
		 (is (type infile) 'fn))

	     (it "outfile should be a fn"
		 (is (type outfile) 'fn))

	     (it "instring should be a fn"
		 (is (type instring) 'fn))

	     (it "outstring should be a fn"
		 (is (type outstring) 'fn))

	     (it "inside should be a fn"
		 (is (type inside) 'fn))

	     (it "stdout should be a fn"
		 (is (type stdout) 'fn))

	     (it "stdin should be a fn"
		 (is (type stdin) 'fn))

	     (it "stderr should be a fn"
		 (is (type stderr) 'fn))

	     (it "call-w/stdout should be a fn"
		 (is (type call-w/stdout) 'fn))

	     (it "call-w/stdin should be a fn"
		 (is (type call-w/stdin) 'fn))

	     (it "readc should be a fn"
		 (is (type readc) 'fn))

	     (it "readb should be a fn"
		 (is (type readb) 'fn))

	     (it "peekc should be a fn"
		 (is (type peekc) 'fn))

	     (it "writec should be a fn"
		 (is (type writec) 'fn))

	     (it "writeb should be a fn"
		 (is (type writeb) 'fn))

	     (it "write should be a fn"
		 (is (type write) 'fn))

	     (it "sread should be a fn"
		 (is (type sread) 'fn))

	     (it "coerce should be a fn"
		 (is (type coerce) 'fn))

	     (it "open-socket should be a fn"
		 (is (type open-socket) 'fn))

	     (it "socket-accept should be a fn"
		 (is (type socket-accept) 'fn))

	     (it "setuid should be a fn"
		 (is (type setuid) 'fn))

	     (it "new-thread should be a fn"
		 (is (type new-thread) 'fn))

	     (it "kill-thread should be a fn"
		 (is (type kill-thread) 'fn))

	     (it "break-thread should be a fn"
		 (is (type break-thread) 'fn))

	     (it "current-thread should be a fn"
		 (is (type current-thread) 'fn))

	     (it "sleep should be a fn"
		 (is (type sleep) 'fn))

	     (it "sleep should be a fn"
		 (is (type sleep) 'fn))

	     (it "system should be a fn"
		 (is (type system) 'fn))

	     (it "pipe-from should be a fn"
		 (is (type pipe-from) 'fn))

	     (it "table should be a fn"
		 (is (type table) 'fn))

	     (it "maptable should be a fn"
		 (is (type maptable) 'fn))

	     (it "protect should be a fn"
		 (is (type protect) 'fn))

	     (it "rand should be a fn"
		 (is (type rand) 'fn))

	     (it "dir should be a fn"
		 (is (type dir) 'fn))

	     (it "file-exists should be a fn"
		 (is (type file-exists) 'fn))

	     (it "dir-exists should be a fn"
		 (is (type dir-exists) 'fn))

	     (it "rmfile should be a fn"
		 (is (type rmfile) 'fn))

	     (it "mvfile should be a fn"
		 (is (type mvfile) 'fn))

	     (it "macex should be a fn"
		 (is (type macex) 'fn))

	     (it "macex1 should be a fn"
		 (is (type macex1) 'fn))

	     (it "eval should be a fn"
		 (is (type eval) 'fn))

	     (it "on-err should be a fn"
		 (is (type on-err) 'fn))

	     (it "details should be a fn"
		 (is (type details) 'fn))

	     (it "scar should be a fn"
		 (is (type scar) 'fn))

	     (it "scdr should be a fn"
		 (is (type scdr) 'fn))

	     (it "sref should be a fn"
		 (is (type sref) 'fn))

	     (it "bound should be a fn"
		 (is (type bound) 'fn))

	     (it "newstring should be a fn"
		 (is (type newstring) 'fn))

	     (it "trunc should be a fn"
		 (is (type trunc) 'fn))

	     (it "exact should be a fn"
		 (is (type exact) 'fn))

	     (it "msec should be a fn"
		 (is (type msec) 'fn))

	     (it "current-process-milliseconds should be a fn"
		 (is (type current-process-milliseconds) 'fn))

	     (it "current-gc-milliseconds should be a fn"
		 (is (type current-gc-milliseconds) 'fn))

	     (it "seconds should be a fn"
		 (is (type seconds) 'fn))

	     (it "client-ip should be a fn"
		 (is (type client-ip) 'fn))

	     (it "atomic-invoke should be a fn"
		 (is (type atomic-invoke) 'fn))

	     (it "dead should be a fn"
		 (is (type dead) 'fn))

	     (it "flushout should be a fn"
		 (is (type flushout) 'fn))

	     (it "ssyntax should be a fn"
		 (is (type ssyntax) 'fn))

	     (it "ssexpand should be a fn"
		 (is (type ssexpand) 'fn))

	     (it "quit should be a fn"
		 (is (type quit) 'fn))

	     (it "close should be a fn"
		 (is (type close) 'fn))

	     (it "force-close should be a fn"
		 (is (type force-close) 'fn))

	     (it "memory should be a fn"
		 (is (type memory) 'fn))


	     (it "declare should be a fn"
		 (is (type declare) 'fn))

	     (it "timedate should be a fn"
		 (is (type timedate) 'fn))

	     (it "sin should be a fn"
		 (is (type sin) 'fn))

	     (it "cos should be a fn"
		 (is (type cos) 'fn))

	     (it "tan should be a fn"
		 (is (type tan) 'fn))

	     (it "asin should be a fn"
		 (is (type asin) 'fn))

	     (it "acos should be a fn"
		 (is (type acos) 'fn))

	     (it "atan should be a fn"
		 (is (type atan) 'fn))

	     (it "log should be a fn"
		 (is (type log) 'fn))))



(print-results (test-builtins) t)
