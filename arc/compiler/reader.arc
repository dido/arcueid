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


;; Scan for the first non-blank character.  The non-blank character will
;; not be removed from fp.
(def scan (fp)
  (loop (r (peekc fp))
	(if (no r) nil
	    (whitec r) (do (readc fp) (recur (peekc fp)))
	    r)))

(def read (src (o eof nil))
  (let fp (if (isa src 'string) (instring src) src)
    (loop (ch (scan fp))
	  ((case ch
	     #\( readlist
	     #\) (err "misplaced right paren")
	     #\[ readanonf
	     #\] (err "misplaced left paren")
	     #\' readquote
	     #\` readqquote
	     #\, readcomma
	     #\" readstring
	     #\# readchar
	     #\; (do (readcomment fp) (recur (scan fp)))
	     readsym) fp eof)))

(def getsymbol (fp)
  (loop (state 0 ch (peekc fp) buf (outstring) rxflags)
	(let newstate
	    (case state
	      ;; Check if ch is r. If it does, transition to state 1,
	      ;; (possible read regex) else state 5
	      0 (if (is ch #\r) 2 5)
	      ;; Check if ch is '/'. If so, we are reading a regex,
	      ;; transition to state 2 and clear the buffer.  If not,
	      ;; go to state 5.
	      1
	      (if (is ch #\/) (do (readc fp)
				  (recur 2 (peekc fp) (outstring) rxflags))
		  5)
	      ;; Reading the body of the regex.
	      2
	      ;; If we see a /, we should have the possible flags of the regex
	      ;; to follow.  Do not write that / to the buffer.
	      (if (is ch #\/) (do (readc fp) (recur 4 (peekc fp) buf rxflags))
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
		  (do (readc fp) (recur 4 (peekc fp) buf (+ rxflags 2)))
		  (and (is ch #\m) (is (mod rxflags 2) 0))
		  (do (readc fp) (recur 4 (peekc fp) buf (+ rxflags 1)))
		  (or (is ch #\i) (is ch #\m)) (err "Regular expression flags used more than once")
		  (whitec ch) (recur 7 ch buf rxflags)
		  (err "invalid regular expression flag" ch))
	      5
	      ;; State 5, reading a normal symbol
	      (if (no (symc ch))
		  (recur 6 ch buf rxflags))
	      (err "FATAL: Invalid parsing state"))
	  (readc fp)
	  ;; Write character to buffer
	  (writec ch buf)
	  (if (is newstate 6)
	      ;; Terminal state, the buffer contains a symbol or
	      ;; a number, return it
	      (inside buf)
	      (is newstate 7)
	      ;; Terminal state, the buffer contains a regex
	      (mkregexp (inside buf) rxflags)
	      (recur newstate (peekc fp) buf rxflags)))))