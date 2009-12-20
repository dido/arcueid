;; This is Will Fitzgerald's spec.arc.  Reproduced with permission.
;; See http://willwhim.wordpress.com/2008/02/02/more-better-spec-for-arc/
;; for more details.

(= first car)
(= rest cdr)
(= second cadr)
(def third (list) (car (cddr list)))
(def fourth (list) (car (nthcdr 3 list)))
(def fifth (list) (car (nthcdr 4 list)))
(def sixth (list) (car (nthcdr 5 list)))

;; a result is the description, the body (expected) and a value (nil or non-nil)
(def def-result (desc body value) (list desc body value))
(def result-desc (result) (first result))
(def result-body (result) (second result))
(def result-value (result) (third result))
;; a spec result is the description plus the results
(def def-results (desc results) (list desc results))
(def results-description (results) (first results))
(def results-results (results) (second results))

(def print-results (results (o all nil))
     (pr ";; " (results-description results) "\n")
     (with (totals 0 goods 0 errors 0)
       (each result (results-results results)
         (++ totals)
           (when (result-value result)
             (if (is 'exception (type (result-value result)))
                 (++ errors)
                 (++ goods)))
           (when (or all (no (result-value result)))
             (pr ";; " (result-desc result) ": " (result-value result) "\t"
                 (first (result-body result)) "\n")))
       (pr ";; Tests: " totals "; Good: " goods "; Errors: " errors "; Pct: " (* 100.0 (/ goods totals)) "\n")
       (if (is totals goods)
           'green
           'red)))
            
(def assocs (key list)
     (keep [caris _ key] list))

(mac do2 args
  (w/uniq g
    `(do
       ,(car args)
         (let ,g ,(cadr args)
       ,@(cddr args)
       ,g))))

(mac describe body
  (with (desc (car body)
     prolog (cdr (assoc 'prolog (cdr body)))
     epilog (cdr (assoc 'epilog (cdr body)))
     setup  (cdr (assoc 'setup (cdr body)))
     teardown (cdr (assoc 'teardown (cdr body)))
     its (assocs 'it (cdr body)))
     `(fn ()
        (def-results ,desc
          (do2
           (do ,@prolog)
           (list
            ,@(map
               (fn (it)
                   `(do2
                     (do ,@setup)
                     (def-result
                       ,(cadr it)
                       ',(cddr it)
                       (on-err (fn (err) err) (fn () ,@(cddr it))))
                     (do ,@teardown)))
               its))
           (do ,@epilog))))))
             
