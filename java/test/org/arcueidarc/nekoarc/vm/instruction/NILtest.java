package org.arcueidarc.nekoarc.vm.instruction;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.vm.VirtualMachine;
import org.junit.Test;

public class NILtest
{
	@Test
	public void test() throws NekoArcException
	{
		byte inst[] = { 0x13, 0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Fixnum.get(1234));
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(Nil.NIL, vm.getAcc());
		assertEquals(2, vm.getIP());
	}

}
