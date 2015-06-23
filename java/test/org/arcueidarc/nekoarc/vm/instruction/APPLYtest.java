package org.arcueidarc.nekoarc.vm.instruction;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.types.Cons;
import org.arcueidarc.nekoarc.vm.VirtualMachine;
import org.junit.Test;

public class APPLYtest
{
	@Test
	public void testCons()
	{
		// cont 13; ldi 2; push; ldl 0; apply 1; ret
		byte inst[] = { (byte)0x89, 0x0d, 0x00, 0x00, 0x00,
				0x44, 0x02, 0x00, 0x00, 0x00,
				0x01,
				0x43, 0x00, 0x00, 0x00, 0x00,
				0x4c, 0x01,
				0x0d };
		ArcObject literals[] = new ArcObject[1];
		literals[0] = new Cons(Fixnum.get(10), new Cons(Fixnum.get(11), new Cons(Fixnum.get(12), Nil.NIL)));
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0, literals);
		vm.setAcc(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(12, ((Fixnum)vm.getAcc()).fixnum);
		assertEquals(19, vm.getIP());
		assertEquals(0, vm.getSP());
	}

	@Test
	public void testClosure()
	{
		// env 1 0 0; ldi 2; push; cls 3; apply 1; ret; env 1 0 0; lde0 0; push; lde 1 0; add; ret
		// more or less the code that should be produced by compiling (fn (x) ((fn (y) (+ x y)) 2))
		byte inst[] = { (byte)0xca, 0x01, 0x00, 0x00, 0x00,
				0x44, 0x02, 0x00, 0x00, 0x00,
				0x01,
				0x4d, 0x03, 0x00, 0x00, 0x00,
				0x4c, 0x01,
				0x0d,
				(byte)0xca, 0x01, 0x00, 0x00, 0x00,
				(byte)0x69, 0x00,
				0x01,
				(byte)0x87, 0x01, 0x00,
				0x15,
				0x0d
		};
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Nil.NIL);
		vm.push(Fixnum.get(5));
		vm.setargc(1);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertEquals(7, ((Fixnum)vm.getAcc()).fixnum);
		assertEquals(32, vm.getIP());
	}
}
