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
;; This version of the Arc reader depends only on the following foundation
;; I/O functions being present:
;;
;; 1. instring
;; 2. outstring
;; 3. readc
;; 4. peekc
;; 5. writec
;;
;; Still unimplemented that are present in reference Arc (most of them
;; artefacts of the Racket reader):
;; 1. #i/#e/#x/#o/#d/#b prefixes for numbers
;; 2. #<< syntax for here documents
;; 3. Vectors (obviously not supported by reference Arc)
;;

;; Parse one s-expression from src.  Returns the s-expression.
(def zread (src (o eof nil) (o rparen [err "misplaced " _]))
  (let fp (if (isa src 'string) (instring src) src)
    ((aloop (ch (scan fp))
	    (case ch
	      nil readeof
	      #\( readlist
	      #\) (rparen (readc fp))
	      #\[ readbrfn
	      #\] (rparen (readc fp))
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
			#\; (do (readsexprcomment fp eof) (recur (scan fp)))
			;; #\( readvector
			;; #\< (do (readc fp)
			;; 	       (if (is (peekc fp) #\<)
			;; 		   (do (readc fp)
			;; 		       readheredoc)
			;; 		   (fn (fp eof) (readsym fp eof "#<"))))
			#\/ readregex
			(fn (fp eof) (readsym fp eof "#"))))
	      #\; (do (readcomment fp) (recur (scan fp)))
	      readsym)) fp eof)))

;; Scan for the first non-blank character.  The non-blank character will
;; not be removed from fp.
(def scan (fp)
  (aloop (r (peekc fp))
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
  (aloop (ch (peekc fp) buf (outstring))
	 (if (or (no ch) (no (symc ch)))
	     (inside buf)
	     (do (writec (readc fp) buf)
		 (recur (peekc fp) buf)))))

;; Read a symbol from fp, adding the prefix (if specified).  Should
;; return the symbol or a number if the symbol can be parsed as a number.
(def readsym (fp eof (o prefix ""))
  (let mysym (+ prefix (getsymbol fp))
    (aif (string->num mysym) it
	 (sym mysym))))

(def readlist (fp eof)
  (readc fp)			;remove opening paren that got us here
  (aloop (top nil last nil indot nil)
	 (scan fp)
	 (let ch (peekc fp)
	   (if (no ch) (err "unterminated list meets end of file")
	       (is ch #\.)
	       (if indot (err "illegal use of .")
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
			   (recur top mylast indot))))))))))

