package org.arcueidarc.nekoarc.vm.instruction;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.types.Symbol;
import org.arcueidarc.nekoarc.vm.VirtualMachine;
import org.junit.Test;

public class LDGtest
{
	@Test
	public void test()
	{
		// ldg 0; hlt
		byte inst[] = { 0x45, 0x00, 0x00, 0x00, 0x00, 0x14 };
		ArcObject literals[] = new ArcObject[1];
		Symbol sym = (Symbol)Symbol.intern("foo");
		literals[0] = sym;
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0, literals);
		vm.setAcc(Nil.NIL);
		vm.bind(sym, Fixnum.get(1234));
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(1234, ((Fixnum)vm.getAcc()).fixnum);
		assertEquals(6, vm.getIP());
	}

}
