package org.arcueidarc.nekoarc.vm.instruction;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.Unbound;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.vm.VirtualMachine;
import org.junit.Test;

public class ENVtest
{
	@Test
	public void test()
	{
		// ldi 1; push; ldi 2; push; ldi 3; push; env 3 1 1; hlt;
		byte inst[] = { 0x44, 0x01, 0x00, 0x00, 0x00,
						0x01,
						0x44, 0x02, 0x00, 0x00, 0x00,
						0x01,
						0x44, 0x03, 0x00, 0x00, 0x00,
						0x01,
						(byte)0xca, 0x03, 0x01, 0x01,
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
		assertTrue(Unbound.UNBOUND.is(vm.getenv(0, 4)));
		assertEquals(23, vm.getIP());
	}

	@Test
	public void testTooFewArgs()
	{
		// ldi 1; push; env 3 1 1; hlt;
		byte inst[] = { 0x44, 0x01, 0x00, 0x00, 0x00,
						0x01,
						(byte)0xca, 0x03, 0x01, 0x01,
						0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setargc(1);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		try {
			vm.run();
			fail("exception not thrown");
		} catch (NekoArcException e) {
			assertEquals("too few arguments, at least 3 required, 1 passed", e.getMessage());
		}
	}

	@Test
	public void testTooManyArgs()
	{
		// ldi 1; push; ldi 1; push; ldi 1; push; ldi 1; push; ldi 1; push; env 3 1 1; hlt;
		byte inst[] = { 0x44, 0x01, 0x00, 0x00, 0x00,
						0x01,
						0x44, 0x01, 0x00, 0x00, 0x00,
						0x01,
						0x44, 0x01, 0x00, 0x00, 0x00,
						0x01,
						0x44, 0x01, 0x00, 0x00, 0x00,
						0x01,
						0x44, 0x01, 0x00, 0x00, 0x00,
						0x01,
						(byte)0xca, 0x03, 0x01, 0x01,
						0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setargc(5);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		try {
			vm.run();
			fail("exception not thrown");
		} catch (NekoArcException e) {
			assertEquals("too many arguments, at most 4 allowed, 5 passed", e.getMessage());
		}
	}
}
