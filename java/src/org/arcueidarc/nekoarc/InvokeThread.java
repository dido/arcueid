package org.arcueidarc.nekoarc;

import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.util.Callable;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

/** The main reason this class exists is that Java is a weak-sauce language that doesn't have closures or true continuations. We have to
 *  emulate them using threads. */
public class InvokeThread implements Runnable
{
	public final Callable caller;
	public final ArcObject obj;
	public final VirtualMachine vm;

	public InvokeThread(VirtualMachine v, Callable c, ArcObject o)
	{
		vm = v;
		caller = c;
		obj = o;
	}

	@Override
	public void run()
	{
		caller.caller().put(obj.invoke(this));
	}

	public ArcObject getenv(int i, int j)
	{
		return(vm.getenv(i, j));
	}

	/** Perform a call to some Arc object. This should prolly work for ANY ArcObject that has a proper invoke method defined.  If it is a built-in or
	 *  some function defined in Java, that function will run in its own thread while the current object's thread is suspended. */
	public ArcObject apply(ArcObject fn, ArcObject...args)
	{
		// First, push all of the arguments to the stack
		for (ArcObject arg : args)
			vm.push(arg);

		// new continuation
		vm.setCont(new JavaContinuation(obj, vm.getCont()));

		// Apply the function.
		fn.apply(vm, obj);

		return(vm.getAcc());
	}
}
