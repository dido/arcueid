package org.arcueidarc.nekoarc.vm;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.types.Closure;
import org.arcueidarc.nekoarc.Nil;
import org.junit.Test;

public class FibonacciTest
{

	/* This is a recursive Fibonacci test, essentially (afn (x) (if (is x 0) 1 (is x 1) 1 (+ (self (- x 1)) (self (- x 2)))))
	 * Since this isn't tail recursive it runs in exponential time and will use up a lot of stack space for continuations.
	 * If we don't migrate continuations to the heap this will very quickly use up stack space if it's set low enough.
	 */
	@Test
	public void test()
	{
		// env 1 0 0; lde0 0; push; ldi 0; is; jf xxx; ldi 1; ret; lde0 0; push ldi 1; is; jf xxx; ldi 1; ret;
		// cont xxx; lde0 0; push; ldi 1; sub; push; ldl 0; apply 1; push;
		// cont xxx; lde0 0; push; ldi 2; sub; push; ldl 0; apply 1; add; ret
		byte inst[] = { (byte)0xca, 0x01, 0x00, 0x00,	// env 1 0 0
				0x69, 0x00,								// lde0 0
				0x01,									// push
				0x44, 0x00, 0x00, 0x00, 0x00,			// ldi 0
				0x1f,									// is
				0x50, 0x06, 0x00, 0x00, 0x00,			// jf L1 (6)
				0x44, 0x01, 0x00, 0x00, 0x00,			// ldi 1
				0x0d,									// ret
				0x69, 0x00,								// L1: lde0 0
				0x01,									// push
				0x44, 0x01, 0x00, 0x00, 0x00,			// ldi 1
				0x1f,									// is
				0x50, 0x06, 0x00, 0x00, 0x00,			// jf L2 (6)
				0x44, 0x01, 0x00, 0x00, 0x00,			// ldi 1
				0x0d,									// ret
				(byte)0x89, 0x11, 0x00, 0x00, 0x00,		// L2: cont L3 (0x11)
				0x69, 0x00,								// lde0 0
				0x01,									// push
				0x44, 0x01, 0x00, 0x00, 0x00,			// ldi 1
				0x16,									// sub
				0x01,									// push
				0x43, 0x00, 0x00, 0x00, 0x00,			// ldl 0
				0x4c, 0x01,								// apply 1
				0x01,									// L3: push
				(byte)0x89, 0x11, 0x00, 0x00, 0x00,		// cont L4 (0x11)
				0x69, 0x00,								// lde0 0
				0x01,									// push
				0x44, 0x02, 0x00, 0x00, 0x00,			// ldi 2
				0x16,									// sub
				0x01,									// push
				0x43, 0x00, 0x00, 0x00, 0x00,			// ldl 0
				0x4c, 0x01,								// apply 1
				0x15,									// add
				0x0d									// ret
		};
		VirtualMachine vm = new VirtualMachine(12);
		ArcObject literals[] = new ArcObject[1];
		literals[0] = new Closure(Nil.NIL, Fixnum.get(0));
		vm.load(inst, 0, literals);
		vm.setargc(1);
		vm.push(Fixnum.get(8));
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(34, ((Fixnum)vm.getAcc()).fixnum);
	}

}
