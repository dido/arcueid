package org.arcueidarc.nekoarc.vm.instruction;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.vm.VirtualMachine;
import org.junit.Test;

public class LDItest
{
	@Test
	public void testLDI() throws NekoArcException
	{
		// ldi 1; hlt
		byte inst[] = { 0x44, 0x01, 0x00, 0x00, 0x00, 0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(Fixnum.get(1), vm.getAcc());
		assertEquals(6, vm.getIP());
		
	}
}
