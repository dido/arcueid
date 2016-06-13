package org.arcueidarc.nekoarc.vm.instruction;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Symbol;
import org.arcueidarc.nekoarc.vm.VirtualMachine;
import org.junit.Test;

public class LDLtest
{
	@Test
	public void test() throws NekoArcException
	{
		// ldl 0; hlt
		byte inst[] = { 0x43, 0x00, 0x00, 0x00, 0x00, 0x14 };
		ArcObject literals[] = new ArcObject[1];
		literals[0] = Symbol.intern("foo");
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0, literals);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(Symbol.intern("foo"), vm.getAcc());
		assertEquals(6, vm.getIP());		
	}
}
