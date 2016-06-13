package org.arcueidarc.nekoarc.vm;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.InvokeThread;
import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.functions.Builtin;
import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Closure;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.junit.Test;

/** Test for Java to Arc bytecode calls */
public class Java2ArcTest 
{

	@Test
	public void test()
	{
		Builtin builtin = new Builtin("test", 1, 0, 0, false)
		{
			@Override
			public ArcObject invoke(InvokeThread thr)
			{
				ArcObject arg = thr.getenv(0, 0);
				return(thr.apply(arg, Fixnum.get(1)));
			}
		};
		// Apply the above builtin
		// env 0 0 0; ldl 0; push; ldl 1; apply 1; ret;
		// to (fn (x) (+ x 1))
		// env 1 0 0; lde0 0; push; ldi 1; add; ret
		byte inst[] = { (byte)0xca, 0x00, 0x00, 0x00,	// env 0 0 0
				0x43, 0x00, 0x00, 0x00, 0x00,			// ldl 0
				0x01,									// push
				0x43, 0x01, 0x00, 0x00, 0x00,			// ldl 0
				0x4c, 0x01,								// apply 1
				0x0d,									// ret
				(byte)0xca, 0x01, 0x00, 0x00,			// env 1 0 0
				0x69, 0x00,								// lde0 0
				0x01,									// push
				0x44, 0x01, 0x00, 0x00, 0x00,			// ldi 1
				0x15,									// add
				0x0d									// ret
		};
		VirtualMachine vm = new VirtualMachine(1024);
		ArcObject literals[] = new ArcObject[2];
		literals[0] = new Closure(Nil.NIL, Fixnum.get(18));	// position of second
		literals[1] = builtin;
		vm.load(inst, 0, literals);
		vm.setargc(0);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(2, ((Fixnum)vm.getAcc()).fixnum);
	}

}
