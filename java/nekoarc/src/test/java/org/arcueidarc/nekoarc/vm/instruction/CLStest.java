package org.arcueidarc.nekoarc.vm.instruction;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.HeapEnv;
import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.types.Closure;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.vm.VirtualMachine;
import org.junit.Test;

public class CLStest
{
	@Test
	public void test()
	{
		// env 1 0 0 cls 2; ret; ldi 1; ret
		byte inst[] = { (byte)0xca, 0x01, 0x00, 0x00, 0x00,
						0x4d, 0x02, 0x00, 0x00, 0x00,
						0x0d,
						0x44, 0x01, 0x00, 0x00, 0x00,
						0x0d };
		VirtualMachine vm = new VirtualMachine(1024);
		vm.load(inst, 0);
		vm.setAcc(Nil.NIL);
		vm.setargc(1);
		vm.push(Nil.NIL);
		assertTrue(vm.runnable());
		vm.run();
		assertFalse(vm.runnable());
		assertTrue(vm.getAcc() instanceof Closure);
		assertTrue(vm.getAcc().car() instanceof HeapEnv);
		assertEquals(12, ((Fixnum)vm.getAcc().cdr()).fixnum);
		assertEquals(11, vm.getIP());
		
	}

}
