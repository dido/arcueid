package org.arcueidarc.nekoarc.vm.instruction;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.vm.VirtualMachine;
import org.junit.Test;

public class CONTtest
{
	@Test
	public void test()
	{
		// cont 4; ret; hlt; nil; ldi 1; ret
		byte inst[] = { (byte)0x89, 0x03, 0x00, 0x00, 0x00,
					0x0d,
					0x14,
					0x13,
					0x44, 0x01, 0x00, 0x00, 0x00, 0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Fixnum.get(1234));
		vm.makecont(3);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(1, ((Fixnum)vm.getAcc()).fixnum);
	}
}
