package org.arcueidarc.nekoarc.vm.instruction;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.types.Flonum;
import org.arcueidarc.nekoarc.vm.VirtualMachine;
import org.junit.Test;

public class ADDtest {

	@Test
	public void testFixnumPlusFixnum() throws NekoArcException
	{
		// ldi 1; push; ldi 2; add; hlt
		byte inst[] = { 0x44, (byte) 0x01, (byte) 0x00, (byte) 0x00, (byte) 0x00,
				0x01,
				0x44, (byte) 0x02, (byte) 0x00, (byte) 0x00, (byte) 0x00,
				0x15,
				0x14};
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(3, ((Fixnum)vm.getAcc()).fixnum);
		assertEquals(13, vm.getIP());
	}

	@Test
	public void testFixnumPlusFlonum() throws NekoArcException
	{
		// ldi 1; push; ldl 0; add; hlt
		byte inst[] = { 0x44, (byte) 0x01, (byte) 0x00, (byte) 0x00, (byte) 0x00,
				0x01,
				0x43, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
				0x15,
				0x14};
		ArcObject literals[] = new ArcObject[1];
		literals[0] = new Flonum(3.14);
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0, literals);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(4.14, ((Flonum)vm.getAcc()).flonum, 1e-6);
		assertEquals(13, vm.getIP());
	}

	@Test
	public void testFlonumPlusFixnum() throws NekoArcException
	{
		// ldl 0; push; ldi 1; add; hlt
		byte inst[] = { 0x43, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, 
				0x01,
				0x44, (byte) 0x01, (byte) 0x00, (byte) 0x00, (byte) 0x00,
				0x15,
				0x14};
		ArcObject literals[] = new ArcObject[1];
		literals[0] = new Flonum(3.14);
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0, literals);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(4.14, ((Flonum)vm.getAcc()).flonum, 1e-6);
		assertEquals(13, vm.getIP());
	}

	@Test
	public void testFlonumPlusFlonum() throws NekoArcException
	{
		// ldl 0; push; ldl 1; add; hlt
		byte inst[] = { 0x43, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, 
				0x01,
				0x43, (byte) 0x01, (byte) 0x00, (byte) 0x00, (byte) 0x00,
				0x15,
				0x14};
		ArcObject literals[] = new ArcObject[2];
		literals[0] = new Flonum(3.14);
		literals[1] = new Flonum(2.71);
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0, literals);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(5.85, ((Flonum)vm.getAcc()).flonum, 1e-6);
		assertEquals(13, vm.getIP());
	}

}
