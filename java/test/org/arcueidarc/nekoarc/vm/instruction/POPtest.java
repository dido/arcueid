package org.arcueidarc.nekoarc.vm.instruction;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.vm.VirtualMachine;
import org.junit.Test;

public class POPtest
{
	@Test
	public void test() throws NekoArcException
	{
		// ldi 555279757; push; ldi -555729757; pop; hlt
		byte inst[] = { 0x44, (byte) 0x5d, (byte) 0xc3, (byte) 0x1f, (byte) 0x21, 0x01,
				0x44, (byte) 0xa3, (byte) 0x3c, (byte) 0xe0, (byte) 0xde, 0x02, 0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(Fixnum.get(555729757), vm.getAcc());
		assertEquals(0, vm.getSP());
		assertEquals(13, vm.getIP());
	}
}
