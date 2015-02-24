package org.arcueidarc.nekoarc.vm;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.True;
import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.types.Symbol;
import org.arcueidarc.nekoarc.vm.VirtualMachine;
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
		assertEquals(1, vm.getIP());
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
		assertEquals(2, vm.getIP());
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
		assertEquals(2, vm.getIP());
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

		byte data4[] = { (byte) 0x5d, (byte) 0xc3, (byte) 0x1f, (byte) 0x21 };
		vm.load(data4, 0);
		assertEquals(555729757, vm.instArg());
		
		// two's complement negative
		byte data5[] = { (byte) 0xa3, (byte) 0x3c, (byte) 0xe0, (byte) 0xde };
		vm.load(data5, 0);
		assertEquals(-555729757, vm.instArg());

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
		assertEquals(6, vm.getIP());
		
	}

	@Test
	public void testLDL() throws NekoArcException
	{
		// ldl 0; hlt
		byte inst[] = { 0x43, 0x00, 0x00, 0x00, 0x00, 0x14 };
		ArcObject literals[] = new ArcObject[1];
		literals[0] = Symbol.intern("foo");
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0, literals);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(Symbol.intern("foo"), vm.getAcc());
		assertEquals(6, vm.getIP());		
	}

	@Test
	public void testPUSH() throws NekoArcException
	{
		// ldi 555279757; push; ldi -555729757; hlt
		byte inst[] = { 0x44, (byte) 0x5d, (byte) 0xc3, (byte) 0x1f, (byte) 0x21, 0x01,
				0x44, (byte) 0xa3, (byte) 0x3c, (byte) 0xe0, (byte) 0xde, 0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(Fixnum.get(-555729757), vm.getAcc());
		assertEquals(1, vm.getSP());
		assertEquals(Fixnum.get(555729757), vm.pop());
		assertEquals(12, vm.getIP());
	}

	@Test
	public void testPOP() throws NekoArcException
	{
		// ldi 555279757; push; ldi -555729757; pop; hlt
		byte inst[] = { 0x44, (byte) 0x5d, (byte) 0xc3, (byte) 0x1f, (byte) 0x21, 0x01,
				0x44, (byte) 0xa3, (byte) 0x3c, (byte) 0xe0, (byte) 0xde, 0x02, 0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(Fixnum.get(555729757), vm.getAcc());
		assertEquals(0, vm.getSP());
		assertEquals(13, vm.getIP());
	}

	@Test
	public void testTRUE() throws NekoArcException
	{
		byte inst[] = { 0x12, 0x14 };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Fixnum.get(1234));
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(True.T, vm.getAcc());
		assertEquals(2, vm.getIP());
	}

}
