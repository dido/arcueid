package org.arcueidarc.nekoarc.vm.instruction;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.types.Cons;
import org.arcueidarc.nekoarc.vm.VirtualMachine;
import org.junit.Test;

public class APPLYtest
{
	@Test
	public void testCons()
	{
		// cont 13; ldi 2; push; ldl 0; apply 1; ret
		byte inst[] = { (byte)0x89, 0x0d, 0x00, 0x00, 0x00,
				0x44, 0x02, 0x00, 0x00, 0x00,
				0x01,
				0x43, 0x00, 0x00, 0x00, 0x00,
				0x4c, 0x01,
				0x0d };
		ArcObject literals[] = new ArcObject[1];
		literals[0] = new Cons(Fixnum.get(10), new Cons(Fixnum.get(11), new Cons(Fixnum.get(12), Nil.NIL)));
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0, literals);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(12, ((Fixnum)vm.getAcc()).fixnum);
		assertEquals(19, vm.getIP());
		assertEquals(0, vm.getSP());
	}

}
