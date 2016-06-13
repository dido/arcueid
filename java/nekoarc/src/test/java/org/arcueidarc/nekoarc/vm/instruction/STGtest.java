package org.arcueidarc.nekoarc.vm.instruction;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.types.Symbol;
import org.arcueidarc.nekoarc.vm.VirtualMachine;
import org.junit.Test;

public class STGtest
{

	@Test
	public void test()
	{
		// ldi 0x7f, stg 0; hlt
		byte inst[] = { 0x44, 0x7f, 0x00, 0x00, 0x00,
				0x46, 0x00, 0x00, 0x00, 0x00, 0x14 };
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
		assertEquals(0x7f, ((Fixnum)vm.getAcc()).fixnum);
		assertEquals(0x7f, ((Fixnum)vm.value(sym)).fixnum);
		assertEquals(11, vm.getIP());
	}

}
