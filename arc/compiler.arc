
;; Compile an expression with all special syntax expanded.
(def compile (expr ctx cont)
  (if (literalp expr) (compile-literal expr ctx cont)
      (is (type expr) 'sym) (compile-ident expr ctx cont)
      (is (type expr) 'cons) (compile-list expr ctx cont)
      (syntax-error "invalid expression" expr)))

(def compile-literal (expr ctx cont)
  (do (if (no expr) (generate ctx 'inil)
	  (is expr t) (generate ctx 'itrue)
	  (fixnump expr) (generate ctx 'ildi (+ (* expr 2) 1))
	  (generate ctx 'ildl (find-literal expr ctx)))
      (compile-continuation ctx cont)))

(def compile-ident (expr ctx cont)
  (do (let (found level offset) (find-var expr ctx)
	(if found (generate ctx cont 'ilde level offset)
	    (generate ctx cont 'ildg (find-literal expr ctx))))
      (compile-continuation ctx cont)))
  
(def compile-list (expr ctx cont)
  (do (if (spformp (car expr)) (compile-spform expr ctx cont)
	  (inlinablep (car expr)) (compile-inline expr ctx cont)
	  (compile-apply expr ctx cont))
      (compile-continuation ctx cont)))

(def compile-continuation (ctx cont)
  (if cont (generate ctx 'iret) ctx))

(def literalp (expr)
  (let tp (type expr)
    (or (no expr)
	(is expr t)
	(is tp 'char)
	(is tp 'string)
	(is tp 'int)
	(is tp 'num))))

;; This should be a C function (not standard Arc), but is defined here
;; for testing.
(def fixnump (val)
  (and (is (type val) 'int)
       (>= val -9223372036854775808)	; valid for 64-bit arches
       (<= val 9223372036854775807)))

(def list-append (list2 list)
  (if (no (cdr list)) (scdr list list2)
      (list-append list2 (cdr list))))

;; This should be a C function as well.  The context argument is a
;; compiler context, which is in the CArc runtime an opaque type
;; which can only be manipulated from within an Arc function in
;; limited ways.  For development purposes, we let it be a list of
;; three elements, giving a list of instructions, an environment
;; structure, and a list of literals used in the code being
;; generated.
(def generate (ctx opcode . args)
  (do (let code (car ctx)
	(if (no code)
	    (scar ctx (cons opcode args))
	    (list-append (cons opcode args) code)))
      ctx))

;; This should be a C function as well.  If the literal +lit+ is not
;; found in the literal list of the context, it will add the literal
;; to the context.
(def find-literal (lit ctx)
  (let literals (car (cddr ctx))
    (if (no literals)
	(do (scar (cddr ctx) (cons lit nil)) 0)
	(aif ((afn (lit literals idx)
	       (if (no literals) nil
		   (is (car literals) lit) idx
		   (self lit (cdr literals) (+ 1 idx)))) lit literals 0)
	     it
	     (do1 (len literals)
		  (list-append (cons lit nil) literals))))))

;; These should be parts of a C function as well.  Environments are
;; defined as lists of vectors (they appear here as lists as well because
;; standard Arc doesn't have vectors).  The first element of the
;; environment is all we really care about.
(def find-in-frame (var frame idx)
  (if (no frame) nil
      (is (car frame) var) idx
      (find-in-frame var (cdr frame) (+ idx 1))))

(def find-in-env (var env idx)
  (if (no env) '(nil 0 0)
      (aif (find-in-frame var (car (car env)) 0) `(t ,idx ,it)
	   (find-in-env var (cdr env) (+ idx 1)))))

(def find-var (var ctx)
  (do prn ctx
      (find-in-env var (cadr ctx) 0)))
