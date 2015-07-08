package org.arcueidarc.nekoarc.vm;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.HeapContinuation;
import org.arcueidarc.nekoarc.HeapEnv;
import org.arcueidarc.nekoarc.Nil;
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

}
