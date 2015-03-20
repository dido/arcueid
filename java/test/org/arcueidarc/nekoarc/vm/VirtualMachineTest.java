package org.arcueidarc.nekoarc.vm;

import static org.junit.Assert.*;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Symbol;
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
}