;; Read an Arc square bracketed anonymous function. This expands
;; [ ... _ ... ] to (fn (_) (... _ ...))
(def readbrfn (fp eof)
  (readc fp)			; remove opening bracket that got us here
  (aloop (top nil last nil)
	(scan fp)
	(let ch (peekc fp)
	  (if (no ch) (err "unterminated bracket fn meets end of file")
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
  (aloop (r (readc fp))
	 (if (or (no r) (newlinec r)) r
	     (recur (readc fp)))))

;; Read block comments.  Keep reading and just ignore everything until
;; we see a |#.
(def readblockcomment (fp)
  (readc fp)
  (aloop (r (readc fp) nestlevel 0)
	 (if (and (is r #\|) (is (peekc fp) #\#))
	     (do (readc fp)
		 (if (is nestlevel 0)
		     (peekc fp)
		     (recur (readc fp) (- nestlevel 1))))
	     (and (is r #\#) (is (peekc fp) #\|))
	     (do (readc fp) (recur (readc fp) (+ nestlevel 1)))
	     (recur (readc fp) nestlevel))))

;; Read a sexpr comment.  Does a single read and ignores it.
(def readsexprcomment (fp eof)
  (readc fp)			; remove the extra #\;
  (zread fp eof))

;; Read some quote qsym.
(def readquote (fp eof qsym)
  (let val (zread fp eof) (cons qsym (cons val nil))))

(def readcomma (fp eof)
  (readc fp)			;remove the comma
  (let qsym (if (is (peekc fp) #\@) (do (readc fp) 'unquote-splicing)
		'unquote)
    (readquote fp eof qsym)))

;; Read a character.  XXX -- no limits yet on digits for Unicode escapes
(def readchar (fp eof)
  (readc fp)
  (let sym (getsymbol fp)
    (case sym
      "null" #\null
      "nul" #\nul
      "backspace" #\backspace
      "tab" #\tab
      "newline" #\newline
      "linefeed" #\linefeed
      "vtab" #\vtab
      "page" #\page
      "return" #\return
      "space" #\space
      "rubout" #\rubout
      (aif (or (string->num sym 8)
	       (and (in (sym 0) #\u #\U)
		    (string->num (substring sym 1) 16)))
	   (coerce it 'char)		; Unicode escape
	   (is (len sym) 1)
	   (sym 0)			; plain char
	   (err (+ "bad character constant: #\\" sym))))))

;; Read a string.
(def readstring (fp eof (o endch #\") (o metachar (fn (ch buf) nil)))
  (readc fp)
  (aloop (state 0 ch (peekc fp) buf (outstring) digits nil)
	 (case state
	   ;; normal characters
	   0 (if (is ch endch)
		 (recur 1 (readc fp) buf digits)
		 (is ch #\\) (do (readc fp) (recur 2 (peekc fp) buf digits))
		 (do (writec (readc fp) buf)
		     (recur state (peekc fp) buf digits)))
	   ;; end of string
	   1 (inside buf)
	   ;; escape sequence
	   2 (let addesc [do (readc fp) (writec _ buf)
			     (recur 0 (peekc fp) buf digits)]
	       (if (is ch endch) (addesc endch)
		   (case ch
		     #\a (addesc #\u0007)
		     #\b (addesc #\backspace)
		     #\t (addesc #\tab)
		     #\n (addesc #\newline)
		     #\v (addesc #\vtab)
		     #\f (addesc #\page)
		     #\r (addesc #\return)
		     #\e (addesc #\u001b)
		     #\' (addesc #\')
		     #\\ (addesc #\\)
		     #\u (do (readc fp) (recur 4 (peekc fp) buf (list 0 0 4)))
		     #\U (do (readc fp) (recur 4 (peekc fp) buf (list 0 0 8)))
		     (if (metachar ch buf)
			 (do (readc fp) (recur 0 (peekc fp) buf digits))
			 (and (digit ch) (< ch #\8)) (recur 3 ch buf (list 0 0))
			 (err (+ "unknown escape sequence \\" ch " in string"))))))
	   ;; octal escape sequence
	   3
	   (let (value ndigits) digits
	     (if (and (digit ch) (< ch #\8) (< ndigits 3))
		 (do (readc fp)
		     (recur state (peekc fp) buf
			    (list (+ (* value 8) (- (int ch) 48))
				  (+ ndigits 1))))
		 ;; terminate
		 (do (writec (coerce value 'char) buf)
		     (recur 0 ch buf nil))))
	   4
	   (if (hexdigit ch) (recur 5 ch buf digits)
	       (err "no hex digit following Unicode escape in string"))
	   5
	   (let (value ndigits max) digits
	     (if (and (< ndigits max) (hexdigit ch))
		 (let val (if (digit ch)
			      (- (int ch) 48)
			      (- (int:downcase ch) 87))
		   (readc fp)
		   (recur state (peekc fp) buf
			  (list (+ (* value 16) val) (+ ndigits 1) max)))
		 (do (writec (coerce value 'char) buf)
		     (recur 0 ch buf nil))))
	   (err "FATAL: invalid readstring state"))))

;; Unique to Arcueid.  Not supported by regular Arc.
(def readregex (fp eof)
  ;; This metachar function permits recognition of the regular expression
  ;; escape sequences.  Otherwise, the only way to enter them would be to
  ;; prefix them with double rather than single slashes.
  (withs (metachar
	  (fn (ch buf)
	      (if (in ch #\[ #\^ #\$ #\. #\| #\? #\* #\+ #\( #\) #\{ #\})
		  (do (writec #\\ buf) (writec ch buf) t)
		  nil))
	  rx (readstring fp eof #\/ metachar))
	 ;; Now we have to determine what flags are defined.  Arcueid
	 ;; recognises the i and m flags which denote case-insensitive
	 ;; and multiline regexes respectively.  They may only be specified
	 ;; exactly once.
	 (aloop (iflag nil mflag nil)
		(case (peekc fp)
		  #\i (if iflag (err "case-insensitive flag specified more than once")
			  (do (readc fp) (recur t mflag)))
		  #\m (if mflag (err "multiline flag specified more than once")
			  (do (readc fp) (recur iflag t)))
		  (if (and iflag mflag) (mkregexp rx 3)
		      iflag (mkregexp rx 2)
		      mflag (mkregexp rx 1)
		      (mkregexp rx 0))))))

;; (def readheredoc (fp eof)
;;   ;; first, read the terminator string. Include the newline at the end.
;;   (let terminator 
;;       (loop (ch (peekc fp) buf (outstring))
;; 	    (if (newlinec ch) (do (readc fp) (inside buf))
;; 		(do (writec (readc fp) buf)
;; 		    (recur (peekc fp) buf))))
;;     ;; Now we have a state machine that will proceed until we see
;;     ;; the terminator string on a line by itself
;;     (loop (ch (peekc fp) str "" obuf (outstring))
;; 	  (if (no ch) (err "unterminated heredoc meets end of file")
;; 	      (newlinec ch) (let outstr (inside obuf)
;; 			      (if (is outstr terminator) str
;; 				  (do (readc fp)
;; 				      (recur (peekc fp)
;; 					     (+ str (inside obuf))
;; 					     (outstring)))))
;; 	      (do (writec (readc fp) obuf)
;; 		  (recur (peekc fp) str obuf))))))
