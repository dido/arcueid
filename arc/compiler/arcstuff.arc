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
;; Some functions normally defined in ac.scm rewritten in pure Arc

(def ssyntax (x)
  (let sscharp
      (afn (str i)
	   (and (>= i 0)
		(or (let c (str i)
		      (or (is c #\:) (is c #\~) (is c #\&)
			  ; (is c #\_)
			  (is c #\.) (is c #\!)))
		    (self str (- i 1)))))
    (and (isa x 'sym)
	 (no (or (is x '+) (is x '++) (is x '_)))
	 (let name (coerce x 'string)
	   (sscharp name (- (len name) 1))))))
