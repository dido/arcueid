package org.arcueidarc.nekoarc.vm.instruction;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Cons;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.vm.VirtualMachine;
import org.junit.Test;

public class SCARtest
{
	@Test
	public void test()
	{
		// ldl 0; push; ldi 3; scar; hlt 
		byte inst[] = {0x43, 0x00, 0x00, 0x00, 0x00,
				0x01,
				0x44, (byte) 0x03, (byte) 0x00, (byte) 0x00, (byte) 0x00,
				0x1c,
				0x14};
		ArcObject literals[] = new ArcObject[1];
		literals[0] = new Cons(Fixnum.get(1), Fixnum.get(2));
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0, literals);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(3, ((Fixnum)((Cons)vm.getAcc()).car()).fixnum);
		assertEquals(2, ((Fixnum)((Cons)vm.getAcc()).cdr()).fixnum);
		assertEquals(3, ((Fixnum)((Cons)literals[0]).car()).fixnum);
		assertEquals(2, ((Fixnum)((Cons)literals[0]).cdr()).fixnum);
		assertEquals(13, vm.getIP());
	}

}
