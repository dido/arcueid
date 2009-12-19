(load "spec.arc")
(load "../compiler.arc")

(= test-literals
   (describe "Literal management function"
	     (prolog (= ctx '(nil nil nil)))
	     (it "should add a literal not already present to the list"
		 (is (find-literal 1.0 ctx) 0))
	     (it "should add another literal to the list"
		 (is (find-literal 2.0 ctx) 1))
	     (it "should not add the same literal to the list twice"
		 (is (find-literal 1.0 ctx) 0))
	     (it "should handle symbols"
		 (is (find-literal 'some-symbol ctx) 2))
	     (it "should not add the symbol more than once"
		 (is (find-literal 'some-symbol ctx) 2))
	     (it "should have added three elements to the literal list"
		 (is (len (car:cddr ctx)) 3))))
(= test-codegen
   (describe "Code generation"
	     (prolog (= ctx '(nil nil nil)))
	     (it "should generate an instruction with no arguments"
		 (do (generate ctx 'inop)
		     (and (is (len (car ctx)) 1)
			  (is (caar ctx) 'inop))))
	     (it "should generate an instruction with one argument"
		 (do (generate ctx 'ildi 31337)
		     (and (is (len (car ctx)) 3)
			  (is ((car ctx) 1) 'ildi)
			  (is ((car ctx) 2) 31337))))
	     (it "should generate an instruction with two arguments"
		 (do (generate ctx 'ilde 1 2)
		     (and (is (len (car ctx)) 6)
			  (is ((car ctx) 3) 'ilde)
			  (is ((car ctx) 4) 1)
			  (is ((car ctx) 5) 2))))))

(= test-compile-literal
   (describe "The compilation of literals"
	     (it "should compile character literals"
		 (do (= ctx '(nil nil nil))
		     (compile #\a ctx nil)
		     (and (is ((car ctx) 0) 'ildl)
			  (is ((car ctx) 1) 0)
			  (is (len (car:cddr ctx)) 1)
			  (is ((car:cddr ctx) 0) #\a))))
	     (it "should compile string literals"
		 (do (= ctx '(nil nil nil))
		     (compile "foo" ctx nil)
		     (and (is ((car ctx) 0) 'ildl)
			  (is ((car ctx) 1) 0)
			  (is (len (car:cddr ctx)) 1)
			  (is ((car:cddr ctx) 0) "foo"))))
	     (it "should compile nils"
		 (do (= ctx '(nil nil nil))
		     (compile nil ctx nil)
		     (is ((car ctx) 0) 'inil)))
	     (it "should compile t"
		 (do (= ctx '(nil nil nil))
		     (compile t ctx nil)
		     (is ((car ctx) 0) 'itrue)))
	     (it "should compile fixnums"
		 (do (= ctx '(nil nil nil))
		     (compile 1234 ctx nil)
		     (and (is ((car ctx) 0) 'ildi)
			  (is ((car ctx) 1) (+ (* 1234 2) 1)))))
	     (it "should compile bignums"
		 (do (= ctx '(nil nil nil))
		     (compile 340282366920938463463374607431768211456 ctx nil)
		     (and (is ((car ctx) 0) 'ildl)
			  (is ((car ctx) 1) 0)
			  (is (len (car:cddr ctx)) 1)
			  (is ((car:cddr ctx) 0) 340282366920938463463374607431768211456))))
	     (it "should compile flonums"
		 (do (= ctx '(nil nil nil))
		     (compile 3.1415926535 ctx nil)
		     (and (is ((car ctx) 0) 'ildl)
			  (is ((car ctx) 1) 0)
			  (is (len (car:cddr ctx)) 1)
			  (< (abs (- ((car:cddr ctx) 0) 3.1415926535)) 1e-6))))))

(print-results (test-literals) t)
(print-results (test-codegen) t)
(print-results (test-compile-literal) t)
