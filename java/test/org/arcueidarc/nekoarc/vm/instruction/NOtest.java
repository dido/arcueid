package org.arcueidarc.nekoarc.vm.instruction;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.True;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.vm.VirtualMachine;
import org.junit.Test;

public class NOtest
{
	@Test
	public void testNoNil()
	{
		// nil; no; hlt
		byte inst[] = { 0x13, 0x11, 0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Fixnum.get(1234));
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertTrue(vm.getAcc() instanceof True);
		assertEquals(3, vm.getIP());
	}
	
	@Test
	public void testNoTrue()
	{
		// true; no; hlt
		byte inst[] = { 0x12, 0x11, 0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Fixnum.get(1234));
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertTrue(vm.getAcc() instanceof Nil);
		assertEquals(3, vm.getIP());
	}

	@Test
	public void testNoNumber()
	{
		// ldi 2; no; hlt
		byte inst[] = { 0x44, (byte) 0x02, (byte) 0x00, (byte) 0x00, (byte) 0x00,
				0x11, 0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Fixnum.get(1234));
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertTrue(vm.getAcc() instanceof Nil);
		assertEquals(7, vm.getIP());
	}

}
