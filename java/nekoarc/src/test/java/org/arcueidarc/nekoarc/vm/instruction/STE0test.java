package org.arcueidarc.nekoarc.vm.instruction;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.Unbound;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.vm.VirtualMachine;
import org.junit.Test;

public class STE0test
{
	@Test
	public void test0()
	{
		// ldi 1; push; ldi 2; push; ldi 3; push; env 3 2; nil; ste0 0; hlt;
		byte inst[] = { 0x44, 0x01, 0x00, 0x00, 0x00,
						0x01,
						0x44, 0x02, 0x00, 0x00, 0x00,
						0x01,
						0x44, 0x03, 0x00, 0x00, 0x00,
						0x01,
						(byte)0xca, 0x03, 0x01, 0x01,
						0x13,
						(byte)0x6a, 0x00,
						0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setargc(3);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertTrue(vm.getenv(0, 0).is(Nil.NIL));
		assertEquals(Fixnum.get(2), vm.getenv(0, 1));
		assertEquals(Fixnum.get(3), vm.getenv(0, 2));
		assertTrue(Unbound.UNBOUND.is(vm.getenv(0, 3)));
		assertTrue(Unbound.UNBOUND.is(vm.getenv(0, 4)));
		assertEquals(26, vm.getIP());
	}

	@Test
	public void test1()
	{
		// ldi 1; push; ldi 2; push; ldi 3; push; env 3 2; nil; ste0 1; hlt;
		byte inst[] = { 0x44, 0x01, 0x00, 0x00, 0x00,
						0x01,
						0x44, 0x02, 0x00, 0x00, 0x00,
						0x01,
						0x44, 0x03, 0x00, 0x00, 0x00,
						0x01,
						(byte)0xca, 0x03, 0x01, 0x01,
						0x13,
						(byte)0x6a, 0x01,
						0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setargc(3);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(Fixnum.get(1), vm.getenv(0, 0));
		assertTrue(vm.getenv(0, 1).is(Nil.NIL));
		assertEquals(Fixnum.get(3), vm.getenv(0, 2));
		assertTrue(Unbound.UNBOUND.is(vm.getenv(0, 3)));
		assertTrue(Unbound.UNBOUND.is(vm.getenv(0, 4)));
		assertEquals(26, vm.getIP());
	}

	@Test
	public void test2()
	{
		// ldi 1; push; ldi 2; push; ldi 3; push; env 3 2; nil; ste0 2; hlt;
		byte inst[] = { 0x44, 0x01, 0x00, 0x00, 0x00,
						0x01,
						0x44, 0x02, 0x00, 0x00, 0x00,
						0x01,
						0x44, 0x03, 0x00, 0x00, 0x00,
						0x01,
						(byte)0xca, 0x03, 0x01, 0x01,
						0x13,
						(byte)0x6a, 0x02,
						0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setargc(3);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(Fixnum.get(1), vm.getenv(0, 0));
		assertEquals(Fixnum.get(2), vm.getenv(0, 1));
		assertTrue(vm.getenv(0, 2).is(Nil.NIL));
		assertTrue(Unbound.UNBOUND.is(vm.getenv(0, 3)));
		assertTrue(Unbound.UNBOUND.is(vm.getenv(0, 4)));
		assertEquals(26, vm.getIP());
	}

	@Test
	public void test3()
	{
		// ldi 1; push; ldi 2; push; ldi 3; push; env 3 2; nil; ste0 3; hlt;
		byte inst[] = { 0x44, 0x01, 0x00, 0x00, 0x00,
						0x01,
						0x44, 0x02, 0x00, 0x00, 0x00,
						0x01,
						0x44, 0x03, 0x00, 0x00, 0x00,
						0x01,
						(byte)0xca, 0x03, 0x01, 0x01,
						0x13,
						(byte)0x6a, 0x03,
						0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setargc(3);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(Fixnum.get(1), vm.getenv(0, 0));
		assertEquals(Fixnum.get(2), vm.getenv(0, 1));
		assertEquals(Fixnum.get(3), vm.getenv(0, 2));
		assertTrue(vm.getenv(0, 3).is(Nil.NIL));
		assertTrue(Unbound.UNBOUND.is(vm.getenv(0, 4)));
		assertEquals(26, vm.getIP());
	}

	@Test
	public void test4()
	{
		// ldi 1; push; ldi 2; push; ldi 3; push; env 3 2; nil; ste0 4; hlt;
		byte inst[] = { 0x44, 0x01, 0x00, 0x00, 0x00,
						0x01,
						0x44, 0x02, 0x00, 0x00, 0x00,
						0x01,
						0x44, 0x03, 0x00, 0x00, 0x00,
						0x01,
						(byte)0xca, 0x03, 0x01, 0x01,
						0x13,
						(byte)0x6a, 0x04,
						0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setargc(3);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(Fixnum.get(1), vm.getenv(0, 0));
		assertEquals(Fixnum.get(2), vm.getenv(0, 1));
		assertEquals(Fixnum.get(3), vm.getenv(0, 2));
		assertTrue(Unbound.UNBOUND.is(vm.getenv(0, 3)));
		assertTrue(vm.getenv(0, 4).is(Nil.NIL));
		assertEquals(26, vm.getIP());
	}

}
