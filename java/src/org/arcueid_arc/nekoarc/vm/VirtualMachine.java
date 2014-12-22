package org.arcueid_arc.nekoarc.vm;

import org.arcueid_arc.nekoarc.types.ArcObject;
import org.arcueid_arc.nekoarc.vm.instruction.*;
import org.arcueid_arc.nekoarc.vm.instruction.Instruction;

public class VirtualMachine
{
	private int sp;					// stack pointer
	private ArcObject[] stack;		// stack
	private int ip;					// instruction pointer
	private byte[] code;
	private boolean runnable;
	private static final Instruction[] jmptbl = {
		new HLT(),
		new LDI(),
		new NIL(),
	};

	public VirtualMachine(int stacksize)
	{
		sp = 0;
		stack = new ArcObject[stacksize];
		ip = 0;
		code = null;
		runnable = true;
	}

	public void load(final byte[] instructions, int ip)
	{
		this.code = instructions;
		this.ip = ip;
	}

	public void halt()
	{
		runnable = false;
	}

	public void nextI()
	{
		ip++;
	}

	public void push(ArcObject obj)
	{
		stack[sp++] = obj;
	}

	public ArcObject pop()
	{
		return(stack[--sp]);
	}

	public void run()
	{
		while (runnable)
			jmptbl[code[ip++]].invoke(this);
	}

	// not sure if this works correctly.
	public int instArg()
	{
		long val = 0;
		ip++;
		int data;
		for (int i=0; i<4; i++) {
			data = (((int)code[ip++]) & 0xff);
			val = (val << 8) | data;
		}
		return((int)((val << 1) >> 1));
	}
}
