package org.arcueidarc.nekoarc.vm;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.Unbound;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.vm.VirtualMachine;
import org.junit.Test;

public class VirtualMachineTest
{	
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

		byte data4[] = { (byte) 0x5d, (byte) 0xc3, (byte) 0x1f, (byte) 0x21 };
		vm.load(data4, 0);
		assertEquals(555729757, vm.instArg());
		
		// two's complement negative
		byte data5[] = { (byte) 0xa3, (byte) 0x3c, (byte) 0xe0, (byte) 0xde };
		vm.load(data5, 0);
		assertEquals(-555729757, vm.instArg());
	}

	@Test
	public void testSmallInstArg()
	{
		byte data[] = { (byte) 0x12, (byte) 0xff };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(data, 0);
		assertEquals(0x12, vm.smallInstArg());
		assertEquals(-1, vm.smallInstArg());
	}

	@Test
	public void testEnv() throws NekoArcException
	{
		VirtualMachine vm = new VirtualMachine(1024);

		try {
			vm.getenv(0, 0);
			fail("exception not thrown");
		} catch (NekoArcException e) {
			assertEquals("environment depth exceeded", e.getMessage());
		}

		try {
			vm.setenv(0, 0, Nil.NIL);
			fail("exception not thrown");
		} catch (NekoArcException e) {
			assertEquals("environment depth exceeded", e.getMessage());
		}

		vm.push(Fixnum.get(1));
		vm.push(Fixnum.get(2));
		vm.push(Fixnum.get(3));
		vm.mkenv(3, 2);

		vm.push(Fixnum.get(4));
		vm.push(Fixnum.get(5));
		vm.mkenv(2, 4);

		vm.setenv(1, 3, Fixnum.get(6));
		vm.setenv(0, 2, Fixnum.get(7));

		assertEquals(1, ((Fixnum)vm.getenv(1, 0)).fixnum);
		assertEquals(2, ((Fixnum)vm.getenv(1, 1)).fixnum);
		assertEquals(3, ((Fixnum)vm.getenv(1, 2)).fixnum);
		assertEquals(6, ((Fixnum)vm.getenv(1, 3)).fixnum);
		assertTrue(vm.getenv(1, 4).is(Unbound.UNBOUND));
		
		assertEquals(4, ((Fixnum)vm.getenv(0, 0)).fixnum);
		assertEquals(5, ((Fixnum)vm.getenv(0, 1)).fixnum);
		assertEquals(7, ((Fixnum)vm.getenv(0, 2)).fixnum);
		assertTrue(vm.getenv(0, 3).is(Unbound.UNBOUND));
		assertTrue(vm.getenv(0, 4).is(Unbound.UNBOUND));
		assertTrue(vm.getenv(0, 5).is(Unbound.UNBOUND));
	}

	@Test
	public void testmenv() throws NekoArcException
	{
		VirtualMachine vm = new VirtualMachine(1024);

		// New environment just as big as the old environment
		vm.push(Fixnum.get(0));
		vm.push(Fixnum.get(1));
		vm.push(Fixnum.get(2));
		assertEquals(3, vm.getSP());

		vm.mkenv(3, 0);
		assertEquals(6, vm.getSP());
		assertEquals(0, ((Fixnum)vm.getenv(0, 0)).fixnum);
		assertEquals(1, ((Fixnum)vm.getenv(0, 1)).fixnum);
		assertEquals(2, ((Fixnum)vm.getenv(0, 2)).fixnum);

		vm.push(Fixnum.get(3));
		vm.push(Fixnum.get(4));
		vm.push(Fixnum.get(5));
		vm.menv(3);
		vm.mkenv(3, 0);
		assertEquals(6, vm.getSP());
		assertEquals(3, ((Fixnum)vm.getenv(0, 0)).fixnum);
		assertEquals(4, ((Fixnum)vm.getenv(0, 1)).fixnum);
		assertEquals(5, ((Fixnum)vm.getenv(0, 2)).fixnum);

		// Reset environment and stack pointer
		vm.setenvreg(Nil.NIL);
		vm.setSP(0);

		// New environment smaller than old environment
		vm.push(Fixnum.get(0));
		vm.push(Fixnum.get(1));
		vm.push(Fixnum.get(2));
		vm.mkenv(3, 0);
		assertEquals(0, ((Fixnum)vm.getenv(0, 0)).fixnum);
		assertEquals(1, ((Fixnum)vm.getenv(0, 1)).fixnum);
		assertEquals(2, ((Fixnum)vm.getenv(0, 2)).fixnum);

		vm.push(Fixnum.get(6));
		vm.push(Fixnum.get(7));
		vm.menv(2);
		vm.mkenv(2, 0);
		assertEquals(5, vm.getSP());
		assertEquals(6, ((Fixnum)vm.getenv(0, 0)).fixnum);
		assertEquals(7, ((Fixnum)vm.getenv(0, 1)).fixnum);

		// Reset environment and stack pointer
		vm.setenvreg(Nil.NIL);
		vm.setSP(0);
	
		// New environment larger than old environment
		vm.push(Fixnum.get(0));
		vm.push(Fixnum.get(1));
		vm.push(Fixnum.get(2));
		vm.mkenv(3, 0);
		assertEquals(0, ((Fixnum)vm.getenv(0, 0)).fixnum);
		assertEquals(1, ((Fixnum)vm.getenv(0, 1)).fixnum);
		assertEquals(2, ((Fixnum)vm.getenv(0, 2)).fixnum);

		vm.push(Fixnum.get(8));
		vm.push(Fixnum.get(9));
		vm.push(Fixnum.get(10));
		vm.push(Fixnum.get(11));
		vm.menv(4);
		vm.mkenv(4, 0);
		assertEquals(7, vm.getSP());
		assertEquals(8, ((Fixnum)vm.getenv(0, 0)).fixnum);
		assertEquals(9, ((Fixnum)vm.getenv(0, 1)).fixnum);
		assertEquals(10, ((Fixnum)vm.getenv(0, 2)).fixnum);
		assertEquals(11, ((Fixnum)vm.getenv(0, 3)).fixnum);
	}
}
