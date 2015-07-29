package org.arcueidarc.nekoarc.functions;

import static org.junit.Assert.*;

import org.junit.Test;

public class CCCtest
{
	@Test
	public void test()
	{
		// A rather contrived use of ccc to do something sort of useful as a test. (fn (treat lst) (ccc (fn (return) (each x lst (treat x return))))
		// Apply with something like (fn (x return) (if (is x 10) (return (+ x 1)))) and '(1 2 3 10 5). Should produce 11.
		// env 2 0 0; ldl 0 [inner fn]; cls; push; ldl 1 [ccc]; apply 1; ret
		// [inner fn]
		// env 1 0 0; L0: lde 1 1; cdr; jt L1; ret; L1: lde 1 1; car; push; lde0 0; push; lde 1 0; apply 2; jmp L0;
		// [treat]
		// env 2 0 0; lde0 0; push; ldl 10; is; jt L0; ret; lde0 0; push; ldl 1; add; push; lde0 1; apply 1; ret
		fail("Not yet implemented");
	}

}
