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
;; An Arc reader that depends on only the presence of the readc function.

;; Parse one s-expression from src.  Returns the s-expression.
(def zread (src (o eof nil))
  (let fp (if (isa src 'string) (instring src) src)
    ((loop (ch (scan fp))
	   (case ch
	     nil readeof
	     #\( readlist
	     #\) (err "misplaced right paren")
	     #\[ readbrfn
	     #\] (err "misplaced left paren")
	     #\' readquote
	     #\` readqquote
	     #\, readcomma
	     #\" readstring
	     #\# readchar
	     #\; (do (readcomment fp) (recur (scan fp)))
	     readsym)) fp eof)))

;; Scan for the first non-blank character.  The non-blank character will
;; not be removed from fp.
(def scan (fp)
  (loop (r (peekc fp))
	(if (no r) nil
	    (whitec r) (do (readc fp) (recur (peekc fp)))
	    r)))

(def readeof (fp eof)
  eof)

;; Valid characters in symbols are any non-whitespace, and anything that
;; is not a parenthesis, square bracket, quote, comma, or semicolon.
;; 
(def symc (ch)
  (no (or (whitec ch) (in ch #\( #\) #\' #\, #\; #\[ #\]))))

;; Read a symbol from fp.  Should return a string or a regex.
(def getsymbol (fp)
  (loop (state 0 ch (peekc fp) buf (outstring) rxflags 0)
	(let newstate
	    (case state
	      ;; Check if ch is r. If it does, transition to state 1,
	      ;; (possible read regex) else state 5
	      0 (if (is ch #\r) 1 5)
	      ;; Check if ch is '/'. If so, we are reading a regex,
	      ;; transition to state 2 and clear the buffer.  If not,
	      ;; go to state 5.
	      1
	      (if (is ch #\/) (do (readc fp)
				  (list 2 (peekc fp) (outstring) rxflags))
		  5)
	      ;; Reading the body of the regex.
	      2
	      ;; If we see a /, we should have the possible flags of the regex
	      ;; to follow.  Do not write that / to the buffer.
	      (if (is ch #\/) (do (readc fp) (list 4 (peekc fp) buf rxflags))
		  ;; backslash, this is regex escape state.  Write the slash
		  ;; and stay in that state for the next character
		  (is ch #\\) 3
		  (no ch) (err "unexpected end of source while reading regex")
		  ;; stay in the state otherwise
		  state)
	      3
	      ;; escape state, go back to state 2.
	      2
	      4
	      ;; State 4, reading regex flags
	      (if (and (is ch #\i) (is (/ rxflags 2) 0))
		  (do (readc fp) (list 4 (peekc fp) buf (+ rxflags 2)))
		  (and (is ch #\m) (is (mod rxflags 2) 0))
		  (do (readc fp) (list 4 (peekc fp) buf (+ rxflags 1)))
		  (or (is ch #\i) (is ch #\m)) (err "Regular expression flags used more than once")
		  (or (no ch) (whitec ch)) 7
		  (err "invalid regular expression flag" ch))
	      5
	      ;; State 5, reading a normal symbol
	      (if (or (no ch) (no (symc ch))) 6
		  5)
	      (err "FATAL: Invalid parsing state"))
	  ;; Terminal states are 6 and 7.  Return a string or regex
	  ;; based on the buffer.
	  (if (is newstate 6) (inside buf) ; return contents of buffer
	      ;; create new regex based on buffer
	      (is newstate 7) (mkregexp (inside buf) rxflags)
	      ;; Otherwise, keep looping. If newstate is a list, bind it
	      ;; so as to get new parameters for recursion. Otherwise,
	      ;; recur with the default values.
	      (isa newstate 'cons)
	      (let (ns nc nbuf nrxf) newstate (recur ns nc nbuf nrxf))
	      (do (readc fp)
		  (writec ch buf)
		  (recur newstate (peekc fp) buf rxflags))))))

(def readsym (fp eof)
  (let mysym (getsymbol fp)
    (aif (isa mysym 'regexp) mysym
	 (string->num mysym) it
	 (sym mysym))))

(def readlist (fp eof)
  (readc fp)			;remove opening paren that got us here
  (loop (top nil last nil indot nil)
	(scan fp)
	(let ch (peekc fp)
	  (if (no ch) eof
	      (case ch
		#\; (do (readcomment fp) (recur top last indot))
		#\) top
		#\.
		(if indot (err "illegal use of .")
		    (do (readc fp) (recur top last 1)))
		(let val (zread fp eof)
		  (if (is indot 1)
		      (if (no last) (err "illegal use of .")
			  (do (scdr last val)
			      (recur top last 2)))
		      (is indot 2) (err "illegal use of .")
		      (no last)
		      (let mytop (cons val nil)
			(recur mytop mytop indot))
		      (let mylast (cons val nil)
			(scdr last mylast)
			(recur top mylast indot)))))))))

;; Read an Arc square bracketed anonymous function. This expands
;; [ ... _ ... ] to (fn (_) (... _ ...))
(def readbrfn (fp eof)
  (readc fp)			; remove opening bracket that got us here
  (loop (top nil last nil)
	(scan fp)
	(let ch (peekc fp)
	  (if (no ch) eof
	      (case ch
		#\; (do (readcomment fp) (recur top last))
		#\]
		;; complete the fn
		`(fn (_) ,top)
		(let val (zread fp eof)
		  (if (no last)
		      (let mytop (cons val nil)
			(recur mytop mytop))
		      (let mylast (cons val nil)
			(scdr last mylast)
			(recur top mylast)))))))))

;; Read comments. Keep reading until we reach the end of line or end of
;; file.
(def readcomment (fp)
  (loop (r (readc fp))
	(if (or (no r) (in r #\return #\newline)) r
	    (recur (readc fp)))))
