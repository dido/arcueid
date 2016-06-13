package org.arcueidarc.nekoarc.vm;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.HeapContinuation;
import org.arcueidarc.nekoarc.HeapEnv;
import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Closure;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.junit.Test;

public class HeapContinuationTest
{
	/** First we try to invent a HeapContinuation out of whole cloth. */
	@Test
	public void test()
	{
		HeapContinuation hc;

		// Prepare an environment
		HeapEnv env = new HeapEnv(3, Nil.NIL);
		env.setEnv(0, Fixnum.get(4));
		env.setEnv(1, Fixnum.get(5));
		env.setEnv(2, Fixnum.get(6));

		// Synthetic HeapContinuation
		hc = new HeapContinuation(3,		// 3 stack elements
				Nil.NIL,					// previous continuation
				env,						// environment
				20);						// saved IP
		// Stack elements
		hc.setIndex(0, Fixnum.get(1));
		hc.setIndex(1, Fixnum.get(2));
		hc.setIndex(2, Fixnum.get(3));

		// Code:
		// env 2 0 0; lde0 0; push; lde0 1; apply 1; ret; hlt; hlt; hlt; hlt; hlt;
		// add everything already on the stack to the value applied to the continuation, get the
		// stuff in the environment and add it too, and return the sum.
		// add; add; add; push; lde0 0; add; push; lde0 1; add; push; lde0 2; add; ret
		byte inst[] = { (byte)0xca, 0x02, 0x00, 0x00,	// env 1 0 0
				0x69, 0x00,								// lde0 0  ; value (fixnum)
				0x01,									// push
				0x69, 0x01,								// lde0 1  ; continuation
				0x4c, 0x01,								// apply 1
				0x0d,									// ret
				0x14,									// hlt
				0x14,									// hlt
				0x14,									// hlt
				0x14,									// hlt
				0x14,									// hlt
				0x14,									// hlt
				0x14,									// hlt
				0x14,									// hlt
				0x15,									// add		; continuation ip address
				0x15,									// add
				0x15,									// add
				0x01,									// push
				0x69, 0x00,								// lde0 0
				0x15,									// add
				0x01,									// push
				0x69, 0x01,								// lde0 1
				0x15,									// add
				0x01,									// push
				0x69, 0x02,								// lde0 2
				0x15,									// add
				0x0d,									// ret
		};
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setargc(2);
		vm.push(Fixnum.get(7));
		vm.push(hc);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(28, ((Fixnum)vm.getAcc()).fixnum);
	}

	/** Next, we make a function that recurses several times, essentially
	 *  (afn (x) (if (is x 0) 0 (+ (self (- x 1)) 2)))
	 *  If stack space is very limited on the VM, the continuations that the recursive calls need to make should
	 *  eventually migrate from the stack to the heap.
	 */
	@Test
	public void test2()
	{
		// env 1 0 0; lde0 0; push; ldi 0; is; jf L1; ldi 2; ret;
		// L1: cont L2; lde0 0; push; ldi 1; sub; push; ldl 0; apply 1; L2: push; ldi 2; add; ret
		byte inst[] = { (byte)0xca, 0x01, 0x00, 0x00,	// env 1 0 0
				0x69, 0x00,								// lde0 0
				0x01,									// push
				0x44, 0x00, 0x00, 0x00, 0x00,			// ldi 0
				0x1f,									// is
				0x50, 0x06, 0x00, 0x00, 0x00,			// jf L1 (6)
				0x44, 0x00, 0x00, 0x00, 0x00,			// ldi 0
				0x0d,									// ret
				(byte)0x89, 0x11, 0x00, 0x00, 0x00,		// L1: cont L2 (0x11)
				0x69, 0x00,								// lde0 0
				0x01,									// push
				0x44, 0x01, 0x00, 0x00, 0x00,			// ldi 1
				0x16,									// sub
				0x01,									// push
				0x43, 0x00, 0x00, 0x00, 0x00,			// ldl 0
				0x4c, 0x01,								// apply 1
				0x01,									// L2: push
				0x44, 0x02, 0x00, 0x00, 0x00,			// ldi 2
				0x15,									// add
				0x0d									// ret
		};
		VirtualMachine vm = new VirtualMachine(4);
		ArcObject literals[] = new ArcObject[1];
		literals[0] = new Closure(Nil.NIL, Fixnum.get(0));
		vm.load(inst, 0, literals);
		vm.setargc(1);
		vm.push(Fixnum.get(100));
		vm.setAcc(literals[0]);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(200, ((Fixnum)vm.getAcc()).fixnum);
	}

	/** Next, we make a slight variation
	 *  (afn (x) (if (is x 0) 0 (+ 2 (self (- x 1)))))
	 */
	@Test
	public void test3()
	{
		// env 1 0 0; lde0 0; push; ldi 0; is; jf L1; ldi 2; ret;
		// L1: ldi 2; push; cont L2; lde0 0; push; ldi 1; sub; push; ldl 0; apply 1; L2: add; ret
		byte inst[] = { (byte)0xca, 0x01, 0x00, 0x00,	// env 1 0 0
				0x69, 0x00,								// lde0 0
				0x01,									// push
				0x44, 0x00, 0x00, 0x00, 0x00,			// ldi 0
				0x1f,									// is
				0x50, 0x06, 0x00, 0x00, 0x00,			// jf L1 (6)
				0x44, 0x00, 0x00, 0x00, 0x00,			// ldi 0
				0x0d,									// ret
				0x44, 0x02, 0x00, 0x00, 0x00,			// L1: ldi 2
				0x01,									// push
				(byte)0x89, 0x11, 0x00, 0x00, 0x00,		// cont L2 (0x11)
				0x69, 0x00,								// lde0 0
				0x01,									// push
				0x44, 0x01, 0x00, 0x00, 0x00,			// ldi 1
				0x16,									// sub
				0x01,									// push
				0x43, 0x00, 0x00, 0x00, 0x00,			// ldl 0
				0x4c, 0x01,								// apply 1
				0x15,									// L2: add
				0x0d									// ret
		};
		VirtualMachine vm = new VirtualMachine(5);
		ArcObject literals[] = new ArcObject[1];
		literals[0] = new Closure(Nil.NIL, Fixnum.get(0));
		vm.load(inst, 0, literals);
		vm.setargc(1);
		vm.push(Fixnum.get(1));
		vm.setAcc(literals[0]);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(2, ((Fixnum)vm.getAcc()).fixnum);
	}

}
