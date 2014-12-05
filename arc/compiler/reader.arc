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
;; An Arc reader that depends on only the presence of the readc and peekc
;; functions.

;; Parse one s-expression from src.  Returns the s-expression.
(def zread (src (o eof nil) (o rparen [err "misplaced " _]))
  (let fp (if (isa src 'string) (instring src) src)
    ((loop (ch (scan fp))
	   (case ch
	     nil readeof
	     #\( readlist
	     #\) (rparen ch)
	     #\[ readbrfn
	     #\] (rparen ch)
	     #\' (do (readc fp) (fn (f e) (readquote f e 'quote)))
	     #\` (do (readc fp) (fn (f e) (readquote f e 'quasiquote)))
	     #\, readcomma
	     #\" readstring
	     #\# (do (readc fp)
		     (case (peekc fp)
		       nil (err "end of file reached while reading #")
		       #\\ readchar
		       #\| (do (readblockcomment fp) (recur (scan fp)))
		       #\! (do (readcomment fp) (recur (scan fp)))
		       #\; (do (readsexprcomment fp) (recur (scan fp)))
		       ;; #\( readvector
		       #'< (do (readc fp)
			       (if (is (peekc fp) #\<) readheredoc
				   (fn (fp eof) (readsym fp eof "#<"))))
		       #\/ readregex
		       (fn (fp eof) (readsym fp eof "#"))))
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

;; Read a symbol from fp.  Should return a string.
(def getsymbol (fp)
  (loop (ch (peekc fp) buf (outstring))
	(if (or (no ch) (no (symc ch)))
	    (inside buf)
	    (do (writec (readc fp) buf)
		(recur (peekc fp) buf)))))

;; Read a symbol from fp, adding the prefix (if specified).  Should
;; return the symbol or a number if the symbol can be parsed as a number.
(def readsym (fp eof (o prefix ""))
  (let mysym (+ prefix (getsymbol fp))
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
		#\) top
		#\. (if indot (err "illegal use of .")
			(do (readc fp) (recur top last 1)))
		(ccc
		 (fn (k)
		     (let val (zread fp eof
				     [if (is _ #\)) (k top)
					 (err "misplaced " _)])
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
			     (recur top mylast indot)))))))))))

;; Read an Arc square bracketed anonymous function. This expands
;; [ ... _ ... ] to (fn (_) (... _ ...))
(def readbrfn (fp eof)
  (readc fp)			; remove opening bracket that got us here
  (loop (top nil last nil)
	(scan fp)
	(let ch (peekc fp)
	  (if (no ch) eof
	      (is ch #\]) `(fn (_) ,top)
	      (ccc (fn (k)
		       (let val (zread fp eof
				       [if (is _ #\]) (k `(fn (_), top))
					   (err "misplaced " _)])
			 (if (no last)
			     (let mytop (cons val nil)
			       (recur mytop mytop))
			     (let mylast (cons val nil)
			       (scdr last mylast)
			       (recur top mylast))))))))))

;; Read comments. Keep reading until we reach the end of line or end of
;; file.
(def readcomment (fp)
  (loop (r (readc fp))
	(if (or (no r) (in r #\return #\newline)) r
	    (recur (readc fp)))))

;; Read block comments.  Keep reading and just ignore everything until
;; we see a #|

;; Read some quote qsym.
(def readquote (fp eof qsym)
  (let val (zread fp eof) (cons qsym (cons val nil))))

(def readcomma (fp eof)
  (readc fp)			;remove the comma
  (let qsym (if (is (peekc fp) #\@) (do (readc fp) 'unquote-splicing)
		'unquote)
    (readquote fp eof qsym)))
