package org.arcueidarc.nekoarc.vm.instruction;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.Unbound;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.vm.VirtualMachine;
import org.junit.Test;

public class LDE0test
{
	@Test
	public void test0()
	{
		// ldi 1; push; ldi 2; push; ldi 3; push; env 3 2; lde0 0; hlt;
		byte inst[] = { 0x44, 0x01, 0x00, 0x00, 0x00,
						0x01,
						0x44, 0x02, 0x00, 0x00, 0x00,
						0x01,
						0x44, 0x03, 0x00, 0x00, 0x00,
						0x01,
						(byte)0xca, 0x03, 0x01, 0x01,
						(byte)0x69, 0x00,
						0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setargc(3);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(Fixnum.get(1), vm.getAcc());
		assertEquals(25, vm.getIP());
	}

	@Test
	public void test1()
	{
		// ldi 1; push; ldi 2; push; ldi 3; push; env 3 2; lde0 1; hlt;
		byte inst[] = { 0x44, 0x01, 0x00, 0x00, 0x00,
						0x01,
						0x44, 0x02, 0x00, 0x00, 0x00,
						0x01,
						0x44, 0x03, 0x00, 0x00, 0x00,
						0x01,
						(byte)0xca, 0x03, 0x01, 0x01,
						(byte)0x69, 0x01,
						0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setargc(3);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(Fixnum.get(2), vm.getAcc());
		assertEquals(25, vm.getIP());
	}

	@Test
	public void test2()
	{
		// ldi 1; push; ldi 2; push; ldi 3; push; env 3 2; lde0 2; hlt;
		byte inst[] = { 0x44, 0x01, 0x00, 0x00, 0x00,
						0x01,
						0x44, 0x02, 0x00, 0x00, 0x00,
						0x01,
						0x44, 0x03, 0x00, 0x00, 0x00,
						0x01,
						(byte)0xca, 0x03, 0x01, 0x01,
						(byte)0x69, 0x02,
						0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setargc(3);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(Fixnum.get(3), vm.getAcc());
		assertEquals(25, vm.getIP());
	}

	@Test
	public void test3()
	{
		// ldi 1; push; ldi 2; push; ldi 3; push; env 3 2; lde0 3; hlt;
		byte inst[] = { 0x44, 0x01, 0x00, 0x00, 0x00,
						0x01,
						0x44, 0x02, 0x00, 0x00, 0x00,
						0x01,
						0x44, 0x03, 0x00, 0x00, 0x00,
						0x01,
						(byte)0xca, 0x03, 0x01, 0x01,
						(byte)0x69, 0x03,
						0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setargc(3);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertTrue(vm.getAcc().is(Unbound.UNBOUND));
		assertEquals(25, vm.getIP());
	}

	@Test
	public void test4()
	{
		// ldi 1; push; ldi 2; push; ldi 3; push; env 3 2; lde 0 0; hlt;
		byte inst[] = { 0x44, 0x01, 0x00, 0x00, 0x00,
						0x01,
						0x44, 0x02, 0x00, 0x00, 0x00,
						0x01,
						0x44, 0x03, 0x00, 0x00, 0x00,
						0x01,
						(byte)0xca, 0x03, 0x01, 0x01,
						(byte)0x69, 0x03,
						0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setargc(3);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertTrue(vm.getAcc().is(Unbound.UNBOUND));
		assertEquals(25, vm.getIP());
	}

}
