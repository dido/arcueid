package org.arcueidarc.nekoarc.vm.instruction;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.True;
import org.arcueidarc.nekoarc.Unbound;
import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.vm.VirtualMachine;
import org.junit.Test;

public class JBNDtest
{
	@Test
	public void testBound1()
	{
		// ldi 1; jbnd 1; nil; hlt
		byte inst[] = { 0x44, (byte) 0x01, (byte) 0x00, (byte) 0x00, (byte) 0x00,
				0x51, 0x01, 0x00, 0x00, 0x00,
				0x13,
				0x14};
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Fixnum.get(1234));
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(Fixnum.get(1).fixnum, ((Fixnum)vm.getAcc()).fixnum);
		assertEquals(12, vm.getIP());
	}

	@Test
	public void testBound2()
	{
		// true; jbnd 1; nil; hlt
		byte inst[] = { (byte) 0x12,
				0x51, 0x01, 0x00, 0x00, 0x00,
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
	public void testBound3()
	{
		// nil; jbnd 1; true; hlt
		byte inst[] = { (byte) 0x13,
				0x51, 0x01, 0x00, 0x00, 0x00,
				0x12,
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
	public void testUnbound()
	{
		// ldl 0; jbnd 1; nil; hlt
		byte inst[] = { 0x43, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
				0x51, 0x01, 0x00, 0x00, 0x00,
				0x12,
				0x14};
		VirtualMachine vm = new VirtualMachine(1024);
		ArcObject literals[] = new ArcObject[1];
		literals[0] = Unbound.UNBOUND;
		vm.load(inst, 0, literals);
		vm.setAcc(Fixnum.get(1234));
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(True.T, vm.getAcc());
		assertEquals(12, vm.getIP());
	}

}
