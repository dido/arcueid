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

(print-results (test-literals) t)
(print-results (test-codegen) t)
