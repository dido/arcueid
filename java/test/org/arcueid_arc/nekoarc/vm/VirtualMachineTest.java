package org.arcueid_arc.nekoarc.vm;

import static org.junit.Assert.*;

import org.arcueid_arc.nekoarc.NekoArcException;
import org.arcueid_arc.nekoarc.Nil;
import org.arcueid_arc.nekoarc.types.Fixnum;
import org.junit.Test;

public class VirtualMachineTest {

	@Test
	public void testHLT() throws NekoArcException
	{
		byte inst[] = { 0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Fixnum.get(1234));
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(1234, ((Fixnum)vm.getAcc()).fixnum);
		assertEquals(1, vm.getIp());
	}

	@Test
	public void testNOP() throws NekoArcException
	{
		byte inst[] = { 0x00, 0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Fixnum.get(1234));
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(1234, ((Fixnum)vm.getAcc()).fixnum);
		assertEquals(2, vm.getIp());
	}

	@Test
	public void testNIL() throws NekoArcException
	{
		byte inst[] = { 0x13, 0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Fixnum.get(1234));
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(Nil.NIL, vm.getAcc());
		assertEquals(2, vm.getIp());
	}

	@Test
	public void testInstArg()
	{
		byte data[] = { 0x01, 0x00, 0x00, 0x00 };
		VirtualMachine vm = new VirtualMachine(1024);

		vm.load(data, 0);
		assertEquals(1, vm.instArg());
		
		byte data2[] = { (byte) 0xff, 0x00, 0x00, 0x00 };
		vm.load(data2, 0);
		assertEquals(255, vm.instArg());

		byte data3[] = { (byte) 0xff, (byte) 0xff, (byte) 0xff, (byte) 0xff };
		vm.load(data3, 0);
		assertEquals(-1, vm.instArg());
	}

	@Test
	public void testLDI() throws NekoArcException
	{
		// ldi 1; hlt
		byte inst[] = { 0x44, 0x01, 0x00, 0x00, 0x00, 0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(Fixnum.get(1), vm.getAcc());
		assertEquals(6, vm.getIp());
		
	}
}
