package org.arcueidarc.nekoarc.vm.instruction;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.Unbound;
import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.vm.VirtualMachine;
import org.junit.Test;

public class DCDRtest
{
	@Test
	public void testBasic()
	{
		// ldi 2; push; ldi 1; cons; dcdr; hlt
		byte inst[] = { 0x44, (byte) 0x02, (byte) 0x00, (byte) 0x00, (byte) 0x00,
				0x01,
				0x44, (byte) 0x01, (byte) 0x00, (byte) 0x00, (byte) 0x00,
				0x19,
				0x27,
				0x14};
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(1, ((Fixnum)(vm.getAcc())).fixnum);
		assertEquals(14, vm.getIP());
	}

	@Test
	public void testNil()
	{
		// nil; dcdr; hlt
		byte inst[] = { 0x13, 0x27, 0x14};
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Fixnum.get(1234));
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertTrue(vm.getAcc().is(Unbound.UNBOUND));
		assertEquals(3, vm.getIP());
	}

	@Test
	public void testUnbound()
	{
		// ldl 0; dcdr; hlt
		byte inst[] = { 0x43, 0x00, 0x00, 0x00, 0x00, 0x27, 0x14 };
		ArcObject literals[] = new ArcObject[1];
		literals[0] = Unbound.UNBOUND;
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0, literals);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertTrue(vm.getAcc().is(Unbound.UNBOUND));
		assertEquals(7, vm.getIP());
	}

}
