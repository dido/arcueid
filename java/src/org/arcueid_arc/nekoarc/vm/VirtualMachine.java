package org.arcueid_arc.nekoarc.vm;

import org.arcueid_arc.nekoarc.NekoArcException;
import org.arcueid_arc.nekoarc.Nil;
import org.arcueid_arc.nekoarc.types.ArcObject;
import org.arcueid_arc.nekoarc.vm.instruction.*;

public class VirtualMachine
{
	private int sp;					// stack pointer
	private ArcObject[] stack;		// stack
	private int ip;					// instruction pointer
	private byte[] code;
	private boolean runnable;
	private ArcObject acc;			// accumulator
	private static final INVALID NOINST = new INVALID();
	private static final Instruction[] jmptbl = {
		new NOP(),		// 0x00
		new PUSH(),		// 0x01
		new POP(),		// 0x02
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
		new RET(),		// 0x0d
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		new TRUE(),		// 0x12
		new NIL(),		// 0x13
		new HLT(),		// 0x14
		new ADD(),		// 0x15
		new SUB(),		// 0x16
		new MUL(),		// 0x17
		new DIV(),		// 0x18
		new CONS(),		// 0x19
		new CAR(),		// 0x1a
		new CDR(),		// 0x1b
		new SCAR(),		// 0x1c
		new SCDR(),		// 0x1d
		NOINST,
		new IS(),		// 0x1f
		NOINST,
		NOINST,
		new DUP(),		// 0x22
		new CLS(),		// 0x23
		new CONSR(),		// 0x24
		NOINST,
		new DCAR(),		// 0x26
		new DCDR(),		// 0x27
		new SPL(),		// 0x28
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
		new LDL(),		// 0x43
		new LDI(),		// 0x44
		new LDG(),		// 0x45
		new STG(),		// 0x46
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		NOINST,
		new APPLY(),		// 0x4c
		NOINST,
		new JMP(),		// 0x4e
		new JT(),		// 0x4f
		new JF(),		// 0x50
		new JBND(),		// 0x51
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
		new MENV(),		// 0x65
		NOINST,
		NOINST,
		NOINST,
		new LDE0(),		// 0x69
		new STE0(),		// 0x6a
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
		new LDE(),		// 0x87
		new STE(),		// 0x88
		new CONT(),		// 0x89
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
		new ENV(),		// 0xca
		new ENVR(),		// 0xcb
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
		setAcc(Nil.NIL);
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

	public ArcObject getAcc()
	{
		return acc;
	}

	public void setAcc(ArcObject acc)
	{
		this.acc = acc;
	}

	public boolean runnable()
	{
		return(this.runnable);
	}

	public int getIp()
	{
		return ip;
	}

	public void setIp(int ip)
	{
		this.ip = ip;
	}
}
