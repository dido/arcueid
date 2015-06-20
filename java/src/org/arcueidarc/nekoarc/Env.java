package org.arcueidarc.nekoarc;

import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

/** Stack-based environments. This is a singleton, as it's only used to make accessing
 * 	stack-based environments easier.
 */
public class Env implements Environment
{
	private ArcObject prev;
	private Fixnum start, size;
	private static Env env = new Env();
	private VirtualMachine vm;

	private Env()
	{
	}

	@Override
	public Environment prevEnv()
	{
		return(env(vm, prev));
	}

	public ArcObject getEnv(int index) throws NekoArcException
	{
		if (index > size.fixnum)
			throw new NekoArcException("stack environment index exceeded");
		return(vm.stackIndex((int)start.fixnum + index));
	}

	public ArcObject setEnv(int index, ArcObject value)
	{
		if (index > size.fixnum)
			throw new NekoArcException("stack environment index exceeded");
		return(vm.setStackIndex((int)start.fixnum + index, value));
	}

	private void fromStack(VirtualMachine vm, int index)
	{
		this.vm = vm;
		start = (Fixnum)vm.stackIndex(index);
		size = (Fixnum)vm.stackIndex(index+1);
		prev = vm.stackIndex(index+2);
	}

	public static Environment env(VirtualMachine vm, ArcObject envr)
	{
		if (envr instanceof Fixnum) {
			env.fromStack(vm, (int)((Fixnum)envr).fixnum);
			return(env);
		}
		if (envr.is(Nil.NIL))
			return(null);
		return((Environment)envr);
	}
}
