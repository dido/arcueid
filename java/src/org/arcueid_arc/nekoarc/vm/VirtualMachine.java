package org.arcueid_arc.nekoarc.vm;

import org.arcueid_arc.nekoarc.NekoArcException;
import org.arcueid_arc.nekoarc.types.ArcObject;
import org.arcueid_arc.nekoarc.vm.instruction.*;

public class VirtualMachine
{
	private int sp;					// stack pointer
	private ArcObject[] stack;		// stack
	private int ip;					// instruction pointer
	private byte[] code;
	private boolean runnable;
	private static final INVALID NOINST = new INVALID();
	private static final Instruction[] jmptbl = {
		new NOP(),
		new PUSH(),
		new POP(),
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		new RET(),
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		new TRUE(),
		new NIL(),
		new HLT(),
		new ADD(),
		new SUB(),
		new MUL(),
		new DIV(),
		new CONS(),
		new CAR(),
		new CDR(),
		new SCAR(),
		new SCDR(),
		NOINST,
		new IS(),
		NOINST,
		NOINST,
		new DUP(),
		new CLS(),
		new CONSR(),
		NOINST,
		new DCAR(),
		new DCDR(),
		new SPL(),
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		new LDL(),
		new LDI(),
		new LDG(),
		new STG(),
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		new APPLY(),
		NOINST,
		new JMP(),
		new JT(),
		new JF(),
		new JBND(),
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		new MENV(),
		NOINST,
		NOINST,
		NOINST,
		new LDE0(),
		new STE0(),
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		new LDE(),
		new STE(),
		new CONT(),
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		new ENV(),
		new ENVR(),
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
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
		throws NekoArcException
	{
		while (runnable)
			jmptbl[code[ip++]].invoke(this);
	}

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
