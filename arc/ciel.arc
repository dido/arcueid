;; Copyright (C) 2013 Rafael R. Sevilla
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

(assign ciel-dumpers* (table))

(def ciel-dump (expr)
  (if (atom expr)
      (ciel-dump-atom expr)
      (isa expr 'cons)
      (ciel-dump-cons expr)
      (isa expr 'table)
      (ciel-dump-table expr)
      ((ciel-dumpers* (type expr)) expr)))

(def ciel-dump-atom (expr)
  (if (no expr)
      (ciel-emit 'inil)
      (is expr t)
      (ciel-emit 'itrue)
      (isa expr 'sym)
      (ciel-emit-sym expr)
      (isa expr 'string)
      (ciel-emit-string expr)
      (isa expr 'int)
      (do (ciel-emit 'iint) (ciel-emit-int expr))
      (isa expr 'num)
      (ciel-emit-num expr)
      ((ciel-dumpers* (type expr)) expr)))

(def ciel-emit (opcode)
  (let oc (case opcode
	    inil 0
	    itrue 1
	    iint 2
	    isym 3
	    istr 4
	    inum 5
	    icons 128
	    itbl 129)
    (writec (coerce oc 'char))))

;; Note: NO CIEL opcode generated!
(def ciel-emit-int (i)
  (let conv (fn (x) (coerce x 'char))
    ((afn (x)
       (with (quotient (trunc (/ x 128)) remainder (mod x 128))
	 (if (is quotient 0)
	     (write (conv (+ 128 remainder)))
	     (do (write (conv remainder))
		 (self quotient))))) i)))

(def ciel-wstring (str)
  (each ch str
    (writec ch)))

(def ciel-emit-string (str)
  (do (ciel-emit 'istr)
      (ciel-emit-int (len str))
      (ciel-wstring str)))

;; XXX - this needs to distinguish the various numeric types and
;; generate an IEEE-754 compatible binary version of a non-integer
;; type!
(def ciel-emit-num (num)
  (let nstr (+ "" num)
    (do (ciel-emit 'inum)
	(ciel-emit-int (len nstr))
	(ciel-wstring nstr))))

;; XXX - a cycle detection algorithm might be needed someday
(def ciel-dump-cons (expr)
  (do (ciel-emit 'icons)
      (ciel-dump (car expr))
      (ciel-dump (cdr expr))))


(def ciel-dump-table (tbl)
  (do (ciel-emit 'itbl)
      (ciel-emit-int (len tbl))
      (maptable (fn (k v) (+ (ciel-dump k) (ciel-dump v))) tbl)))
