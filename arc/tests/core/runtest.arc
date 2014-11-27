; This software is copyright (c) Conan Dalton 2008. Permission to use
; it is granted under the Perl Foundations's Artistic License 2.0.

(def rat ()
  (assign all-tests nil)
  (let loader [load (string _ ".arc")]
    (map loader '(foundation-test
                  misc-tests
                  core-errors-continuations-test
                  ; core-evaluation-test
                  core-lists-test
                  core-macros-test
                  core-maths-test
                  core-predicates-test
                  core-special-forms-test
                  core-typing-test))
                  ;; parser-test))
    (run-all-tests)))
