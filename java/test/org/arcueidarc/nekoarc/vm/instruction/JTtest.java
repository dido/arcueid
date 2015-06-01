package org.arcueidarc.nekoarc.vm.instruction;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.True;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.vm.VirtualMachine;
import org.junit.Test;

public class JTtest
{
	@Test
	public void testTrue1()
	{
		// ldi 1; jt 1; nil; hlt
		byte inst[] = { 0x44, (byte) 0x01, (byte) 0x00, (byte) 0x00, (byte) 0x00,
				0x4f, 0x01, 0x00, 0x00, 0x00,
				0x13,
				0x14};
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Fixnum.get(1234));
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(1, ((Fixnum)vm.getAcc()).fixnum);
		assertEquals(12, vm.getIP());

	}

	@Test
	public void testTrue2()
	{
		// true; jt 1; nil; hlt
		byte inst[] = { (byte) 0x12,
				0x4f, 0x01, 0x00, 0x00, 0x00,
				0x13,
				0x14};
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Fixnum.get(1234));
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(True.T, vm.getAcc());
		assertEquals(8, vm.getIP());
	}

	@Test
	public void testFalse()
	{
		// nil; jt 1; ldi 1; hlt
		byte inst[] = { (byte) 0x13,
				0x4f, 0x05, 0x00, 0x00, 0x00,
				0x44, (byte) 0x03, (byte) 0x00, (byte) 0x00, (byte) 0x00,
				0x14};
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Fixnum.get(1234));
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(3, ((Fixnum)vm.getAcc()).fixnum);
		assertEquals(12, vm.getIP());
	}

}
