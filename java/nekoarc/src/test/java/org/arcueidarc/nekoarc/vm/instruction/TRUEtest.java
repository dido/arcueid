package org.arcueidarc.nekoarc.vm.instruction;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.True;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.vm.VirtualMachine;
import org.junit.Test;

public class TRUEtest
{
	@Test
	public void testTRUE() throws NekoArcException
	{
		byte inst[] = { 0x12, 0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Fixnum.get(1234));
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(True.T, vm.getAcc());
		assertEquals(2, vm.getIP());
	}
}
