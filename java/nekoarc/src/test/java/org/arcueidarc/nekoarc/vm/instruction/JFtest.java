package org.arcueidarc.nekoarc.vm.instruction;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.vm.VirtualMachine;
import org.junit.Test;

public class JFtest
{
	@Test
	public void testTrue1()
	{
		// ldi 1; jf 1; nil; hlt
		byte inst[] = { 0x44, (byte) 0x01, (byte) 0x00, (byte) 0x00, (byte) 0x00,
				0x50, 0x01, 0x00, 0x00, 0x00,
				0x13,
				0x14};
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Fixnum.get(1234));
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(Nil.NIL, vm.getAcc());
		assertEquals(12, vm.getIP());

	}

	@Test
	public void testTrue2()
	{
		// true; jf 1; nil; hlt
		byte inst[] = { (byte) 0x12,
				0x50, 0x01, 0x00, 0x00, 0x00,
				0x13,
				0x14};
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Fixnum.get(1234));
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(Nil.NIL, vm.getAcc());
		assertEquals(8, vm.getIP());
	}

	@Test
	public void testFalse()
	{
		// nil; jf 5; ldi 1; hlt
		byte inst[] = { (byte) 0x13,
				0x50, 0x05, 0x00, 0x00, 0x00,
				0x44, (byte) 0x03, (byte) 0x00, (byte) 0x00, (byte) 0x00,
				0x14};
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Fixnum.get(1234));
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(Nil.NIL, vm.getAcc());
		assertEquals(12, vm.getIP());
	}
}
